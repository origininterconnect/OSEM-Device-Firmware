/**
 * @file main.cpp
 * @brief Adds host-driven UTC time sync atop the original ADS1299 streaming code.
 */

#include <WiFi.h>
#include <WiFiManager.h>
#include <WebSocketsServer.h>
#include <Adafruit_NeoPixel.h>
#include <ESPmDNS.h>
#include "config.h"
#include "ADS1299.h"

// 24-bit 4/6/8-channel Analog frontend
ADS1299 ADS;

// WebSockets on port 81
WebSocketsServer webSocket(81);

// Onboard NeoPixel on pin 6
Adafruit_NeoPixel pixels(1, 6, NEO_GRB + NEO_KHZ800);

// WiFi AP credentials (fallback)
const char *ssid     = "ORIC-EEG";
const char *password = "";

// --- UTC Sync Globals ---
unsigned long long utc_offset_ms = 0;   // Base UTC time in ms from host
bool time_synced = false; // Have we received a SYNC yet?
bool ADS_connected = false; // True if device detected

unsigned long sync_micros = 0;     // micros() when sync arrived

// Returns current UTC timestamp (ms) as host-provided offset + elapsed
unsigned long getUTCTimestamp() {
  if (!time_synced) return 0;
  unsigned long elapsed_ms = (micros() - sync_micros) / 1000;
  return utc_offset_ms + elapsed_ms;
}

// Only send once both ADS and time are ready
bool isReadyToSendData() {
  return time_synced && ADS_connected;
}

// ESP memory & ADS error helpers
void ESPmemcheck();
void ADSerrorcheck();

// Global variables
const char *hardware_type = ""; // can be 4/6/8 ch device type
int max_channels = 0; // can be 4/6/8
bool data_ready = false; // Data ready pin (DRDY) state
static int dataQueueLen = 4000; // Queue length for ADC data
static QueueHandle_t dataQueue; // Queue
static TaskHandle_t ws_task = NULL; // WebSockets task
static TaskHandle_t ads_task = NULL; // Data acquisition task
static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;
bool wm = false; // WiFiManager portal access

// Data Buffers
uint8_t packetBytes[PACKET_SIZE]; // Single packet of length PACKET_SIZE in Bytes
uint8_t blockBytes[BLOCK_SIZE]; // Single block of BLOCK_SIZE (Default:32) in Bytes
uint8_t statusBytes[ADS_STATUS_SIZE]; // Status Bytes of size ADS_STATUS_SIZE

// UTC timestamp (8 bytes for 64-bit ms precision)
#define UTC_TIMESTAMP_SIZE_IN_BYTES 8
union {
    char timestamp_bytes[UTC_TIMESTAMP_SIZE_IN_BYTES];
    unsigned long long timestamp;
} utc_timestamp_union;

// sample number counter
#define SAMPLE_NUMBER_SIZE_IN_BYTES 4
union {
    char sample_number_bytes[SAMPLE_NUMBER_SIZE_IN_BYTES];
    unsigned long sample_number = 0;
} sample_number_union;

// Calculate SPI data sizes
int num_spi_bytes = (3 * (max_channels + 1));
int num_timestamped_spi_bytes = num_spi_bytes + UTC_TIMESTAMP_SIZE_IN_BYTES + SAMPLE_NUMBER_SIZE_IN_BYTES;

// Data ready interrupt service routine 
void IRAM_ATTR DRDY_ISR(void) {
  portDISABLE_INTERRUPTS();
  BaseType_t task_woken = pdFALSE;

  vTaskNotifyGiveFromISR(ads_task, &task_woken);

  if (task_woken) {
    portYIELD_FROM_ISR();
  }
}

// Data acquisition and storing in queue
void ads_receiveQueue(void *parameter) {
  while(1) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    
    // Only process if we have time sync
    if (!time_synced) {
      continue;
    }
    
    portENTER_CRITICAL(&spinlock);
    
    // Get UTC timestamp (8 bytes, little-endian)
    utc_timestamp_union.timestamp = getUTCTimestamp();
    
    // Add UTC timestamp bytes to data (first 8 bytes)
    for(int i = 0; i < UTC_TIMESTAMP_SIZE_IN_BYTES; i++) {
      blockBytes[i] = utc_timestamp_union.timestamp_bytes[i];
    }
    
    // Add sample counter bytes to data (next 4 bytes)
    for(int i = 0; i < SAMPLE_NUMBER_SIZE_IN_BYTES; i++) {
      blockBytes[UTC_TIMESTAMP_SIZE_IN_BYTES + i] = sample_number_union.sample_number_bytes[i];
    }

    // ADS1299.cpp -> ADS1299::readData (after timestamp and sample number)
    bool dataStat = ADS.readData(statusBytes, blockBytes + UTC_TIMESTAMP_SIZE_IN_BYTES + SAMPLE_NUMBER_SIZE_IN_BYTES);
    
    if(!dataStat){
      if(xQueueSend(dataQueue, (void*)&blockBytes, 0) != pdTRUE) {
        Serial.println("Data Queue full");
      }
    } else {
      Serial.println("Data reading issue");
    }
    sample_number_union.sample_number++;
    portEXIT_CRITICAL(&spinlock);
  }
}

// Send data over websockets
void ws_sendQueue(void *parameter) {
  while(1) {
    // ALWAYS call webSocket.loop() first to process events (including sync messages)
    webSocket.loop();
    
    // Only send data when both ADS and time are synced
    if (!isReadyToSendData()) {
      vTaskDelay(10 / portTICK_PERIOD_MS); // Shorter delay for more responsive sync
      continue;
    }
    
    memset(packetBytes, 0, sizeof(packetBytes));
    for(int block=0; block<TOTAL_BLOCKS; block++) {
      // Get data from queue
      uint8_t blockBytes[BLOCK_SIZE];
      while(xQueueReceive(dataQueue, (void*)&blockBytes, 0) != pdTRUE) vTaskDelay(1/portTICK_PERIOD_MS);
      // Add block of data to packet
      unsigned int block_offset = block * BLOCK_SIZE;
      memcpy((uint8_t *)&packetBytes[block_offset], (void*)&blockBytes, BLOCK_SIZE);
    }
    webSocket.sendBIN(0, (uint8_t *)&packetBytes, sizeof(packetBytes));
    vTaskDelay(1/portTICK_PERIOD_MS);
  }
}

// WebSocket event handler - handles connection management and UTC sync
void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length) {      
  switch (type) {                                     
    case WStype_DISCONNECTED:                         
      Serial.println("Client " + String(num) + " disconnected");
      // Reset time sync on disconnect
      time_synced = false;
      utc_offset_ms = 0;
      sample_number_union.sample_number = 0; // Reset sample counter
      pixels.setPixelColor(0, pixels.Color(PIXEL_BRIGHTNESS, PIXEL_BRIGHTNESS, 0)); // yellow
      pixels.show();
      break;
      
    case WStype_CONNECTED:                            
      Serial.println("Client " + String(num) + " connected");
      pixels.setPixelColor(0, pixels.Color(0, PIXEL_BRIGHTNESS, PIXEL_BRIGHTNESS)); // cyan
      pixels.show();
      break;
      
    case WStype_TEXT:                                 
      // Handle text commands
      if (length >= 4 && strncmp((char*)payload, "SYNC", 4) == 0) {
        Serial.println("SYNC command received");
        webSocket.sendTXT(num, "SYNC_READY");
      }
      // Log other text data
      for (int i=0; i<length; i++) {                  
        log_d((char)payload[i]);
      }
      log_d("");
      break;
      
    case WStype_BIN:
      // Handle UTC time sync - expect exactly 8-byte little-endian UTC ms
      if (length == 8) {
        unsigned long long recvUtc = 0;
        for (int i = 0; i < 8; i++) {
          recvUtc |= ((unsigned long long)payload[i]) << (8 * i);
        }
        utc_offset_ms = recvUtc;
        sync_micros = micros();
        time_synced = true;
        sample_number_union.sample_number = 0; // Reset sample counter on sync

        Serial.print("UTC sync completed: ");
        Serial.println(utc_offset_ms);
        webSocket.sendTXT(num, "TIME_SYNCED");

        pixels.setPixelColor(0, pixels.Color(0, 0, PIXEL_BRIGHTNESS)); // blue
        pixels.show();
      }
      break;
      
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    default:
      break;
  }
}

// Setup
void setup() {
  Serial.begin(115200);
  vTaskDelay(100/portTICK_PERIOD_MS);
  
  // Trigger pin to put device in AP mode
  pinMode(TRIGGER_PIN, INPUT_PULLUP);

  // Data queue
  dataQueue = xQueueCreate(dataQueueLen, sizeof(blockBytes));
  WiFi.mode(WIFI_AP_STA);

  // Initiate wifimanager
  WiFiManager wifiManager;

  // Set new pixel
  pixels.begin();
  pixels.setPixelColor(0, pixels.Color(PIXEL_BRIGHTNESS, 0, 0)); //RED
  pixels.show();
  vTaskDelay(100/portTICK_PERIOD_MS);

  // Check for WiFiManager portal trigger
  for(int i=0; i<10; i++){
    vTaskDelay(100/portTICK_PERIOD_MS);
    if(digitalRead(TRIGGER_PIN)== LOW) {
      wm = true;
      pixels.setPixelColor(0, pixels.Color(PIXEL_BRIGHTNESS, 0, PIXEL_BRIGHTNESS)); // magenta
      pixels.show();
    }
    vTaskDelay(1/portTICK_PERIOD_MS);
  }

  // Allow to put device into AP mode
  for(int i=0; i<10; i++){
    vTaskDelay(100/portTICK_PERIOD_MS);
    if(digitalRead(TRIGGER_PIN)== LOW) {
      wifiManager.resetSettings();
      pixels.setPixelColor(0, pixels.Color(0, 0, PIXEL_BRIGHTNESS)); // BLUE
      pixels.show();
    }
    vTaskDelay(1/portTICK_PERIOD_MS);
  }

  pixels.setPixelColor(0, pixels.Color(0, PIXEL_BRIGHTNESS, 0)); // Green
  pixels.show();

  // Try to connect
  if (wm && !wifiManager.autoConnect("CORTX-FM5", "sleepyeeg")) {
    Serial.println("failed to connect, we should reset and see if it connects");
    vTaskDelay(1/portTICK_PERIOD_MS);
    ESP.restart();
  } else {
      if (!WiFi.softAP(ssid, password)) {
        ESP_LOGE("APMode","APModeStatus: %s","Soft AP creation failed!");
        while(1){
            vTaskDelay(1/portTICK_PERIOD_MS);
            pixels.setPixelColor(0, pixels.Color(PIXEL_BRIGHTNESS, 0, 0)); //RED
            pixels.show();
            ESP.restart();
        }
      } else {
        ESP_LOGI("APMode","APModeStatus: %s","Soft AP creation success!");
        pixels.setPixelColor(0, pixels.Color(0, PIXEL_BRIGHTNESS, PIXEL_BRIGHTNESS)); // cyan
        pixels.show();
      }
  }

  // Initiate MDNS
  if (!MDNS.begin("oric")) {
    Serial.println("Error setting up MDNS responder!");
    while(1) {
        vTaskDelay(1/portTICK_PERIOD_MS);
        pixels.setPixelColor(0, pixels.Color(PIXEL_BRIGHTNESS, 0, 0)); //RED
        pixels.show();
        ESP.restart();
    }
  } else {
    Serial.println("mDNS responder started");
    pixels.setPixelColor(0, pixels.Color(0, PIXEL_BRIGHTNESS, PIXEL_BRIGHTNESS)); // cyan
    pixels.show();
  }

  // Create websocket connection
  webSocket.begin();                                 
  webSocket.onEvent(webSocketEvent);
  MDNS.addService("http", "tcp", 80);

  vTaskDelay(100/portTICK_PERIOD_MS);

  // Setup ADS
  ADS.setup(PIN_DRDY, PIN_CS);
  delay(1);
  ESPmemcheck();
  ADSerrorcheck();
  
  // Recalculate SPI data sizes after ADS detection
  num_spi_bytes = (3 * (max_channels + 1));
  num_timestamped_spi_bytes = num_spi_bytes + UTC_TIMESTAMP_SIZE_IN_BYTES + SAMPLE_NUMBER_SIZE_IN_BYTES;
  
  // Set ADS bit configuration
  ADS.setSingleended();

  // websockets task
  xTaskCreatePinnedToCore(
    ws_sendQueue,
    "Send queue",
    4096,
    NULL,
    1,
    &ws_task,
    0
  );

  // ADS task
  xTaskCreatePinnedToCore(
    ads_receiveQueue,
    "ADS Receive",
    4096,
    NULL,
    2,
    &ads_task,
    1  // Pin to core 1 for better performance
  );

  // interrupt for DRDY
  attachInterrupt(digitalPinToInterrupt(PIN_DRDY), DRDY_ISR, FALLING);
  
  Serial.println("Setup complete. Waiting for UTC time sync...");
}

void loop() {
  // Main loop is empty - FreeRTOS tasks handle everything
}

void ESPmemcheck() {
  Serial.printf("Total heap: %u\n", ESP.getHeapSize());
  Serial.printf("Free heap: %u\n", ESP.getFreeHeap());
  Serial.printf("Minimum free heap: %u\n", ESP.getMinFreeHeap());
  Serial.printf("Total PSRAM: %u\n", ESP.getPsramSize());
  Serial.printf("Free PSRAM: %u\n", ESP.getFreePsram());
}

void ADSerrorcheck() { 
  int val = ADS.getDeviceID();
  Serial.printf("ADS Device ID: %d\n", val);
  
  switch (val & B00011111) {
    case B11110:
        hardware_type = "ADS1299";
        max_channels = 8;
        break;
    case B11100:
        hardware_type = "ADS1299-4";
        max_channels = 4;
        break;
    case B11101:
        hardware_type = "ADS1299-6";
        max_channels = 6;
        break;
    default:
        hardware_type = "Unknown";
        max_channels = 0;
  }

  if (max_channels == 0) {
    Serial.println("ADS1299 connection issue");
    delay(500);
    ADS.setup(PIN_DRDY, PIN_CS);
    ADS_connected = false;
  } else {
    Serial.print("Detected: ");
    Serial.println(hardware_type);
    ADS_connected = true;
  }
}
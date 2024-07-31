// Copyright 2024 OriginInterconnect PVT. LTD.
//
// Commons Clause
//
// The Software is provided to you by the Licensor under the License, as 
// defined below, subject to the following condition.
//
// Without limiting other conditions in the License, the grant of rights 
// under the License will not include, and the License does not grant you, 
// the right to Sell the Software or use it for any Commercial Purposes.
//
// For purposes of the foregoing, “Sell” or "Commercial Purposes" means 
// practicing any or all of the rights granted to you under the License 
// to provide to third parties, for a fee or other consideration 
// (including without limitation fees for hosting or consulting/support 
// services related to the Software), a product or service whose value 
// derives, entirely or substantially, from the functionality of the 
// Software. Any license notice or attribution required by the License 
// must also include this Commons Clause License Condition notice.
//
// Software: OSEM-Device-Firmware
//
// License: Apache 2.0
//
// Licensor: OriginInterconnect PVT. LTD.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <WiFi.h>
#include <WiFiManager.h>
#include <WebSocketsServer.h>
#include <Adafruit_NeoPixel.h>
#include <ESPmDNS.h>
#include <ADS1299config.h>
#include <ADS1299.h>
#include "esp_log.h"

// 24bit 4/6/8 channel Analog frontend
ADS1299 ADS;
// Websockets connection on port 81
WebSocketsServer webSocket = WebSocketsServer(81);
// Onboard neopixel at pin 6
Adafruit_NeoPixel pixels(1, 6, NEO_GRB + NEO_KHZ800);

const char *ssid = "ORIC-EEG";
const char *password = "";

// ESP memory check
void ESPmemcheck();
// ADS hardware detection and error check
void ADSerrorcheck();

// Global variables
const char *hardware_type = "";      // can be 4/6/8 ch device type
int max_channels = 0;                // can be 4/6/8
static int dataQueueLen = 4000;      // Queue length for ADC data
static QueueHandle_t dataQueue;      // Queue
static TaskHandle_t ws_task = NULL;  // WebSockets task
bool wm = false; // WiFiManager portal access

int TOTAL_BLOCKS = 9; // Number of blocks to send in a packet

void webSocketEvent(byte num, WStype_t type, uint8_t *payload, size_t length);

// Data ready interrupt service routine
void IRAM_ATTR DRDY_ISR(void)
{
  uint8_t blockBytes[ADS_DATA_SIZE];
  uint8_t statusBytes[ADS_STATUS_SIZE];

  bool dataStat = ADS.readData(statusBytes, blockBytes);
  if (!dataStat)
  {
    if (xQueueSendFromISR(dataQueue, (void *)&blockBytes, 0) != pdTRUE)
    {
      ESP_LOGE("ads_receiveQueue", "Data Queue full");
    }
  }
  else
  {
    ESP_LOGE("ads_recieveQueue", "Data reading issue");
  }
}
bool ws_connected = false;
// Send data over websockets
void ws_sendQueue(void *parameter)
{
  ESP_LOGI("ws_sendQueue", "Stream started");
  uint8_t counter = 0;

  while (1)
  {
    if (ws_connected)
    {
      uint8_t *packetBytes = (uint8_t *)calloc(TOTAL_BLOCKS * BLOCK_SIZE, sizeof(uint8_t));
      memset(packetBytes, 0, TOTAL_BLOCKS * BLOCK_SIZE);
      for (int block = 0; block < TOTAL_BLOCKS; block++)
      {
        // Get data from queue
        uint8_t blockBytes[BLOCK_SIZE] = {0};
        blockBytes[0] = counter;
        while (xQueueReceive(dataQueue, (void *)&blockBytes[1], 0) != pdTRUE)
          vTaskDelay(1 / portTICK_PERIOD_MS);
        unsigned int block_offset = block * BLOCK_SIZE;
        memcpy((uint8_t *)&packetBytes[block_offset], (void *)&blockBytes, BLOCK_SIZE);
        counter = counter < 255 ? ++counter : 0;
      }
      webSocket.sendBIN(0, packetBytes, TOTAL_BLOCKS * BLOCK_SIZE);
      free(packetBytes);
      ESP_LOGD("ws_sendQueue", "ws working");
    }
    webSocket.loop();
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

// Setup
void setup()
{
  Serial.begin(115200);
  vTaskDelay(100 / portTICK_PERIOD_MS);

  // Trigger pin to put device in AP mode
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  dataQueue = xQueueCreate(dataQueueLen, ADS_DATA_SIZE);

  WiFi.mode(WIFI_AP_STA);

  // Initiate wifimanager
  WiFiManager wifiManager;

  // Set new pixel
  pixels.begin();
  pixels.setPixelColor(0, pixels.Color(PIXEL_BRIGHTNESS, 0, 0)); // RED
  pixels.show();
  vTaskDelay(100 / portTICK_PERIOD_MS);

  for (int i = 0; i < 10; i++)
  {
    vTaskDelay(100 / portTICK_PERIOD_MS);
    if (digitalRead(TRIGGER_PIN) == LOW)
    {
      wm = true;
      pixels.setPixelColor(0, pixels.Color(PIXEL_BRIGHTNESS, 0, PIXEL_BRIGHTNESS));
      pixels.show();
    }
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }

  // Allow to put device into AP mode
  for (int i = 0; i < 10; i++)
  {
    vTaskDelay(100 / portTICK_PERIOD_MS);
    if (digitalRead(TRIGGER_PIN) == LOW)
    {
      wifiManager.resetSettings();
      pixels.setPixelColor(0, pixels.Color(0, 0, PIXEL_BRIGHTNESS)); // BLUE
      pixels.show();
    }
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }

  pixels.setPixelColor(0, pixels.Color(0, PIXEL_BRIGHTNESS, 0)); // Green
  pixels.show();

  // Try to connect
  if (wm and !wifiManager.autoConnect("ORIC-OSEM-DEVICE-OTA", "awesomeness"))
  {
    ESP_LOGE("WiFI manager", "failed to connect, we should reset and see if it connects");
    vTaskDelay(1 / portTICK_PERIOD_MS);
    ESP.restart();
  }
  else
  {
    if (!WiFi.softAP(ssid, password))
    {
      ESP_LOGE("APMode", "APModeStatus: Soft AP creation failed!");
      while (1)
      {
        vTaskDelay(1 / portTICK_PERIOD_MS);
        pixels.setPixelColor(0, pixels.Color(PIXEL_BRIGHTNESS, 0, 0)); // RED
        pixels.show();
        ESP.restart();
      }
    }
    else
    {
      ESP_LOGD("APMode", "APModeStatus: soft AP creation success!");
      pixels.setPixelColor(0, pixels.Color(0, PIXEL_BRIGHTNESS, PIXEL_BRIGHTNESS)); // cyan
      pixels.show();
    }
  }
  // Initiate MDNS
  if (!MDNS.begin("oric"))
  {
    ESP_LOGE("mdns", "Error setting up MDNS responder!");
    while (1)
    {
      vTaskDelay(1 / portTICK_PERIOD_MS);
      pixels.setPixelColor(0, pixels.Color(PIXEL_BRIGHTNESS, 0, 0)); // RED
      pixels.show();
      ESP.restart();
    }
  }
  else
  {
    ESP_LOGD("mdns", "mDNS responder started");
    pixels.setPixelColor(0, pixels.Color(0, PIXEL_BRIGHTNESS, PIXEL_BRIGHTNESS)); // cyan
    pixels.show();
  }
  // Create weboscket connection
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  MDNS.addService("http", "tcp", 80);

  vTaskDelay(100 / portTICK_PERIOD_MS);

  // Setup ADS
  ADS.setup(PIN_DRDY, PIN_CS);
  delay(1);
  ESPmemcheck();
  ADSerrorcheck();
  // Set ADS bit configuration
  ADS.setSingleended(max_channels);

  // websockets task
  xTaskCreatePinnedToCore(
      ws_sendQueue,
      "Send queue",
      4096,
      NULL,
      6,
      &ws_task,
      0);

  // interrupt for DRDY
  attachInterrupt(0, DRDY_ISR, FALLING);
}

void loop() {}

int samplingRate[7] = {16000, 8000, 4000, 2000, 1000, 500, 250};

void setsamplingrate(int samplingrate, int FPS)
{
  if (samplingrate > 1000)
  {
    TOTAL_BLOCKS = FPS ? ceil(samplingrate / 30) : samplingrate / 10;
  }
  else
  {
    TOTAL_BLOCKS = FPS ? samplingrate / 25 : samplingrate / 5;
  }
  ESP_LOGD("setSamplingRate", "Sampling rate: %d, FPS: %d", samplingrate, FPS);
}
void webSocketEvent(byte num, WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED:
    ws_connected = false;
    ESP_LOGI("WS-msg", "Client %d disconnected", num);
    pixels.setPixelColor(0, pixels.Color(PIXEL_BRIGHTNESS, PIXEL_BRIGHTNESS, 0));
    pixels.show();
    ADS.SDATAC();
    ADS.STOP();
    break;
  case WStype_CONNECTED:
    ESP_LOGI("WS-msg", "Client %d connected", num);
    pixels.setPixelColor(0, pixels.Color(0, PIXEL_BRIGHTNESS, PIXEL_BRIGHTNESS));
    pixels.show();
    break;
  case WStype_TEXT:
    ESP_LOGI("WS-msg", "Received Text data: %s", payload);
    break;
  case WStype_BIN:
    ESP_LOGI("WS-msg", "Received binary data");
    if ((payload[0] & 0x60) == 0x00 && (payload[0] != 0x00))
    {
      int sr_index = (int)(payload[0] & 0x0E) >> 1;
      int fps_index = (int)(payload[0] & 0x01);
      byte sr_command = sr_index | (0x90 & 0xF8);
      ESP_LOGD("WS-msg", "sr_command %x", sr_command);
      ADS.SDATAC();
      ADS.WREG(CONFIG1, sr_command);
      setsamplingrate(samplingRate[sr_index], fps_index);
      ADS.START();
      ADS.RDATAC();

      ws_connected = true;
      ESP_LOGD("WS-msg", "Sampling rate:%d sr_index: %d FPS: %d ", samplingRate[sr_index], sr_index, fps_index);
    }
    else if ((payload[0] & 0x60) == 0x40)
    {
      ESP_LOGD("WS-msg", "Gain Command");
      byte value = ((payload[0] & 0x07) << 4) | 0x00;
      ESP_LOGD("WS-msg", "Gain value: %x", value);
      ADS.setSingleended(max_channels, value);
    }
    else if (payload[0] == 0x00)
    {
      ESP_LOGD("WS-msg", "Impedance command");
      ADS.SDATAC();
      ADS.WREG(LOFF, 0x02);                             // set lead off 6nA and 31.2Hz
      ADS.WREG(LOFF_SENSP, 0xFF >> (8 - max_channels)); // set max channels to lead off
      ADS.RDATAC();
    }
    ESP_LOGD("WS-msg", "Bin length: %d", length);
  }
}

void ESPmemcheck()
{
  ESP_LOGD("ESP_mem", "Total heap: %d", ESP.getHeapSize());
  ESP_LOGD("ESP_mem", "Free heap: %d", ESP.getFreeHeap());
  ESP_LOGD("ESP_mem", "Minimum free heap: %d", ESP.getMinFreeHeap());
  ESP_LOGD("ESP_mem", "Total PSRAM: %d", ESP.getPsramSize());
  ESP_LOGD("ESP_mem", "Free PSRAM: %d", ESP.getFreePsram());
}

void ADSerrorcheck()
{
  int val = ADS.getDeviceID();
  switch (val & B00011111)
  {
  case B11110:
    hardware_type = "ADS1299";
    max_channels = 8;
    break;
  case B11101:
    hardware_type = "ADS1299-6";
    max_channels = 6;
    break;
  case B11100:
    hardware_type = "ADS1299-4";
    max_channels = 4;
    break;
  default:
    max_channels = 0;
  }

  if (max_channels == 0)
  {
    ESP_LOGE("ADS error check", "ADS1299 connection issue");
  }
  else
  {
    ESP_LOGD("ADS error check", "Detected: %s", hardware_type);
  }
}

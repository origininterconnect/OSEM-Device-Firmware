/**
 *
 * Copyright (c) 2024 OriginInterconnect PVT. LTD.
 * Copyright (c) 2024 Deepak Khatri <deepak@oric.io>
 * Copyright (c) 2024 Mahesh Tupe
 * Copyright (C) 2013-2019 Adam Feuer <adam@adamfeuer.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <ads129x.h>
#include <adscommand.h>
#include <wscommand.h>
#include <spidma.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include <Adafruit_NeoPixel.h>

const char *ssid = "ORIC-EEG";
const char *password = "";

#define BAUD_RATE 2000000
#define BOARD_NAME "OctaEEG"
#define CHANNELS 8
#define ADS_DATA_SIZE (CHANNELS * 3)
#define ADS_STATUS_SIZE 3
#define BLOCK_SIZE 32 // Data + Timestamp + Counter
#define SAMPLES_PER_BUFFER 250
#define PACKET_SIZE (BLOCK_SIZE * SAMPLES_PER_BUFFER)
#define NUM_BUFFERS 20
#define MAX_PAYLOAD_SIZE 256

uint8_t data_buffers[NUM_BUFFERS][PACKET_SIZE];
volatile int current_buffer_index = 0;
volatile int current_sample_index = 0;
volatile bool buffer_completed[NUM_BUFFERS] = {false};
uint8_t buffer_to_send = 0;

const char *STATUS_TEXT_OK = "Ok";
const char *STATUS_TEXT_BAD_REQUEST = "Bad request";
const char *STATUS_TEXT_ERROR = "Error";
const char *STATUS_TEXT_NOT_IMPLEMENTED = "Not Implemented";
const char *STATUS_TEXT_NO_ACTIVE_CHANNELS = "No Active Channels";
bool wm = false;

int max_channels = 0;
int num_active_channels = 0;
boolean active_channels[9];
boolean is_rdatac = false;

// microseconds timestamp
#define TIMESTAMP_SIZE_IN_BYTES 4
union
{
    uint8_t timestamp_bytes[TIMESTAMP_SIZE_IN_BYTES];
    uint32_t timestamp;
} timestamp_union;

// sample number counter
#define SAMPLE_NUMBER_SIZE_IN_BYTES 4
union
{
    uint8_t sample_number_bytes[SAMPLE_NUMBER_SIZE_IN_BYTES];
    uint32_t sample_number = 0;
} sample_number_union;

const char *hardware_type = "OSEM-DEVICE";
const char *board_name = BOARD_NAME;
const char *maker_name = "OriginInterconnect PVT. LTD.";
const char *driver_version = "v0.0.1";

WSCommand wsCommand;
WebSocketsServer webSocket = WebSocketsServer(81);
Adafruit_NeoPixel pixels(1, PIN_NEO, NEO_GRB + NEO_KHZ800);
void webSocketEvent(byte num, WStype_t type, uint8_t *payload, size_t length);

void espSetup();
void adsSetup();
void detectActiveChannels();
void unrecognized(const char *);
void nopCommand(unsigned char unused1, unsigned char unused2);
void microsCommand(unsigned char unused1, unsigned char unused2);
void versionCommand(unsigned char unused1, unsigned char unused2);
void statusCommand(unsigned char unused1, unsigned char unused2);
void serialNumberCommand(unsigned char unused1, unsigned char unused2);
void ledOnCommand(unsigned char unused1, unsigned char unused2);
void ledOffCommand(unsigned char unused1, unsigned char unused2);
void boardLedOffCommand(unsigned char unused1, unsigned char unused2);
void boardLedOnCommand(unsigned char unused1, unsigned char unused2);
void wakeupCommand(unsigned char unused1, unsigned char unused2);
void standbyCommand(unsigned char unused1, unsigned char unused2);
void resetCommand(unsigned char unused1, unsigned char unused2);
void startCommand(unsigned char unused1, unsigned char unused2);
void stopCommand(unsigned char unused1, unsigned char unused2);
void rdatacCommand(unsigned char unused1, unsigned char unused2);
void sdatacCommand(unsigned char unused1, unsigned char unused2);
void readRegisterCommand(unsigned char unused1, unsigned char unused2);
void writeRegisterCommand(unsigned char register_number, unsigned char register_value);
void helpCommand(unsigned char unused1, unsigned char unused2);

void setup()
{
    Serial.begin(BAUD_RATE);
    while (!Serial)
    {
        ;
    }

    // Hardware setup
    espSetup();
    adsSetup();

    // Setup callbacks for SerialCommand commands
    wsCommand.addCommand("nop", nopCommand);                   // No operation (does nothing)
    wsCommand.addCommand("micros", microsCommand);             // Returns number of microseconds since the program began executing
    wsCommand.addCommand("version", versionCommand);           // Echos the driver version number
    wsCommand.addCommand("status", statusCommand);             // Echos the driver status
    wsCommand.addCommand("serialnumber", serialNumberCommand); // Echos the board serial number (UUID from the onboard 24AA256UID-I/SN I2S EEPROM)
    wsCommand.addCommand("ledon", ledOnCommand);               // Turns Arduino Due onboard LED on
    wsCommand.addCommand("ledoff", ledOffCommand);             // Turns Arduino Due onboard LED off
    wsCommand.addCommand("boardledoff", boardLedOffCommand);   // Turns HackEEG ADS1299 GPIO4 LED off
    wsCommand.addCommand("boardledon", boardLedOnCommand);     // Turns HackEEG ADS1299 GPIO4 LED on
    wsCommand.addCommand("wakeup", wakeupCommand);             // Send the WAKEUP command
    wsCommand.addCommand("standby", standbyCommand);           // Send the STANDBY command
    wsCommand.addCommand("reset", resetCommand);               // Reset the ADS1299
    wsCommand.addCommand("start", startCommand);               // Send START command
    wsCommand.addCommand("stop", stopCommand);                 // Send STOP command
    wsCommand.addCommand("rdatac", rdatacCommand);             // Enter read data continuous mode, clear the ringbuffer, and read new data into the ringbuffer
    wsCommand.addCommand("sdatac", sdatacCommand);             // Stop read data continuous mode; ringbuffer data is still available
    wsCommand.addCommand("rreg", readRegisterCommand);         // Read ADS129x register, argument in hex, print contents in hex
    wsCommand.addCommand("wreg", writeRegisterCommand);        // Write ADS129x register, arguments in hex
    wsCommand.addCommand("help", helpCommand);                 // Print list of commands
    wsCommand.setDefaultHandler(unrecognized);
    pinMode(TRIGGER_PIN, INPUT_PULLUP);

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
        pixels.setPixelColor(0, pixels.Color(PIXEL_BRIGHTNESS, PIXEL_BRIGHTNESS, 0)); // Yellow
        pixels.show();
    }

    // Create weboscket connection
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
    MDNS.addService("http", "tcp", 80);
    ESP_LOGD("SETUP", "Ready");
}

void loop()
{
    if (buffer_completed[buffer_to_send])
    {
        // Send the current buffer via WebSocket
        webSocket.sendBIN(0, (uint8_t *)&data_buffers[buffer_to_send], PACKET_SIZE);
        
        vTaskDelay(20 / portTICK_PERIOD_MS);
        // Move to the next buffer in sequence
        buffer_completed[buffer_to_send] = false;
        buffer_to_send = (buffer_to_send + 1) % NUM_BUFFERS;
    }

    // Regularly handle WebSocket events
    webSocket.loop();
}

void webSocketEvent(byte num, WStype_t type, uint8_t *payload, size_t length)
{ // the parameters of this callback function are always the same -> num: id of the client who send the event, type: type of message, payload: actual data sent and length: length of payload
    switch (type)
    {                         // switch on the type of information sent
    case WStype_DISCONNECTED: // if a client is disconnected, then type == WStype_DISCONNECTED
        ESP_LOGD("WEBSOCKET", "Client %d disconnected", num);
        pixels.setPixelColor(0, pixels.Color(PIXEL_BRIGHTNESS, PIXEL_BRIGHTNESS, 0)); // Yellow
        pixels.show();
        break;
    case WStype_CONNECTED: // if a client is connected, then type == WStype_CONNECTED
        ESP_LOGD("WEBSOCKET", "Client %d connected", num);
        pixels.setPixelColor(0, pixels.Color(0, PIXEL_BRIGHTNESS, PIXEL_BRIGHTNESS)); // cyan
        pixels.show();
        break;
    case WStype_TEXT: // if a client has sent data, then type == WStype_TEXT
        ESP_LOGD("WEBSOCKET", "Received command from user: %d", num);
        wsCommand.executeCommand(payload);
        break;
    }
}

void send_json_respose(JsonDocument &doc)
{
    String jsonString = "";
    serializeJson(doc, jsonString);
    ESP_LOGD("JSON", "Sending JSON response");
    webSocket.sendTXT(0, jsonString);
}

void send_response(const char *payload)
{
    JsonDocument doc;
    doc["response"] = payload;
    send_json_respose(doc);
}

void send_response_ok()
{
    JsonDocument doc;
    doc["response"] = STATUS_TEXT_OK;
    send_json_respose(doc);
}

void send_response_error()
{
    JsonDocument doc;
    doc["response"] = STATUS_TEXT_ERROR;
    send_json_respose(doc);
}

void versionCommand(unsigned char unused1, unsigned char unused2)
{
    send_response(driver_version);
}

void statusCommand(unsigned char unused1, unsigned char unused2)
{
    detectActiveChannels();
    ESP_LOGD("SYSTEM", "200 Ok");
    ESP_LOGD("SYSTEM", "Driver version: %s", driver_version);
    ESP_LOGD("SYSTEM", "Board name: %s", board_name);
    ESP_LOGD("SYSTEM", "Board maker: %s", maker_name);
    ESP_LOGD("SYSTEM", "Hardware type: %s", hardware_type);
    ESP_LOGD("SYSTEM", "Max channels: %d", max_channels);
    ESP_LOGD("SYSTEM", "Number of active channels: %d", num_active_channels);

    JsonDocument doc;
    doc["driver_version"] = driver_version;
    doc["board_name"] = board_name;
    doc["maker_name"] = maker_name;
    doc["hardware_type"] = hardware_type;
    doc["max_channels"] = max_channels;
    doc["active_channels"] = num_active_channels;
    send_json_respose(doc);
}

void nopCommand(unsigned char unused1, unsigned char unused2)
{
    send_response_ok();
}

void microsCommand(unsigned char unused1, unsigned char unused2)
{
    unsigned long microseconds = micros();
    JsonDocument doc;
    doc["response"] = microseconds;
    send_json_respose(doc);
}

void serialNumberCommand(unsigned char unused1, unsigned char unused2)
{
    send_response(STATUS_TEXT_NOT_IMPLEMENTED);
}

void ledOnCommand(unsigned char unused1, unsigned char unused2)
{
    send_response(STATUS_TEXT_NOT_IMPLEMENTED);
}

void ledOffCommand(unsigned char unused1, unsigned char unused2)
{
    send_response(STATUS_TEXT_NOT_IMPLEMENTED);
}

void boardLedOnCommand(unsigned char unused1, unsigned char unused2)
{
    int state = adcRreg(ADS129x::GPIO);
    state = state & 0xF7;
    state = state | 0x80;
    adcWreg(ADS129x::GPIO, state);
    send_response_ok();
}

void boardLedOffCommand(unsigned char unused1, unsigned char unused2)
{
    int state = adcRreg(ADS129x::GPIO);
    state = state & 0x77;
    adcWreg(ADS129x::GPIO, state);
    send_response_ok();
}

void helpCommand(unsigned char unused1, unsigned char unused2)
{
    ESP_LOGD("HELP", "Available commands:");
    wsCommand.printCommands();
}

void readRegisterCommand(unsigned char register_number, unsigned char unused1)
{
    using namespace ADS129x;
    if (register_number >= 0 and register_number <= 255)
    {
        unsigned char result = adcRreg(register_number);
        JsonDocument doc;

        doc["response"] = result;
        send_json_respose(doc);
    }
    else
    {
        send_response_error();
    }
}

void writeRegisterCommand(unsigned char register_number, unsigned char register_value)
{
    if (register_number >= 0 && register_value >= 0)
    {
        adcWreg(register_number, register_value);
        send_response_ok();
    }
    else
    {
        send_response_error();
    }
}

void wakeupCommand(unsigned char unused1, unsigned char unused2)
{
    using namespace ADS129x;
    adcSendCommand(WAKEUP);
    send_response_ok();
}

void standbyCommand(unsigned char unused1, unsigned char unused2)
{
    using namespace ADS129x;
    adcSendCommand(STANDBY);
    send_response_ok();
}

void resetCommand(unsigned char unused1, unsigned char unused2)
{
    using namespace ADS129x;
    adcSendCommand(RESET);
    adsSetup();
    send_response_ok();
}

void startCommand(unsigned char unused1, unsigned char unused2)
{
    using namespace ADS129x;
    adcSendCommand(START);
    sample_number_union.sample_number = 0;
    send_response_ok();
}

void stopCommand(unsigned char unused1, unsigned char unused2)
{
    using namespace ADS129x;
    adcSendCommand(STOP);
    send_response_ok();
}

void rdatacCommand(unsigned char unused1, unsigned char unused2)
{
    using namespace ADS129x;
    detectActiveChannels();
    if (num_active_channels > 0)
    {
        is_rdatac = true;
        adcSendCommand(RDATAC);
        send_response_ok();
    }
    else
    {
        send_response(STATUS_TEXT_NO_ACTIVE_CHANNELS);
    }
}

void sdatacCommand(unsigned char unused1, unsigned char unused2)
{
    using namespace ADS129x;
    is_rdatac = false;
    current_buffer_index = 0;
    current_sample_index = 0;
    memset((void*)buffer_completed, 0, sizeof(buffer_completed));
    buffer_to_send = 0;
    adcSendCommand(SDATAC);
    using namespace ADS129x;
    send_response_ok();
}

void unrecognized(const char *command)
{
    ESP_LOGD("COMMAND", "Unrecognized command");
    send_response_error();
}

void detectActiveChannels()
{ // set device into RDATAC (continous) mode -it will stream data
    if ((is_rdatac) || (max_channels < 1))
        return; // we can not read registers when in RDATAC mode
    // Serial.println("Detect active channels: ");
    using namespace ADS129x;
    num_active_channels = 0;
    for (int i = 1; i <= max_channels; i++)
    {
        delayMicroseconds(1);
        int chSet = adcRreg(CHnSET + i);
        active_channels[i] = ((chSet & 7) != SHORTED);
        if ((chSet & 7) != SHORTED)
            num_active_channels++;
    }
}

uint8_t readData(uint8_t *data)
{
    uint8_t status[ADS_STATUS_SIZE];
    memset(status, 0, ADS_STATUS_SIZE);
    memset(data, 0, ADS_DATA_SIZE);
    digitalWrite(PIN_CS, LOW);
    spiRec(status, ADS_STATUS_SIZE);
    spiRec(data, ADS_DATA_SIZE);
    digitalWrite(PIN_CS, HIGH);
    return 0;
}

void IRAM_ATTR DRDY_ISR(void)
{
    // Confirm if device is in RDATAC mode
    if (!is_rdatac)
        return;
    // Check if the next buffer is available (not yet sent over WebSocket)
    if (buffer_completed[current_buffer_index])
    {
        // The current buffer is still full and not sent yet, we skip  this write to avoid overflow
        ESP_LOGD("ERROR", "Buffer Overflow detected at buffer %d", current_buffer_index);
        return;
    }
    // Get a pointer to the current position in the buffer
    uint8_t *buffer_ptr = &data_buffers[current_buffer_index][current_sample_index * BLOCK_SIZE];
    timestamp_union.timestamp = micros();
    // Add timestamp Bytes to data
    buffer_ptr[0] = timestamp_union.timestamp_bytes[0];
    buffer_ptr[1] = timestamp_union.timestamp_bytes[1];
    buffer_ptr[2] = timestamp_union.timestamp_bytes[2];
    buffer_ptr[3] = timestamp_union.timestamp_bytes[3];
    // Add counter Bytes to data
    buffer_ptr[4] = sample_number_union.sample_number_bytes[0];
    buffer_ptr[5] = sample_number_union.sample_number_bytes[1];
    buffer_ptr[6] = sample_number_union.sample_number_bytes[2];
    buffer_ptr[7] = sample_number_union.sample_number_bytes[3];

    readData(buffer_ptr + TIMESTAMP_SIZE_IN_BYTES + SAMPLE_NUMBER_SIZE_IN_BYTES);
    sample_number_union.sample_number++;

    // Update sample index and buffer management
    current_sample_index++;

    if (current_sample_index >= SAMPLES_PER_BUFFER)
    {
        // Mark the current buffer as completed
        buffer_completed[current_buffer_index] = true;

        // Move to the next buffer in a circular manner
        current_buffer_index = (current_buffer_index + 1) % NUM_BUFFERS;
        current_sample_index = 0; // Reset the sample index for the new buffer
    }
}

void adsSetup()
{ // default settings for ADS1298 and compatible chips
    using namespace ADS129x;
    // Send SDATAC Command (Stop Read Data Continuously mode)
    attachInterrupt(PIN_DRDY, DRDY_ISR, FALLING);
    adcSendCommand(SDATAC);
    delay(100);
    int val = adcRreg(ID);
    // For ADS21292R
    if(val == B01110011){
        hardware_type = "ADS1292R";
        ESP_LOGD("ADC", "ADS1292R detected");
        max_channels = 2;
    }
    switch (val & B00011111)
    {
    case B10000:
        hardware_type = "ADS1294";
        ESP_LOGD("ADC", "ADS1294 detected");
        max_channels = 4;
        break;
    case B10001:
        hardware_type = "ADS1296";
        ESP_LOGD("ADC", "ADS1296 detected");
        max_channels = 6;
        break;
    case B10010:
        hardware_type = "ADS1298";
        ESP_LOGD("ADC", "ADS1298 detected");
        max_channels = 8;
        break;
    case B11110:
        hardware_type = "ADS1299";
        ESP_LOGD("ADC", "ADS1299 detected");
        max_channels = 8;
        break;
    case B11100:
        hardware_type = "ADS1299-4";
        ESP_LOGD("ADC", "ADS1299-4 detected");
        max_channels = 4;
        break;
    case B11101:
        hardware_type = "ADS1299-6";
        ESP_LOGD("ADC", "ADS1299-6 detected");
        max_channels = 6;
        break;
    default:
        max_channels = 0;
    }

    // All GPIO set to output 0x0000: (floating CMOS inputs can flicker on and off, creating noise)
    adcWreg(ADS129x::GPIO, 0);
    adcWreg(CONFIG3, PD_REFBUF | CONFIG3_const);
    adcSendCommand(ADS129x::START);
}

void espSetup()
{
    using namespace ADS129x;
    // prepare pins to be outputs or inputs
    // pinMode(PIN_SCLK, OUTPUT); //optional - SPI library will do this for us
    // pinMode(PIN_DIN, OUTPUT); //optional - SPI library will do this for us
    // pinMode(PIN_DOUT, INPUT); //optional - SPI library will do this for us
    pinMode(PIN_CS, OUTPUT);
    pinMode(PIN_DRDY, INPUT);
    pinMode(PIN_RST, OUTPUT); // *optional
    // start Serial Peripheral Interface
    spiBegin(PIN_CS);
    spiInit(MSBFIRST, SPI_MODE1, SPI_CLK);
    delay(10); // wait for oscillator to wake up
    digitalWrite(PIN_RST, HIGH);
    delay(1000);
    digitalWrite(PIN_RST, LOW);
    delay(1);
    digitalWrite(PIN_RST, HIGH);
    delay(1);
}
/**
 * Updated by Deepak Khatri <Origin Interconnect Pvt Ltd - oric.io>
 * For ADS1299 + ESP32-C3 wireless EEG project
 * @file ADS1299.hh
 * @author Markus Bäcker (markus.baecker@ovgu.de)
 * @brief Driver class for TI ADS1299 in combination with ESP32-S1
 * @version 0.1
 * @date 2022-02-21
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#ifndef ____ADS1299__
#define ____ADS1299__

#include <Arduino.h>
#include <SPI.h>
#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"

struct results {
  uint8_t status[ADS_STATUS_SIZE];
  uint8_t data[ADS_DATA_SIZE];
};

class ADS1299 {
public:
  //Attributes
  int DRDY; 
  int CS;  //pin numbers for "Data Ready" (DRDY) and "Chip Select" CS (Datasheet, pg. 26)
  struct results res; // contains Data
  
  ADS1299();
  // SETUP for devices
  void setup(int _DRDY, int _CS);
  //ADS1299 SPI Command Definitions (Datasheet, Pg. 35)
  //System Commands
  void WAKEUP();
  void STANDBY();
  void RESET();
  void START();
  void STOP();

  //Data Read Commands
  void RDATAC();  // Reads data from ADC Channels continious
  void SDATAC();  // Stops continous reading
  void RDATA();   // Read one

  //Register Read/Write Commands

  byte getDeviceID();
  byte RREG(byte _address);
  void printRegisterName(byte _address);
  void WREG(byte _address, byte _value);                              //
  void WREG(byte _address, byte _value, byte _numRegistersMinusOne);  //

  
  

  //SPI Library 
  byte transfer(byte _data);

  //------------------------//
  void activateTestSignals(byte _channeladdress); 
  float convertHEXtoVolt(long hexdata);  //convert Data Bytes to float Voltage values
  float* updateData();
  uint8_t readData(uint8_t *status, uint8_t *data);
  
  void attachInterrupt();
  void detachInterrupt();  // Default
 
  
  void calculateLSB(uint8_t gain, float vref);
  void setSingleended(int configvalue = ADS1299_PGA_GAIN01);
  
  //------------------------//
  void TI_setup(); // for Debugging purpose
  void Task_data(void * argument); //const
  TaskHandle_t				task_handle;

private:
  
  
  static void Task_read_DRDY(){}; // Task called by RTOS 
  byte regData [24];	// array is used to mirror register data
  
};

#endif

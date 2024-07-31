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

#ifndef ____ADS1299__
#define ____ADS1299__

#include <Arduino.h>
#include <SPI.h>
#include "ADS1299config.h"
#include "ESPC3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"

class ADS1299 {
public:
  int DRDY; 
  int CS;
  
  ADS1299();
  void setup(int _DRDY, int _CS);
  void WAKEUP();
  void STANDBY();
  void RESET();
  void START();
  void STOP();
  void RDATAC();
  void SDATAC();
  void RDATA();
  byte getDeviceID();
  byte RREG(byte _address);
  void WREG(byte _address, byte _value);
  void activateTestSignals(byte _channeladdress); 
  uint8_t readData(uint8_t *status, uint8_t *data);
  void setSingleended( int max_channels, int configvalue = GAIN_01);
private:
  byte regData [24];
};

#endif

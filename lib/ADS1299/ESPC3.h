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

#ifndef _ESPC3_H_
#define _ESPC3_H_

#define PIN_CS 7 // active low
#define PIN_MISO 10
#define PIN_MOSI 3
#define PIN_SCLK 2

#define PIN_RST 1 // active lOW
#define PIN_DRDY 0 // active lOW

#define PIXEL_BRIGHTNESS 7
#define TRIGGER_PIN 9   // This pin will be checked for to go in OTA mode

#define spiClk 2000000 //2Mhz  // 1.5Mhz to 2.4Mhz according to ADS1299 datasheet page 12
#define TCLK_cycle 0.5 // 1/2Mhz = 0.5us

#endif

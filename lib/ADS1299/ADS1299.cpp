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

#include "ADS1299.h"

/**
 * @brief Construct a new ADS1299::ADS1299 object
 *
 */
ADS1299::ADS1299()
{
  pinMode(SCK, OUTPUT);
  pinMode(MOSI, OUTPUT);
  pinMode(MISO, INPUT);
}

/**
 * @brief Sets up master ADS1299, configures SPI and ADS1299 for
 * @details supply slave ADS1299 with clock and BIAS
 * @param _DRDY DATA Ready pin goes LOW, when DATA is available
 * @param _CS  CHIP SELECT pin, goes LOW, when MCU communicates with ADS
 */
void ADS1299::setup(int _DRDY, int _CS)
{
  CS = _CS;
  DRDY = _DRDY;
  pinMode(CS, OUTPUT);
  pinMode(PIN_RST, OUTPUT);
  pinMode(DRDY, INPUT);


  digitalWrite(SCK, LOW);
  digitalWrite(MOSI, LOW);

  SPI.begin(SCK, MISO, MOSI, CS);
  SPI.setBitOrder(MSBFIRST);
  SPI.setFrequency(spiClk);
  SPI.setDataMode(SPI_MODE1);

  delay(50);

  digitalWrite(PIN_RST, LOW);
  delayMicroseconds(4);
  digitalWrite(PIN_RST, HIGH);
  delayMicroseconds(20);

  RESET();
  delayMicroseconds(20 * TCLK_cycle);
  SDATAC();
  
  WREG(CONFIG1, 0x96);
  WREG(CONFIG2, 0xC0);
  WREG(CONFIG3, 0xEC);
  WREG(MISC1, 0x00);
}

/**
 * @brief Wakes up the ADC
 *
 */
void ADS1299::WAKEUP()
{
  digitalWrite(CS, LOW);
  SPI.transfer(OPCODE_WAKEUP);
  delayMicroseconds(4 * TCLK_cycle);
  digitalWrite(CS, HIGH);
}
/**
 * @brief Put the ADC in Standby
 *  5.1 mW instead of 22 mW in running mode
 */
void ADS1299::STANDBY()
{
  digitalWrite(CS, LOW);
  SPI.transfer(OPCODE_STANDBY);
  delayMicroseconds(4 * TCLK_cycle);
  digitalWrite(CS, HIGH);
}

/**
 * Reset all Registers
 * sends command 0x06
 * @return nothing
 */
void ADS1299::RESET()
{
  digitalWrite(CS, LOW);
  delayMicroseconds(TCLK_cycle);
  SPI.transfer(OPCODE_RESET);
  delayMicroseconds(18 * TCLK_cycle);
  digitalWrite(CS, HIGH);
}
/**
 * @brief Send SPI command or pull START pin LOW, sync multiple ADS
 *
 */
void ADS1299::START()
{
  digitalWrite(CS, LOW);
  delayMicroseconds(TCLK_cycle);
  SPI.transfer(OPCODE_START);
  delayMicroseconds(4 * TCLK_cycle);
  digitalWrite(CS, HIGH);
}
/**
 * @brief STOP data conversion, allows REGISTER reading/writing
 *
 */
void ADS1299::STOP()
{
  digitalWrite(CS, LOW);
  delayMicroseconds(TCLK_cycle);
  SPI.transfer(OPCODE_STOP);
  delayMicroseconds(4 * TCLK_cycle);
  digitalWrite(CS, HIGH);
}
/**
 * @brief Read one data chunk from ADS. Transmits 0001 0010 (12h)
 */
void ADS1299::RDATA()
{
  digitalWrite(CS, LOW);
  delayMicroseconds(TCLK_cycle);
  SPI.transfer(OPCODE_RDATA);
  delayMicroseconds(4 * TCLK_cycle);
  digitalWrite(CS, HIGH);
}

/**
 * @brief Starts ADC conversion in Continuous mode
 */
void ADS1299::RDATAC()
{
  digitalWrite(CS, LOW);
  delayMicroseconds(TCLK_cycle);
  SPI.transfer(OPCODE_RDATAC);
  delayMicroseconds(4 * TCLK_cycle);
  digitalWrite(CS, HIGH);
}

/**
 * @brief Stops ADC conversion
 */
void ADS1299::SDATAC()
{
  digitalWrite(CS, LOW);
  delayMicroseconds(TCLK_cycle);
  SPI.transfer(OPCODE_SDATAC);
  delayMicroseconds(4 * TCLK_cycle);

  digitalWrite(CS, HIGH);
}

/**
 * @brief Read Register from ADS1299.
 * It answers with the contained values, call SDATAC before reading/writing registers
 * @param _address Single Register adress to read
 */
byte ADS1299::RREG(byte _address)
{
  byte opcode1 = OPCODE_RREG + _address;
  digitalWrite(CS, LOW);
  delayMicroseconds(TCLK_cycle);
  SPI.transfer(opcode1);
  SPI.transfer(0x00);
  regData[_address] = SPI.transfer(0x00);
  delayMicroseconds(4 * TCLK_cycle);
  digitalWrite(CS, HIGH);
  return regData[_address];
}

/**
 * @brief Write Settings as 1 Byte of Data to Registers
 *
 * @param _address of Regsiter
 * @param _value of Register
 */
void ADS1299::WREG(byte _address, byte _value)
{
  digitalWrite(CS, LOW);
  delayMicroseconds(TCLK_cycle);
  uint8_t opcode1 = OPCODE_WREG + _address;
  SPI.transfer(opcode1);
  SPI.transfer(0x00);
  SPI.transfer(_value);
  delayMicroseconds(4 * TCLK_cycle);
  digitalWrite(CS, HIGH);
}

/**
 * @brief Ask ADS1299 for device ID;
 * @details  returns REV_ID[2:0] 1 DEV_ID[1:0] NU_CH[1:0]
 * @result  desired answer is 0b 0011 1110
 * @return byte ID value
 */
byte ADS1299::getDeviceID()
{
  SDATAC();
  byte data = RREG(ID);
  RDATAC();
  return data;
}

/**
 * @brief  activate testsignal on passed channel; changes CONFIG1/2/3 !
 *
 * @param _channeladdress
 */

void ADS1299::activateTestSignals(byte _channeladdress)
{
  SDATAC();
  WREG(CONFIG3, 0xEC);
  WREG(CONFIG2, 0xD0);
  WREG(_channeladdress, 0x05);
  RDATAC();
}

/**
 * @brief setup device for singleended measurement against SRB1
 *
 */

void ADS1299::setSingleended(int max_channels, int configvalue)
{
  SDATAC();
  for (int i = 0; i < max_channels; i++)
  {
    WREG(CH1SET + i, configvalue);
  }
  RDATAC();
}

uint8_t ADS1299::readData(uint8_t *status, uint8_t *data)
{
  memset(status, 0, ADS_STATUS_SIZE);
  memset(data, 0, ADS_DATA_SIZE);
  digitalWrite(CS, LOW);
  delayMicroseconds( TCLK_cycle);

  SPI.transfer(status, ADS_STATUS_SIZE);
  SPI.transfer(data, ADS_DATA_SIZE);
  delayMicroseconds(4 * TCLK_cycle);
  digitalWrite(CS, HIGH);
  return 0;
}

 
/**
  Updated by Deepak Khatri <Origin Interconnect Pvt Ltd - oric.io>
  For ADS1299 + ESP32-C3 wireless EEG project
 * @file ADS1299.cpp
 * @author Markus Bäcker (markus.baecker@ovgu.de)
 * @brief Class for interfacing ADS1299
 * @version 0.1
 * @date 2022-03-02
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "ADS1299.h"

/**
 * @brief Construct a new ADS1299::ADS1299 object
 *
 */
ADS1299::ADS1299() {
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
void ADS1299::setup(int _DRDY, int _CS) {
  CS = _CS;
  DRDY = _DRDY;
  pinMode(CS, OUTPUT);
  pinMode(PIN_RST, OUTPUT);
  pinMode(DRDY, INPUT);
  // set up slave select pins as outputs as the Arduino API
  // doesn't handle automatically pulling SS low
  
  digitalWrite(SCK, LOW);
  digitalWrite(MOSI, LOW);
  //digitalWrite(SS, HIGH);
  // SPI Setup
  SPI.begin(SCK, MISO, MOSI, CS); // Initialize SPI library
  SPI.setBitOrder(MSBFIRST);  // Most significant Bit first
  SPI.setFrequency(spiClk);   // Sets SPI clock
  SPI.setDataMode(SPI_MODE1); //// 1...2.4 MHz, clock polarity = 0; clock phase = 1 (pg. 8)

  delay(50);
  // at startup all inputs must be low
  digitalWrite(PIN_RST, LOW);
  delayMicroseconds(4); // wait for oscillator startup 20us
  digitalWrite(PIN_RST, HIGH);
  delayMicroseconds(20);

  delay(40); // Recommended 18 Tclk before using device
  RESET();                           // DEVICE wakes up in RDATAC so no Registers could be written.
  delay(10); // Recommended 18 Tclk before using device

  delayMicroseconds(20 * TCLK_cycle); // Recommended 18 Tclk before using device
  SDATAC();                           // DEVICE wakes up in RDATAC so no Registers could be written.
  delayMicroseconds(20 * TCLK_cycle); // Recommended 18 Tclk before using device
  // Setup Registers for master ADS
  // CLOCK: CLKSEL pin 1 (through R1); COnf1 1111 0xxx
  //WREG(CONFIG1, 0xF5); //F5 Output CLK signal for second ADS, 500 SPS
  // BIAS
  //WREG(MISC1, 0x20);      // connect SRB1 to neg Electrodes
  //WREG(CONFIG3, 0xEC);    // b’x1xx 1100  Turn on BIAS amplifier, set internal BIASREF voltage
  //WREG(BIAS_SENSN, 0x01); // CH1 - bias sensing-> all REFELEC
  //WREG(BIAS_SENSP, 0x01); // CH1 + bias sensing
  // Give slave time to react to external CLK
  delay(1);
   WREG(CONFIG1, 0x96); // 250sps
  // Below rates are not usable with ESP32-C3
//  WREG(CONFIG1, 0x95); // 500sps
  // WREG(CONFIG1, 0x94); // 1000sps
  // WREG(CONFIG1, 0x93); // 2000sps
  WREG(CONFIG2, 0xC0);
  delay(1);
  WREG(CONFIG3, 0xEC);
  delay(1);
  WREG(MISC1, 0x20);
  delay(1);
  // ADS setup as per EVM
  delayMicroseconds(20 * TCLK_cycle); // Recommended 18 Tclk before using device
  START();
}

// ADS1299 SPI Command Definitions (Datasheet, Pg. 35)
/*--------------------------------------------------*/
/*---------------------- System Commands ---------------------*/
/*--------------------------------------------------*/

/**
 * @brief Wakes up the ADC
 *
 */
void ADS1299::WAKEUP() {
  digitalWrite(CS, LOW);
  SPI.transfer(_WAKEUP);
  digitalWrite(CS, HIGH);
  delayMicroseconds(4 * TCLK_cycle); // wait 4 Tclk (Pg. 35)
}
/**
 * @brief Put the ADC in Standby
 *  5.1 mW instead of 22 mW in running mode
 */
void ADS1299::STANDBY() {
  digitalWrite(CS, LOW);
  SPI.transfer(_STANDBY);
  digitalWrite(CS, HIGH);
}

/**
 * Reset all Registers
 * sends command 0x06
 * @return nothing
 */
void ADS1299::RESET() {
  digitalWrite(CS, LOW);
  SPI.transfer(_RESET);
  digitalWrite(CS, HIGH);
  delayMicroseconds(18 * TCLK_cycle); // Recommended 18 Tclk before using device
}
/**
 * @brief Send SPI command or pull START pin LOW, sync multiple ADS
 *
 */
void ADS1299::START() {
  digitalWrite(CS, LOW);
  //SPI.transfer(_START);
  SPI.transfer(_START);
  delayMicroseconds(4 * TCLK_cycle);
  digitalWrite(CS, HIGH);
}
/**
 * @brief STOP data conversion, allows REGISTER reading/writing
 *
 */
void ADS1299::STOP() {
  digitalWrite(CS, LOW);
  SPI.transfer(_STOP);
  delayMicroseconds(4 * TCLK_cycle); // wait 4 clk cycles after this command (DS pg.36)
  digitalWrite(CS, HIGH);
}
/**
 * @brief Read one data chunk from ADS. Transmits 0001 0010 (12h)
 */
void ADS1299::RDATA() {
  digitalWrite(CS, LOW);
  SPI.transfer(_RDATA);
  digitalWrite(CS, HIGH);
}

/**
 * @brief Read Data Continuously mode. Transmits 0001 0000 (10h)
 * Reads data from ADC Channels continious
 * @return nothing
 */
void ADS1299::RDATAC() {
  digitalWrite(CS, LOW);
  SPI.transfer(_RDATAC);
  digitalWrite(CS, HIGH);
}

/**
 * @brief Stop Read Data Continuously mode. Transmits 0001 0001 (11h)
 * @return nothing
 */
void ADS1299::SDATAC() {
  digitalWrite(CS, LOW);
  SPI.transfer(_SDATAC);
  digitalWrite(CS, HIGH);
  delayMicroseconds(4 * TCLK_cycle); // wait 4 clk cycles after this command (DS pg.36)
}

/*--------------------------------------------------*/
/*---------------------- Register Commands ---------------------*/
/*--------------------------------------------------*/

/**
 * @brief Read Register from ADS1299.
 * It answers with the contained values, call SDATAC before reading/writing registers
 * @param _address Single Register adress to read
 */
byte ADS1299::RREG(byte _address) {  
  SDATAC();                               //  RDATAC must be stopped before reading 
  byte opcode1 = _address + 0x20;         //  RREG expects 001rrrrr where rrrrr = _address
  digitalWrite(CS, LOW);                  //  open SPI
  SPI.transfer(opcode1);                  //  opcode1
  SPI.transfer(0x00);                     //  opcode2
  regData[_address] = SPI.transfer(0x00); //  update mirror location with returned byte
  digitalWrite(CS, HIGH);                 //  close SPI
  RDATAC();                                // Continue reading
  return regData[_address];               // return requested register value
}

/**
 * @brief Write Settings as 1 Byte of Data to Registers
 *
 * @param _address of Regsiter
 * @param _value of Register
 */
void ADS1299::WREG(byte _address, byte _value) {
  digitalWrite(CS,LOW);
  // write one Register
  uint8_t opcode1 = _address + 0x40;
  // Send WREG command & address
  SPI.transfer(opcode1);
  // Send number of registers to write
  SPI.transfer(0x00);
  // Write the value to the register
  SPI.transfer(_value);
  digitalWrite(CS, HIGH);
  // a 4 t_CLK (~2 us) period must separate the end of one byte (or command) and the next
} //


/**
 * @brief Ask ADS1299 for device ID;
 * @details  returns REV_ID[2:0] 1 DEV_ID[1:0] NU_CH[1:0]
 * @result  desired answer is 0b 0011 1110
 * @return byte ID value
 */
byte ADS1299::getDeviceID() {
  digitalWrite(CS, LOW); // Low to communicated

  SPI.transfer(_SDATAC);          // 0001 0001 SDATAC Stop Data continous
  SPI.transfer(_RREG);            // 0010 0000 RREG Read Register
  SPI.transfer(0x00);             // 0000 0000 Asking for 1 byte ID
  byte data = SPI.transfer(0x00); // byte to read (hopefully 0b???1 1110) should be 3E or 62
  // Device ID you should get:                    REV_ID[2:0] 1 DEV_ID[1:0] NU_CH[1:0]
  digitalWrite(CS, HIGH); // HIGH to stop communication
  return data;
}

/**
 * @brief  activate testsignal on passed channel; changes CONFIG1/2/3 !
 *
 * @param _channeladdress
 */

void ADS1299::activateTestSignals(byte _channeladdress) {
  SDATAC();            // 0001 0001 Stop Data reading, to write new settings
  WREG(CONFIG3, 0xEC); // internal Reference Voltage 1110 0000 no bias
  //WREG(CONFIG1, 0xD6); // 0x96= 1001 0110 Daisy En 250 SPS |
  WREG(CONFIG2, 0xD0); // Config2: 0100(x40)0010(x02)->x00-> 11010000(D0) internal test signal
  // 0xD5 for faster higher test signal
  WREG(_channeladdress, 0x05); // CHnSET: 0100 0101 Setting: 00000101 no PGA, just activate a (1 mV x V REF / 2.4) Square-Wave Test Signal
  // RDATAC();             // read data continous
}

/**
 * @brief setup device for singleended measurement against SRB1
 *
 */

void ADS1299::setSingleended(int configvalue) { 
  SDATAC();           
  // GAIN 1, normal electrode input -> 0x00
  WREG(CH1SET, configvalue); // measures normal on CH1
  WREG(CH2SET, configvalue); // measures normal on CH1
  WREG(CH3SET, configvalue); // measures normal on CH1
  WREG(CH4SET, configvalue); // measures normal on CH1
  WREG(CH5SET, configvalue); // measures normal on CH1
  WREG(CH6SET, configvalue); // measures normal on CH1
  WREG(CH7SET, configvalue); // measures normal on CH1
  WREG(CH8SET, configvalue);
  RDATAC();

}

uint8_t ADS1299::readData(uint8_t *status, uint8_t *data) {
  // portDISABLE_INTERRUPTS();
  // Get ADS status
  // SDATAC();
  digitalWrite(CS, LOW);
  delayMicroseconds(7 * TCLK_cycle);
  
  memset(status, 0, ADS_STATUS_SIZE);
  SPI.transfer(status, ADS_STATUS_SIZE);
  // res.status[0] = SPI.transfer(0x00);
  // res.status[1] = SPI.transfer(0x00);
  // res.status[2] = SPI.transfer(0x00);

  // Get ADS Data
  memset(data, 0, ADS_DATA_SIZE);
  SPI.transfer(data, ADS_DATA_SIZE);
  // for (int i = 0; i < 7; i++) {
  //   int offset = i*3;
  //   res.data[offset + 0] = SPI.transfer(0x00);
  //   res.data[offset + 1] = SPI.transfer(0x00);
  //   res.data[offset + 2] = SPI.transfer(0x00);
  // }
  // portENABLE_INTERRUPTS();
  // RDATAC();
  digitalWrite(CS,HIGH);
  return 0;
} 

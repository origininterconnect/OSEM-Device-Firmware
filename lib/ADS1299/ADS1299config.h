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

#ifndef _ADS1299config_h
#define _ADS1299config_h


#define CHANNELS 8 
#define ADS_DATA_SIZE CHANNELS * 3 
#define ADS_STATUS_SIZE 3 
#define BLOCK_SIZE 32

/*********************************************//**
*   @name Command Definitions
*	|	Bit 7	|	Bit 6	|	Bit 5	|	Bit 4	|	Bit 3	|	Bit 2	|	Bit 1	|	Bit 0	|
*	-------------------------------------------------------------------------------------------------
*	|			                                Command                                 			|
*	-------------------------------------------------------------------------------------------------
*
*/
#define OPCODE_WAKEUP       0x02
#define OPCODE_STANDBY      0x04
#define OPCODE_RESET        0x06
#define OPCODE_START        0x08
#define OPCODE_STOP         0x0A
#define OPCODE_RDATAC       0x10
#define OPCODE_SDATAC       0x11
#define OPCODE_RDATA        0x12
#define OPCODE_RREG         0x20
#define OPCODE_WREG         0x40

/*****************************************//**
 * @name Register Addresses
 * 
 */

/***********************************************************************************************//**
* 	ADS1299 - ID - 0x00
*	|	Bit 7	|	Bit 6	|	Bit 5	|	Bit 4	|	Bit 3	|	Bit 2	|	Bit 1	|	Bit 0	|
*	-------------------------------------------------------------------------------------------------
*	|					Reserverd					|		Device ID		|		Num-Channels	|
*	-------------------------------------------------------------------------------------------------
*
*/
#define ID 0x00
#define ID_ADS1299_4CH 0x08  // 0b1100 4channels
#define ID_ADS1299_6CH 0x0A  // 0b1101 6channels
#define ID_ADS1299_8CH 0x0E  // 0b1110 8channels


/***********************************************************************************************//**
* 	ADS1299 - CONFIG1 - 0x01
*	|	Bit 7	|	Bit 6	|	Bit 5	|	Bit 4	|	Bit 3	|	Bit 2	|	Bit 1	|	Bit 0	|
*	-------------------------------------------------------------------------------------------------
*	| Reserverd	| DAISY_EN	|   CLK_EN  |		 Reserverd		|			    DR[2:0]				|
*	-------------------------------------------------------------------------------------------------
*
*/
#define CONFIG1 0x01

#define CFG1RES7                                0x80    // 10000000
#define DAISY_EN                                0x40    // 01000000
#define CLK_EN                                  0x20    // 00100000
#define CFG1RES43                               0x10    // 00010000
#define SAMPLING_RATE_00250HZ                   0x06    // 00000110
#define SAMPLING_RATE_00500HZ                   0x05    // 00000101
#define SAMPLING_RATE_01000HZ                   0x04    // 00000100
#define SAMPLING_RATE_02000HZ                   0x03    // 00000011
#define SAMPLING_RATE_04000HZ                   0x02    // 00000010
#define SAMPLING_RATE_08000HZ                   0x01    // 00000001
#define SAMPLING_RATE_16000HZ                   0x00    // 00000000

/***********************************************************************************************//**
* 	ADS1299 - CONFIG2 - 0x02
*	|	Bit 7	|	Bit 6	|	Bit 5	|	Bit 4	|	Bit 3	|	Bit 2	|	Bit 1	|	Bit 0	|
*	-------------------------------------------------------------------------------------------------
*	|			    Reserverd			|  INT_CAL	|  Reserved	|  CAL_AMP	|      CAL_FREQ[1:0]    |
*	-------------------------------------------------------------------------------------------------
*
*/
#define CONFIG2 0x02

#define CFG2RES75                               0xC0   // 11000000
#define INT_CAL                                 0x10   // 00010000
#define CFG2RES3                                0x00   // 00000000
#define CAL_AMP                                 0x04   // 00000100
#define CAL_FREQ_221                            0x00   // 00000000
#define CAL_FREQ_220                            0x01   // 00000001
#define CAL_FREQ_DC                             0x03   // 00000011

/***********************************************************************************************//**
* 	ADS1299 - CONFIG3 - 0x03
*	|	Bit 7	|	Bit 6	|	Bit 5	|	Bit 4	|	Bit 3	|	Bit 2	|	Bit 1	|	Bit 0	|
*	-------------------------------------------------------------------------------------------------
*	| PD_REFBUF |          Reserved		| BIAS_MEAS |BIASREF_INT|  PD_BIAS  |B_LOFF_SENS| BIAS_STAT	|
*	-------------------------------------------------------------------------------------------------
*
*/
#define CONFIG3 0x03

#define PD_REFBUF                               0x80 // 10000000
#define CFG3RES_65                              0x42 // 01100000
#define BIAS_MEAS                               0x00 // 00010000   
#define BIASREF_INT                             0x08 // 00001000
#define PD_BIAS                                 0x04 // 00000100
#define BIAS_LOFF_SENS                          0x00 // 00000000

/***********************************************************************************************//**
* 	ADS1299 - LOFF - 0x04
*	|	Bit 7	|	Bit 6	|	Bit 5	|	Bit 4	|	Bit 3	|	Bit 2	|	Bit 1	|	Bit 0	|
*	-------------------------------------------------------------------------------------------------
*	|			COMP_TH[7:5]    		| Reserverd |	ILEAD_OFF[3:2]		|	FLEAD_OFF[1:0]		|
*	-------------------------------------------------------------------------------------------------
*
*/
#define LOFF 0x04

#define COMP_TH_95                              0x00 // 00000000
#define COMP_TH_92                              0x20 // 00100000
#define COMP_TH_90                              0x40 // 01000000
#define COMP_TH_87                              0x60 // 01100000
#define COMP_TH_85                              0x80 // 10000000
#define COMP_TH_80                              0xA0 // 10100000
#define COMP_TH_75                              0xC0 // 11000000
#define COMP_TH_70                              0xE0 // 11100000
#define LOFF_RES                                0x10 // 00010000
#define ILEAD_OFF_6nA                           0x00 // 00000000
#define ILEAD_OFF_24nA                          0x04 // 00000100
#define ILEAD_OFF_6uA                           0x08 // 00001000
#define ILEAD_OFF_24uA                          0x0C // 00001100
#define FLEAD_OFF_DC                            0x00 // 00000000
#define FLEAD_OFF_AC_7_8_hz                     0x01 // 00000001
#define FLEAD_OFF_AC_31_2_hz                    0x02 // 00000010
#define FLEAD_OFF_AC_fDR_2_hz                     0x03 // 00000011

/***********************************************************************************************//**
* 	ADS1299 - CHnSET - 0x05 to 0x0C
*	|	Bit 7	|	Bit 6	|	Bit 5	|	Bit 4	|	Bit 3	|	Bit 2	|	Bit 1	|	Bit 0	|
*	-------------------------------------------------------------------------------------------------
*	|	PDn 	|			GAINn[2:0]	    		|	SRB2	|			MUXn[2:0]   			|
*	-------------------------------------------------------------------------------------------------
*
*/
#define CH1SET 0x05
#define CH2SET 0x06
#define CH3SET 0x07
#define CH4SET 0x08
#define CH5SET 0x09
#define CH6SET 0x0A
#define CH7SET 0x0B
#define CH8SET 0x0C

#define CHANNEL_POWER_DOWN                      0x80 // 10000000
#define GAIN_01                                 0x00 // 00000000
#define GAIN_02                                 0x10 // 00010000
#define GAIN_04                                 0x20 // 00100000
#define GAIN_06                                 0x30 // 00110000
#define GAIN_08                                 0x40 // 01000000
#define GAIN_12                                 0x50 // 01010000
#define GAIN_24                                 0x60 // 01100000
#define SRB2                                    0x08 // 00001000
#define MUX_NORMAL                              0x00 // 00000000
#define MUX_SHORTED                             0x01 // 00000001
#define MUX_MEAS_BIAS                           0x02 // 00000010
#define MUX_SUPPLY                              0x03 // 00000011
#define MUX_TEMP                                0x04 // 00000100
#define MUX_TESTSIG                             0x05 // 00000101
#define MUX_BIAS_DRP                            0x06 // 00000110
#define MUX_BIAS_DRN                            0x07 // 00000111

/***********************************************************************************************//**
* 	ADS1299 - BIAS_SENSP - 0x0D
*	|	Bit 7	|	Bit 6	|	Bit 5	|	Bit 4	|	Bit 3	|	Bit 2	|	Bit 1	|	Bit 0	|
*	-------------------------------------------------------------------------------------------------
*	|	BIASP8  |	BIASP7  |	BIASP6  |	BIASP5  |	BIASP4  |	BIASP3  |	BIASP2  |   BIASP1  |
*	-------------------------------------------------------------------------------------------------
*
*/
#define BIAS_SENSP 0x0D

#define BIASP8                                  0x80 // 10000000
#define BIASP7                                  0x40 // 01000000
#define BIASP6                                  0x20 // 00100000
#define BIASP5                                  0x10 // 00010000
#define BIASP4                                  0x08 // 00001000
#define BIASP3                                  0x04 // 00000100
#define BIASP2                                  0x02 // 00000010
#define BIASP1                                  0x01 // 00000001

/***********************************************************************************************//**
* 	ADS1299 - BIAS_SENSN - 0x0E
*	|	Bit 7	|	Bit 6	|	Bit 5	|	Bit 4	|	Bit 3	|	Bit 2	|	Bit 1	|	Bit 0	|
*	-------------------------------------------------------------------------------------------------
*	|	BIASN8  |	BIASN7  |	BIASN6  |	BIASN5  |	BIASN4  |	BIASN3  |	BIASN2  |   BIASN1  |
*	-------------------------------------------------------------------------------------------------
*
*/
#define BIAS_SENSN 0x0E

#define BIASN8                                  0x80 // 10000000
#define BIASN7                                  0x40 // 01000000
#define BIASN6                                  0x20 // 00100000
#define BIASN5                                  0x10 // 00010000
#define BIASN4                                  0x08 // 00001000
#define BIASN3                                  0x04 // 00000100
#define BIASN2                                  0x02 // 00000010
#define BIASN1                                  0x01 // 00000001

/***********************************************************************************************//**
* 	ADS1299 - LOFF_SENSP - 0x0F
*	|	Bit 7	|	Bit 6	|	Bit 5	|	Bit 4	|	Bit 3	|	Bit 2	|	Bit 1	|	Bit 0	|
*	-------------------------------------------------------------------------------------------------
*	|	LOFFP8	|	LOFFP7	|	LOFFP6	|	LOFFP5	|	LOFFP4	|	LOFFP3	|	LOFFP2	|   LOFFP1  |
*	-------------------------------------------------------------------------------------------------
*
*/
#define LOFF_SENSP 0x0F

#define LOFFP8                                  0x80 // 10000000
#define LOFFP7                                  0x40 // 01000000
#define LOFFP6                                  0x20 // 00100000
#define LOFFP5                                  0x10 // 00010000
#define LOFFP4                                  0x08 // 00001000
#define LOFFP3                                  0x04 // 00000100
#define LOFFP2                                  0x02 // 00000010
#define LOFFP1                                  0x01 // 00000001

/***********************************************************************************************//**
* 	ADS1299 - LOFF_SENSN - 0x0F
*	|	Bit 7	|	Bit 6	|	Bit 5	|	Bit 4	|	Bit 3	|	Bit 2	|	Bit 1	|	Bit 0	|
*	-------------------------------------------------------------------------------------------------
*	|	LOFFN8	|	LOFFN7	|	LOFFN6	|	LOFFN5	|	LOFFN4	|	LOFFN3	|	LOFFN2	|   LOFFN1  |
*	-------------------------------------------------------------------------------------------------
*
*/
#define LOFF_SENSN 0x10

#define LOFFN8                                  0x80 // 10000000
#define LOFFN7                                  0x40 // 01000000
#define LOFFN6                                  0x20 // 00100000
#define LOFFN5                                  0x10 // 00010000
#define LOFFN4                                  0x08 // 00001000
#define LOFFN3                                  0x04 // 00000100
#define LOFFN2                                  0x02 // 00000010
#define LOFFN1                                  0x01 // 00000001

/***********************************************************************************************//**
* 	ADS1299 - LOFF_FLIP - 0x11
*	|	Bit 7	|	Bit 6	|	Bit 5	|	Bit 4	|	Bit 3	|	Bit 2	|	Bit 1	|	Bit 0	|
*	-------------------------------------------------------------------------------------------------
*	| LOFF_FLIP8| LOFF_FLIP7| LOFF_FLIP6| LOFF_FLIP5| LOFF_FLIP4| LOFF_FLIP3| LOFF_FLIP2| LOFF_FLIP1|
*	-------------------------------------------------------------------------------------------------
*
*/
#define LOFF_FLIP 0x11

#define LOFF_FLIP8                              0x80 // 10000000
#define LOFF_FLIP7                              0x40 // 01000000
#define LOFF_FLIP6                              0x20 // 00100000
#define LOFF_FLIP5                              0x10 // 00010000
#define LOFF_FLIP4                              0x08 // 00001000
#define LOFF_FLIP3                              0x04 // 00000100
#define LOFF_FLIP2                              0x02 // 00000010
#define LOFF_FLIP1                              0x01 // 00000001

/***********************************************************************************************//**
* 	ADS1299 - LOFF_STATP - 0x12
*	|	Bit 7	|	Bit 6	|	Bit 5	|	Bit 4	|	Bit 3	|	Bit 2	|	Bit 1	|	Bit 0	|
*	-------------------------------------------------------------------------------------------------
*	|  IN8P_OFF	|  IN7P_OFF	|  IN6P_OFF	|  IN5P_OFF	|  IN4P_OFF	|  IN3P_OFF	|  IN2P_OFF	|  IN1P_OFF	|
*	-------------------------------------------------------------------------------------------------
*
*/
#define LOFF_STATP 0x12

/***********************************************************************************************//**
* 	ADS1299 - LOFF_STATN - 0x13
*	|	Bit 7	|	Bit 6	|	Bit 5	|	Bit 4	|	Bit 3	|	Bit 2	|	Bit 1	|	Bit 0	|
*	-------------------------------------------------------------------------------------------------
*	|  IN8N_OFF	|  IN7N_OFF	|  IN6N_OFF	|  IN5N_OFF	|  IN4N_OFF	|  IN3N_OFF	|  IN2N_OFF	|  IN1N_OFF	|
*	-------------------------------------------------------------------------------------------------
*
*/
#define LOFF_STATN 0x13

/***********************************************************************************************//**
* 	ADS1299 - GPIO - 0x14
*	|	Bit 7	|	Bit 6	|	Bit 5	|	Bit 4	|	Bit 3	|	Bit 2	|	Bit 1	|	Bit 0	|
*	-------------------------------------------------------------------------------------------------
*	|			    GPIOD[7:4]						|			    GPIOC[3:0]           			|
*	-------------------------------------------------------------------------------------------------
*
*/
#define GPIO 0x14

/***********************************************************************************************//**
* 	ADS1299 - MISC1 - 0x15
*	|	Bit 7	|	Bit 6	|	Bit 5	|	Bit 4	|	Bit 3	|	Bit 2	|	Bit 1	|	Bit 0	|
*	-------------------------------------------------------------------------------------------------
*	|		Reserved		|	SRB1	|			            Reserved                  			|
*	-------------------------------------------------------------------------------------------------
*
*/
#define MISC1 0x15

#define SRB1                                    0x20 // 00100000

/***********************************************************************************************//**
* 	ADS1299 - MISC2 - 0x16
*	|	Bit 7	|	Bit 6	|	Bit 5	|	Bit 4	|	Bit 3	|	Bit 2	|	Bit 1	|	Bit 0	|
*	-------------------------------------------------------------------------------------------------
*	|			                                Reserved                                			|
*	-------------------------------------------------------------------------------------------------
*
*/
#define MISC2 0x16

/***********************************************************************************************//**
* 	ADS1299 - CONFIG4 - 0x17
*	|	Bit 7	|	Bit 6	|	Bit 5	|	Bit 4	|	Bit 3	|	Bit 2	|	Bit 1	|	Bit 0	|
*	-------------------------------------------------------------------------------------------------
*	|			        Reserverd       			|SINGLE_SHOT| Reserverd	| PD_L_COMP | Reserverd	|
*	-------------------------------------------------------------------------------------------------
*
*/
#define CONFIG4 0x17

#define SINGLE_SHOT                             0x40 // 01000000
#define PD_L_COMP                               0x04 // 00000100

#define SCALE_FACT(x) 1000000 *((4.5/8388607)/x)  // Vref= 4.5V; 2^23-1 = 8388607

#endif

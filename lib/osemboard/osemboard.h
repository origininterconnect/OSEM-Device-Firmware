/*
 * OSEM Board pin configuration file
 *
 * Copyright (c) 2024 OriginInterconnect PVT. LTD.
 * Copyright (c) 2024 Deepak Khatri <deepak@oric.io>
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

#ifndef OSEMBOARD_H
#define OSEMBOARD_H

// Choose your board
#define OCTA_EEG_MARK1 1
#define SLEEPY_EEG_MARK3 0
#define ESP32_S3 0

#if OCTA_EEG_MARK1 or SLEEPY_EEG_MARK3

#define PIN_CS 7 // active low
#define PIN_MISO 10
#define PIN_MOSI 3
#define PIN_SCLK 2

#define PIN_RST 1 // active lOW
#define PIN_DRDY 0 // active lOW

#define PIN_NEO 6

#define PIXEL_BRIGHTNESS 7
#define TRIGGER_PIN 9   // This pin will be checked for to go in OTA mode

#define SPI_CLK 20000000 // SCLK

#elif ESP32_S3

#define PIN_CS 18 // active low
#define PIN_MISO 37
#define PIN_MOSI 35
#define PIN_SCLK 36

#define PIN_RST 38 // active lOW
#define PIN_DRDY 39 // active lOW
#define PIN_NEO 17

#define PIXEL_BRIGHTNESS 7
#define TRIGGER_PIN 0          // This pin will be checked for to go in OTA mode

#define spiClk 8000000 //2Mhz  // 1.5Mhz to 2.4Mhz according to ADS1299 datasheet page 12

#endif

#endif // OSEMBOARD_H
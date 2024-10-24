/*
 * Copyright (c) 2024 OriginInterconnect PVT. LTD.
 * Copyright (c) 2024 Deepak Khatri <deepak@oric.io>
 * Copyright (c) 2013-2019 Adam Feuer <adam@adamfeuer.com>
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

#include <stdio.h>
#include <string.h>
#include "Arduino.h"

#define USE_ARDUINO_SPI_LIBRARY 1
#define USE_NATIVE_ESP32_SPI 0

#if USE_ARDUINO_SPI_LIBRARY

#include <SPI.h>

void spiBegin(uint8_t csPin)
{
    SPI.begin();
    pinMode(csPin, OUTPUT);
}

void spiInit(uint8_t bitOrder, uint8_t spiMode, uint32_t spiFrequency)
{
    SPI.setBitOrder(bitOrder); // MSBFIRST or LSBFIRST
    SPI.setDataMode(spiMode);  // SPI_MODE0, SPI_MODE1; SPI_MODE2; SPI_MODE3
    SPI.setFrequency(spiFrequency);
}

/** SPI receive a byte */
uint8_t spiRec()
{
    return SPI.transfer(0x00);
}

/** SPI receive multiple bytes */
uint8_t spiRec(uint8_t *buf, size_t len)
{
    memset(buf, 0, len);
    SPI.transfer((void *)buf, len);
    return 0;
}

/** SPI send a byte */
void spiSend(uint8_t b)
{
    SPI.transfer(b);
}

/** SPI send multiple bytes */
void spiSend(const uint8_t *buf, size_t len)
{
    SPI.transfer((void *)buf, len);
}

#elif USE_NATIVE_ESP32_SPI
// TODO: implement ESP32 SPI DMA methods
#endif

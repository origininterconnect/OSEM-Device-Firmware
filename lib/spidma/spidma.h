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
#ifndef SPI_DMA_H
#define SPI_DMA_H

void spiBegin(uint8_t csPin);

void spiInit(uint8_t bitOrder, uint8_t spiMode, uint32_t spiFrequency);

uint8_t spiRec();

uint8_t spiRec(uint8_t *buf, size_t len);

void spiSend(uint8_t b);

void spiSend(const uint8_t *buf, size_t len);

#endif // SPI_DMA_H

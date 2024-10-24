/*
 * adsCommand.h
 *
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

#ifndef _ADS_COMMAND_H
#define _ADS_COMMAND_H

#include "Arduino.h"
#include "osemboard.h"

void adcWreg(int reg, int val);
void adcSendCommand(int cmd);
void adcSendCommandLeaveCsActive(int cmd);
int adcRreg(int reg);
uint8_t readData(uint8_t *status, uint8_t *data);

#endif // _ADS_COMMAND_H

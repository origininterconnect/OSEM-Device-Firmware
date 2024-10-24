/**
 * WebSocket Command
 * A Wiring/Arduino library to tokenize and parse commands received over a WebSocket
 *
 * Copyright (c) 2024 OriginInterconnect PVT. LTD.
 * Copyright (c) 2024 Deepak Khatri <deepak@oric.io>
 * Copyright (C) 2013-2019 Adam Feuer <adam@adamfeuer.com>
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

#ifndef WSCOMMAND_H
#define WSCOMMAND_H

#if defined(WIRING) && WIRING >= 100
#include <Wiring.h>
#elif defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include <string.h>
#include <ArduinoJson.h>

#define WSCOMMAND_MAXCOMMANDLENGTH 32

typedef void (*command_func)(unsigned char, unsigned char);

class WSCommand
{
public:
    WSCommand(); // Constructor
    void addCommand(const char *command, void (*function)(unsigned char register_number, unsigned char register_value));
    void executeCommand(uint8_t *payload);
    int findCommand(const char *command);
    void printCommands(); // Prints the list of commands.
    void setDefaultHandler(void (*function)(const char *));

private:
    // Command/handler dictionary
    struct WSCommandCallback
    {
        char command[WSCOMMAND_MAXCOMMANDLENGTH];
        command_func command_function;
    }; // Data structure to hold Command/Handler function key-value pairs
    void (*defaultHandler)(const char *);
    WSCommandCallback *commandList; // Actual definition for command/handler array
    byte commandCount;
};

#endif // WSCOMMAND_H
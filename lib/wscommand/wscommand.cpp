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

#include "wscommand.h"

/**
 * Constructor makes sure some things are set.
 */
WSCommand::WSCommand() : commandList(NULL), commandCount(0) {}

/**
 * Adds a "command" and a handler function to the list of available commands.
 * This is used for matching a found token in the buffer, and gives the pointer
 * to the handler function to deal with it.
 */
void WSCommand::addCommand(const char *command, void (*function)(unsigned char register_number, unsigned char register_value))
{
    ESP_LOGD("COMMAND", "Adding command (%d): %s", commandCount, command);
    commandList = (WSCommandCallback *)realloc(commandList, (commandCount + 1) * sizeof(WSCommandCallback));
    strncpy(commandList[commandCount].command, command, WSCOMMAND_MAXCOMMANDLENGTH);
    commandList[commandCount].command_function = function;
    commandCount++;
}

void WSCommand::setDefaultHandler(void (*function)(const char *))
{
    defaultHandler = function;
}

/**
 * This checks the Serial stream for characters, and assembles them into a buffer.
 * When the terminator character (default '\n') is seen, it starts parsing the
 * buffer for a prefix command, and calls handlers setup by addCommand() member
 */
void WSCommand::executeCommand(uint8_t *payload)
{
    // try to decipher the JSON string received
    JsonDocument json_command;
    DeserializationError error = deserializeJson(json_command, payload);

    if (error)
    {
        ESP_LOGD("JSON", "deserializeJson() failed: %s", error.f_str());
        return;
    }
    JsonObject command_object = json_command.as<JsonObject>();
    JsonVariant command_name_variant = command_object["command"];
    const char *command = command_name_variant.as<const char *>();
    ESP_LOGD("COMMAND", "command: %s", String(command));

    int command_num = findCommand(command);
    if (command_num < 0)
    {   ESP_LOGD("COMMAND", "Invalid command");
        (*defaultHandler)(command);
        return;
    }

    JsonVariant parameters_variant = json_command["parameters"];
    JsonArray params_array = parameters_variant.as<JsonArray>();
    unsigned char register_number = 0;
    unsigned char register_value = 0;
    if (!parameters_variant.isNull())
    {
        JsonArray params_array = parameters_variant.as<JsonArray>();
        size_t number_of_params = params_array.size();
        if (number_of_params > 0)
        {
            register_number = params_array[0];
            ESP_LOGD("COMMAND", "register number: %s", String(register_number));
        }
        if (number_of_params > 1)
        {
            register_value = params_array[1];
            ESP_LOGD("COMMAND", "register value: %s", String(register_value));
        }
    }
    // Execute the stored handler function for the command
    (*commandList[command_num].command_function)(register_number, register_value);
}

int WSCommand::findCommand(const char *command)
{
    int result = -1;
    for (int i = 0; i < commandCount; i++)
    {
        if (strcmp(command, commandList[i].command) == 0)
        {
            result = i;
            break;
        }
    }
    return result;
}

/**
 * Print the list of commands.
 */

void WSCommand::printCommands()
{
    for (int i = 0; i < commandCount; i++)
    {
        ESP_LOGD("COMMAND", "Command: %s", commandList[i].command);
    }
}

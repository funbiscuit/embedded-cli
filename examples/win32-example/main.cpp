/**
 * Simple example of using embedded-cli in win32 application.
 * Create emulator of terminal which prints entered commands and args
 */

#include <iostream>
#include <Windows.h>

#include "embedded_cli.h"

void onCommand(const std::string &name, char *tokens);

int main() {
    HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);

    if (hConsole == INVALID_HANDLE_VALUE) {
        std::cerr << "Can't get handle to console\n";
        return -1;
    }

    EmbeddedCli *cli = embeddedCliNewDefault();

    cli->onCommand = [](EmbeddedCli *embeddedCli, CliCommand *command) {
        embeddedCliTokenizeArgs(command->args);
        onCommand(command->name == nullptr ? "" : command->name, command->args);
    };
    cli->writeChar = [](EmbeddedCli *embeddedCli, char c) {
        std::cout << c;
    };

    std::cout << "Cli is running. Press 'Esc' to exit\n";

    while (true) {
        INPUT_RECORD record;
        DWORD read;
        ReadConsoleInput(hConsole, &record, 1, &read);

        if (read > 0 && record.EventType == KEY_EVENT) {
            KEY_EVENT_RECORD event = record.Event.KeyEvent;
            if (!event.bKeyDown)
                continue;
            char aChar = event.uChar.AsciiChar;

            // escape code
            if (aChar == 27)
                break;

            if (aChar > 0)
                embeddedCliReceiveChar(cli, aChar);

            embeddedCliProcess(cli);
        }
    }

    return 0;
}

void onCommand(const std::string &name, char *tokens) {
    std::cout << "Command: " << name << "\n";

    for (int i = 0; i < embeddedCliGetTokenCount(tokens); ++i) {
        std::cout << "Arg " << i << ": " << embeddedCliGetToken(tokens, i) << "\n";
    }
}

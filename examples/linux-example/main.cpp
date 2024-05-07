/**
 * Simple example of using embedded-cli in Linux application.
 * Shamelessly stolen from Win32 version and modified to run under Linux
 * Runs in terminal / console, using stdio, prints entered commands and args
 */

#include <stdio.h>
#include <unistd.h>
#include <termios.h>

#include <iostream>
#include <string>

#define EMBEDDED_CLI_IMPL
#include "embedded_cli.h"

static bool exitFlag = false;

void onCommand(const std::string &name, char *tokens);

void onExit(EmbeddedCli *cli, char *args, void *context);

void onHello(EmbeddedCli *cli, char *args, void *context);

void onLed(EmbeddedCli *cli, char *args, void *context);

void onAdc(EmbeddedCli *cli, char *args, void *context);

int main() {
    // Single character buffer for reading keystrokes
    unsigned char c;

    // Structures to save the terminal settings for original settings & raw mode
    struct termios original_stdin;
    struct termios raw_stdin;

    // Backup the terminal settings, and switch to raw mode
    tcgetattr(STDIN_FILENO, &original_stdin);
    raw_stdin = original_stdin;
    cfmakeraw(&raw_stdin);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw_stdin);

    EmbeddedCli *cli = embeddedCliNewDefault();

    cli->onCommand = [](EmbeddedCli *embeddedCli, CliCommand *command) {
        embeddedCliTokenizeArgs(command->args);
        onCommand(command->name == nullptr ? "" : command->name, command->args);
    };
    cli->writeChar = [](EmbeddedCli *embeddedCli, char c) {
        write(STDOUT_FILENO, &c, 1);
    };

    embeddedCliAddBinding(cli, {
            "exit",
            "Stop CLI and exit",
            false,
            nullptr,
            onExit
    });
    embeddedCliAddBinding(cli, {
            "get-led",
            "Get current led status",
            false,
            nullptr,
            onLed
    });
    embeddedCliAddBinding(cli, {
            "get-adc",
            "Get current adc value",
            false,
            nullptr,
            onAdc
    });
    embeddedCliAddBinding(cli, {
            "hello",
            "Print hello message",
            true,
            (void *) "World",
            onHello
    });

    std::cout << "Cli is running. Press 'Esc' to exit\r\n";
    std::cout << "Type \"help\" for a list of commands\r\n";
    std::cout << "Use backspace and tab to remove chars and autocomplete\r\n";

    embeddedCliProcess(cli);

    while (!exitFlag) {
        // grab the next character and feed it to the CLI processor
        if(read(STDIN_FILENO,&c,1)>0) {
            embeddedCliReceiveChar(cli, c);
            embeddedCliProcess(cli);
        }
    }

    // restore terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &original_stdin);

    return 0;
}

void onCommand(const std::string &name, char *tokens) {
    std::cout << "Received command: " << name << "\r\n";

    for (int i = 0; i < embeddedCliGetTokenCount(tokens); ++i) {
        std::cout << "Arg " << i << ": " << embeddedCliGetToken(tokens, i + 1) << "\r\n";
    }
}

void onExit(EmbeddedCli *cli, char *args, void *context) {
    exitFlag = true;
    std::cout << "Cli will shutdown now...\r\n";
}

void onHello(EmbeddedCli *cli, char *args, void *context) {
    std::cout << "Hello, ";
    if (embeddedCliGetTokenCount(args) == 0) {
        std::cout << (const char *) context;
    }
    else {
        std::cout << embeddedCliGetToken(args, 1);
    }
    std::cout << "\r\n";
}

void onLed(EmbeddedCli *cli, char *args, void *context) {
    std::cout << "Current led brightness: " << std::rand() % 256 << "\r\n";
}

void onAdc(EmbeddedCli *cli, char *args, void *context) {
    std::cout << "Current adc readings: " << std::rand() % 1024 << "\r\n";
}

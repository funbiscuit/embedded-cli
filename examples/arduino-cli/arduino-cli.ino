/**
 * Simple example of using embedded-cli in arduino.
 * To compile copy embedded-cli.h and embedded-cli.c to sketch directory.
 *
 * With specified settings (32 bytes for RX buffer and no dynamic allocation)
 * library uses 1024 bytes of ROM and 85 bytes of RAM.
 * Total size of firmware is 2576 bytes, 269 bytes of RAM are used.
 */

#include "embedded_cli.h"

// cli uses 15 extra bytes for internal structures (on Arduino Nano)
#define CLI_BUFFER_SIZE 47
#define CLI_RX_BUFFER_SIZE 32

EmbeddedCli *cli;

uint8_t cliBuffer[CLI_BUFFER_SIZE];

void onCommand(EmbeddedCli *embeddedCli, CliCommand *command);

void writeChar(EmbeddedCli *embeddedCli, char c);

void setup() {
    Serial.begin(9600);

    EmbeddedCliConfig *config = embeddedCliDefaultConfig();
    config->cliBuffer = cliBuffer;
    config->cliBufferSize = CLI_BUFFER_SIZE;
    config->rxBufferSize = CLI_RX_BUFFER_SIZE;
    cli = embeddedCliNew(config);

    if (cli == NULL) {
        Serial.println(F("Cli was not created. Check sizes!"));
        return;
    }
    Serial.println(F("Cli has started. Enter your commands."));

    cli->onCommand = onCommand;
    cli->writeChar = writeChar;
}

void loop() {
    if (cli == NULL)
        return;

    // provide all chars to cli
    while (Serial.available() > 0) {
        embeddedCliReceiveChar(cli, Serial.read());
    }

    embeddedCliProcess(cli);
}

void onCommand(EmbeddedCli *embeddedCli, CliCommand *command) {
    Serial.println("received command:");
    Serial.println(command->name);
    embeddedCliTokenizeArgs(command->args);
    for (int i = 0; i < embeddedCliGetTokenCount(command->args); ++i) {
        Serial.print("arg ");
        Serial.print((char) ('0' + i));
        Serial.print(": ");
        Serial.println(embeddedCliGetToken(command->args, i));
    }
}

void writeChar(EmbeddedCli *embeddedCli, char c) {
    Serial.write(c);
}

/**
 * Simple example of using embedded-cli in arduino.
 * To compile copy embedded-cli.h and embedded-cli.c to sketch directory.
 */

#include "embedded_cli.h"

EmbeddedCli *cli;

void onCommand(EmbeddedCli *embeddedCli, CliCommand *command);

void writeChar(EmbeddedCli *embeddedCli, char c);

void setup() {
    cli = embeddedCliNew();
    cli->onCommand = onCommand;
    cli->writeChar = writeChar;

    Serial.begin(9600);
}

void loop() {
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

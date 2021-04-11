/**
 * Simple example of using embedded-cli in arduino.
 * To compile copy embedded-cli.h and embedded-cli.c to sketch directory.
 *
 * With specified settings:
 * 32 bytes for cmd buffer, 16 for RX buffer,
 * 3 binding functions and no dynamic allocation
 * library uses 4218 bytes of ROM and 479 bytes of RAM.
 * Total size of firmware is 6768 bytes, 671 bytes of RAM are used.
 * Most of RAM space is taken up by char arrays so size can be reduced if
 * messages are discarded.
 * For example, by removing code inside onHelp and onUnknown functions inside
 * library (and replacing help strings in bindings by nullptr's) size of FW is
 * reduced by 690 bytes of ROM and 250 bytes of RAM.
 * So library itself will use no more than 3528 bytes of ROM and 229 of RAM.
 *
 */

#include "embedded_cli.h"

// 126 bytes is minimum size for this params on Arduino Nano
#define CLI_BUFFER_SIZE 126
#define CLI_RX_BUFFER_SIZE 16
#define CLI_CMD_BUFFER_SIZE 32
#define CLI_BINDING_COUNT 3

EmbeddedCli *cli;

uint8_t cliBuffer[CLI_BUFFER_SIZE];

void onCommand(EmbeddedCli *embeddedCli, CliCommand *command);

void writeChar(EmbeddedCli *embeddedCli, char c);

void onHello(EmbeddedCli *cli, char *args, void *context);

void onLed(EmbeddedCli *cli, char *args, void *context);

void onAdc(EmbeddedCli *cli, char *args, void *context);

void setup() {
    Serial.begin(9600);

    EmbeddedCliConfig *config = embeddedCliDefaultConfig();
    config->cliBuffer = cliBuffer;
    config->cliBufferSize = CLI_BUFFER_SIZE;
    config->rxBufferSize = CLI_RX_BUFFER_SIZE;
    config->cmdBufferSize = CLI_CMD_BUFFER_SIZE;
    config->maxBindingCount = CLI_BINDING_COUNT;
    cli = embeddedCliNew(config);

    if (cli == NULL) {
        Serial.println(F("Cli was not created. Check sizes!"));
        return;
    }
    Serial.println(F("Cli has started. Enter your commands."));

    embeddedCliAddBinding(cli, {
            "get-led",
            "Get led status",
            false,
            nullptr,
            onLed
    });
    embeddedCliAddBinding(cli, {
            "get-adc",
            "Read adc value",
            false,
            nullptr,
            onAdc
    });
    embeddedCliAddBinding(cli, {
            "hello",
            "Print hello message",
            false,
            (void *) "World",
            onHello
    });

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
    Serial.println(F("Received command:"));
    Serial.println(command->name);
    embeddedCliTokenizeArgs(command->args);
    for (int i = 0; i < embeddedCliGetTokenCount(command->args); ++i) {
        Serial.print(F("arg "));
        Serial.print((char) ('0' + i));
        Serial.print(F(": "));
        Serial.println(embeddedCliGetToken(command->args, i + 1));
    }
}

void onHello(EmbeddedCli *cli, char *args, void *context) {
    Serial.print(F("Hello "));
    if (embeddedCliGetTokenCount(args) == 0)
        Serial.print((const char *) context);
    else
        Serial.print(embeddedCliGetToken(args, 1));
    Serial.print("\r\n");
}

void onLed(EmbeddedCli *cli, char *args, void *context) {
    Serial.print(F("LED: "));
    Serial.print(random(256));
    Serial.print("\r\n");
}

void onAdc(EmbeddedCli *cli, char *args, void *context) {
    Serial.print(F("ADC: "));
    Serial.print(random(1024));
    Serial.print("\r\n");
}

void writeChar(EmbeddedCli *embeddedCli, char c) {
    Serial.write(c);
}

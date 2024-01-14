/**
 * Simple example of using embedded-cli in arduino.
 * To compile copy embedded-cli.h (single header version) to sketch directory.
 *
 * With specified settings:
 * 32 bytes for cmd buffer, 16 for RX buffer,
 * 32 bytes for history,
 * 3 binding functions and no dynamic allocation
 * Total size of firmware is 7538 bytes, 654 bytes of RAM are used.
 * Not everything is used by library, some memory is used by Serial, for
 * example.
 * Most of RAM space is taken up by char arrays so size can be reduced if
 * messages are discarded.
 * For example, by removing code inside onHelp and onUnknown functions inside
 * library (and replacing help strings in bindings by nullptr's) size of FW is
 * reduced by 688 bytes of ROM and 190 bytes of RAM. Total usage is then
 * 6850 of ROM and 464 of RAM.
 */

#define EMBEDDED_CLI_IMPL
#include "embedded_cli.h"

// 164 bytes is minimum size for this params on Arduino Nano
#define CLI_BUFFER_SIZE 166
#define CLI_RX_BUFFER_SIZE 16
#define CLI_CMD_BUFFER_SIZE 32
#define CLI_HISTORY_SIZE 32
#define CLI_BINDING_COUNT 3

EmbeddedCli *cli;

CLI_UINT cliBuffer[BYTES_TO_CLI_UINTS(CLI_BUFFER_SIZE)];

void onCommand(EmbeddedCli *embeddedCli, CliCommand *command);

void writeChar(EmbeddedCli *embeddedCli, char c);

void onHello(EmbeddedCli *cli, char *args, void *context);

void onLed(EmbeddedCli *cli, char *args, void *context);

void onAdc(EmbeddedCli *cli, char *args, void *context);

void setup() {
    Serial.begin(115200);

    EmbeddedCliConfig *config = embeddedCliDefaultConfig();
    config->cliBuffer = cliBuffer;
    config->cliBufferSize = CLI_BUFFER_SIZE;
    config->rxBufferSize = CLI_RX_BUFFER_SIZE;
    config->cmdBufferSize = CLI_CMD_BUFFER_SIZE;
    config->historyBufferSize = CLI_HISTORY_SIZE;
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
            true,
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
    for (int i = 1; i <= embeddedCliGetTokenCount(command->args); ++i) {
        Serial.print(F("arg "));
        Serial.print((char) ('0' + i));
        Serial.print(F(": "));
        Serial.println(embeddedCliGetToken(command->args, i));
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

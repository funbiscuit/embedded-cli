# Embedded CLI

Simple CLI library intended for use in embedded systems (like STM32 or Arduino).

![Arduino Demo](examples/arduino-demo.gif)

## Features
* Dynamic or static allocation
* Configurable memory usage
* Command-to-function binding with arguments support
* Live autocompletion (see demo below)
* Tab (jump to end of current autocompletion) and backspace (remove char) support
* History support (navigate with up and down keypress)
* Any byte-stream interface is supported (for example, UART)

## Introduction
This package contains files to implement a simple command-line interface.
The package includes cli.h, and cli.c.

## Integration
* Add embedded_cli.h and embedded_cli.c to your project
* Include the embedded_cli.h header file in your code:

```c
#include "embedded_cli.h"
```

### Initialisation
To create CLI, you'll need to provide a desired config. Best way is to get default config and change desired values.
For example, change maximum amount of command bindings:
```c
EmbeddedCliConfig *config = embeddedCliDefaultConfig();
config->maxBindingCount = 16;
```
Create an instance of CLI:
```c
EmbeddedCli *cli = embeddedCliNew(config);
```
If default arguments are good enough for you, you can create cli with default config:
```c
EmbeddedCli *cli = embeddedCliNewDefault(config);
```
Provide a function that will be used to send chars to the other end:
```c
void writeChar(EmbeddedCli *embeddedCli, char c);
// ...
cli->writeChar = writeChar;
```
After creation, provide desired bindings to CLI:
```c
embeddedCliAddBinding(cli, {
        "get-led",          // command name (spaces are not allowed)
        "Get led status",   // Optional help for a command (NULL for no help)
        false,              // flag whether to tokenize arguments (see below)
        nullptr,            // optional pointer to any application context
        onLed               // binding function 
});
embeddedCliAddBinding(cli, {
        "get-adc",
        "Read adc value",
        true,
        nullptr,
        onAdc
});
```
Don't forget to create binding functions as well:
```c
void onLed(EmbeddedCli *cli, char *args, void *context) {
    // use args as raw null-terminated string of all arguments
}
void onAdc(EmbeddedCli *cli, char *args, void *context) {
    // use args as list of tokens
}
```
CLI has functions to easily handle list of space separated arguments. If you have null-terminated string
you can convert it to list of tokens with single call:
```c
embeddedCliTokenizeArgs(args);
```
**Note**: Initial array will be modified (so it must be non const), do not use it after this call directly.
After that you can count arguments or get single argument as a null-terminated string:
```c
const char * arg = embeddedCliGetToken(args, pos); // args are counted from 1 (not from 0)

uint8_t pos = embeddedCliFindToken(args, argument);

uint8_t count = embeddedCliGetTokenCount(const char *tokenizedStr);
```

Look into examples to find out more.

### Runtime
At runtime you need to provide all received chars to cli:
```c
// char c = Serial.read();
embeddedCliReceiveChar(cli, c);
```
This function can be called from normal code or from ISRs. This call puts char into internal buffer,
but no processing is done yet. To do all the "hard" work, call process function periodically
```c
embeddedCliProcess(cli);
```

### Static allocation
CLI can be used with statically allocated buffer for its internal structures. Required size of buffer depends on
CLI configuration. If size is not enough, NULL is returned from ```embeddedCliNew```.
To get required size for your config use this call:
```c
uint16_t size = embeddedCliRequiredSize(config);
```
Create a buffer of required size and provide it to CLI:
```c
uint8_t *cliBuffer[CLI_BUFFER_SIZE];
// ...
config->cliBuffer = cliBuffer;
config->cliBufferSize = CLI_BUFFER_SIZE;
```
If ```cliBuffer``` in config is NULL, dynamic allocation (with malloc) is used.
In such case size is computed automatically.


## User Guide
You'll need to begin communication (usually through a UART) with a device running a CLI.
Terminal is required for correct experience. Following control sequences are reserved:
* \r or \n sends a command (\r\n is also supported)
* \b removes last typed character
* \t moves cursor to the end of autocompleted command
* Esc[A (key up) and Esc[B (key down) navigates through history

If you run CLI through a serial port (like on Arduino with its UART-USB converter),
you can use for example PuTTY (Windows) or XTerm (Linux).

## Examples
There is an example for Arduino (tested with Arduino Nano, but should work on anything with at least 1kB of RAM).
Look inside examples directory for a full code.

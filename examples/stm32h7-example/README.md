# STM32H7 example

This STM32 UART example is created for the `STM32H750IBK6 MCU`, placed on a `Electrosmith Daisy Seed` development board.
But below I have described the process, so it should be recreate-able for every STM32 MCU.

# STM32CubeMX general settings

This example is using UART interrupts for each received character, and has a `cli_printf()` function to easily be able
to print formatted strings to the CLI, without the cursor location being incorrect. All written in C.

I have created it using the STM32 HAL, and using the STM32CubeMX configurator (.ioc file), but the process should be
more/less the same for LL drivers & using custom cmake files.

This example uses the STM32CubeMX option: "Generate peripheral initialization as a pair of `.c/.h` files per
peripheral", since I personally like that option enabled. It is found under "Project Manager/Code Generator"
in `STM32CubeMX`.

I've also split the initialization and command binding up into multiple files, for easier re-use in other projects.

# Steps to recreate from IOC file

**Step 0.**

Import project with using .ioc file to your own workspace (can be done by right-clicking
in `STM32CubeIDE->import->Import an Existing STM32CubeMX Configiration File (.ioc)`, or can be done directly from
standalone `STM32CubeMX`). If using this .ioc file, you could skip step 1, step 2 and step 3 (these steps should already
be set in the delivered .ioc file, but it's always a good idea to double-check).

**Step 1.**

Enable U(S)ART in asynchronous mode. This example uses USART1, 115200 Bits/s, 8 bit words, no parity and 1 stop bit. The
H7 family has quite a lot of advanced USART features. Disable at least: `Auto Baudrate`, `Overrun`
and `DMA on RX Error`.

**Step 2.**

Enable USARTx Global Interrupt in the NVIC. I personally like to enable interrupt `select for init sequence order` to
make sure this is the first global interrupt to be handled.

**Step 3.**

Configure your clock correctly. The board I used for this example (Electrosmith Daisy Seed), has an 16 MHz external
resonator. So in RCC, I enabled HSE to: `Crystal/Ceramic resonator` with the default settings.
Then in clock configurator I set the input frequency to 16MHz, and solved my clock issues to get an USART1 frequency of
32MHz.

**Step 4.**

Copy the following `.c` and or `.h` files into their corresponding source/header directories:

* `embedded_cli.h` (single header version, can be
  downloaded [here](https://github.com/funbiscuit/embedded-cli/releases/tag/v0.1.2)
* `cli_setup.c/.h` (a source/header combo to separate the setup settings from your application, found in this
  stm32h7-example folder)
* `cli_binding.c/.h` (a source/header combo to separate the command binding from your application, found in this
  stm32h7-example folder)

**Step 5.**

Change the CLI settings to your liking in the `cli_setup.h` file (USART peripheral as enabled in your .ioc settings,
buffer sizes, `cli_printf()` max length etc).

**Step 6.**

In you main application (`main.c` for this example), include `cli_setup.h`.
After NVIC initialization, call the `setupCli()` function.

**Step 7.**

Periodically call the `void embeddedCliProcess(EmbeddedCli *cli)` function.<br>
I have created a getter for the `EmbeddedCli *cli` parameter, to make sure there is always only one instance of
EmbeddedCli. Easiest way to periodically call this function is to add:<br>
`embeddedCliProcess(getCliPointer()); HAL_Delay(10);` to the main `while(1)` loop. <br>Change the delay to you liking,
or use an RTOS for task scheduling (out of scope for this example).

**Step 8.**

That's it! Connect the debugger, upload and enjoy. <br>Command bindings can be changed/added in `cli_binding.c`. I've
left my testing bindings there for you to reuse, improve or change.
I also added a `cli_binding.h` file to make the callback functions accesible from code as well.
<br>I've used this to be able to clear the terminal output via the CLI, but also in the `setupCli()` function, after
each new upload.

# Printing to the CLI

This example also uses an encapsulation function, to be able to use `printf()` and the CLI at the same time without any
characters being out of place (function called `cli_printf`). I personally like to have all my callback functions and
re-directions and encapsulation functions in the `cli_setup.c` file (since this example only uses 1 UART). But you can
obviously move these functions to other files to make it more generic.

# Implemented example

In the following repository, you can find a fully implemented reference project, for the `STM32H750IBK6` MCU including
all needed HAL driver files & peripheral initialization files. This repository is ready to be built and flashed to the
MCU. This way you would not need the STM32CubeIDE or STM32CubeMX software installed to try out the example.
https://github.com/NeusAap/embedded-cli-stm32h7-reference

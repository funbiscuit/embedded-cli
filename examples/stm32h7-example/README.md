This STM32 example is created for the STM32H750IBK6 MCU.
But below I have described the process, so it should be recreate-able for every STM32 MCU.

This example is using interrupts for each received character, and has re-routed (and buffered) _write function for easy prints to the CLI.

I have created it using the STM32 HAL, and using the STM32CubeMX configurator (.ioc file), but the process should be more/less the same for LL drivers & using custom cmake files.

This example uses the STM32CubeMX option: 'Generate peripheral initialization as pair of '.c/.h' files per peripheral', since I personally like that option enabled. It is found under 'Project Manager/Code Generator' in STM32CubeMX.

Step 1.
Enable U(S)ART in asynchronous mode. This example uses USART1, 115200 Bits/s, 8 bit words, no parity and 1 stop bit. The H7 family has quite a lot of advanced USART features. Disable at least: 'Auto Baudrate', 'Overrun' and 'DMA on RX Error'.

Step 2.
Enable USARTx Global Interrupt in the NVIC. I personally like to enable interrupt 'select for init sequence order' to make sure this is the first global interrupt to be handled.

Step 3.
Configure your clock correctly. The board I used for this example (Electrosmith Daisy Seed), has an 16 MHz external resonator. So in RCC, I enabled HSE to: 'Crystal/Ceramic resonator' with the default settings.
Then in clock configurator I set the input frequency to 16MHz, and solved my clock issues to get an USART1 frequency of 32MHz.


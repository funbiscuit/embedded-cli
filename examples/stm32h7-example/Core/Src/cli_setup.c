/*
 * cli_setup.c
 *
 *  Created on: Jul 7, 2023
 *      Author: NeusAap
 */


// STD LIBS
#include <stdio.h>
#include <stdlib.h>

// STM files
#include "usart.h"
#include "stm32h7xx_it.h"

// Own CLI headers
#include "cli_setup.h"
#include "cli_binding.h"
// Embedded CLI header (only one allowed per project)
#define EMBEDDED_CLI_IMPL

// UART buffers
uint8_t UART_CLI_rxBuffer[UART_RX_BUFF_SIZE] = {0};
struct {
    char buffer[UART_TX_BUFFER_SIZE];
    uint8_t index;
} UART_CLI_txBuffer = { .index = 0 };

// CLI buffer
EmbeddedCli *cli;
CLI_UINT cliBuffer[BYTES_TO_CLI_UINTS(CLI_BUFFER_SIZE)];

// Bool to disable the interrupts, if CLI is not yet ready.
bool cliIsReady = false;

// Getter function, to keep only one instance of the EmbeddedCli pointer in this file.
EmbeddedCli* getCliPointer(){
	return cli;
}

// Write function used in 'setupCli()' to route the chars over UART.
void writeCharToCli(EmbeddedCli *embeddedCli, char c){
	uint8_t c_to_send = c;
    HAL_UART_Transmit(UART_CLI_PERIPH, &c_to_send, 1, 100);
}

// Function to setup the configuration settings for the CLI, based on the definitions from this header file
void setupCli(){
  // UART interrupt
  HAL_UART_Receive_IT (UART_CLI_PERIPH, UART_CLI_rxBuffer, UART_RX_BUFF_SIZE);

  // Initialize the CLI configuration settings
  EmbeddedCliConfig *config = embeddedCliDefaultConfig();
  config->cliBuffer = cliBuffer;
  config->cliBufferSize = CLI_BUFFER_SIZE;
  config->rxBufferSize = CLI_RX_BUFFER_SIZE;
  config->cmdBufferSize = CLI_CMD_BUFFER_SIZE;
  config->historyBufferSize = CLI_HISTORY_SIZE;
  config->maxBindingCount = CLI_MAX_BINDING_COUNT;

  // Create new CLI instance
  cli = embeddedCliNew(config);
  // Assign character write function
  cli->writeChar = writeCharToCli;

  // CLI init failed. Is there not enough memory allocated to the CLI?
  // Please increase the 'CLI_BUFFER_SIZE' in header file.
  // Or decrease max binding / history size.
  while (cli == NULL){
	  HardFault_Handler();
  }

  // Un-comment to add a non-default reaction to unbound commands.
  //cli->onCommand = onCliCommand;

  // Add all the initial command bindings
  initCliBinding();

  // Init the CLI with blank screen
  onClearCLI(cli, NULL, NULL);

  // CLI has now been initialized, set bool to true to enable interrupts.
  cliIsReady = true;
}

// STM32 UART callback function, to pass received characters to the embedded-cli
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart == UART_CLI_PERIPH && cliIsReady){
		HAL_UART_Receive_IT(UART_CLI_PERIPH, UART_CLI_rxBuffer, UART_RX_BUFF_SIZE);
		char c = UART_CLI_rxBuffer[0];
		embeddedCliReceiveChar(cli, c);
	}
}

// Custom write function to route printf statements to the CLI.
// Input is buffered until buffer is full, or \n is encountered to reduce HAL calls.
void custom_write(int file, char *ptr, int len) {
    int i;

    for (i = 0; i < len; i++) {
    	UART_CLI_txBuffer.buffer[UART_CLI_txBuffer.index++] = *ptr++;

        // Check if the buffer is full or a newline character is encountered
        if (UART_CLI_txBuffer.index == UART_TX_BUFFER_SIZE || *(ptr - 1) == '\n') {
            HAL_UART_Transmit(UART_CLI_PERIPH, (uint8_t*)UART_CLI_txBuffer.buffer, UART_CLI_txBuffer.index, 100);
            UART_CLI_txBuffer.index = 0;
        }
    }
}

// Redirect stdout to custom _write function
int _write(int file, char *ptr, int len) {
    custom_write(file, ptr, len);
    return len;
}

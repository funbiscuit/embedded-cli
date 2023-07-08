/*
 * cli_setup.h
 *
 *  Created on: Jul 7, 2023
 *      Author: NeusAap
 */

#ifndef INC_CLI_SETUP_H_
#define INC_CLI_SETUP_H_

#include "embedded_cli.h"

// Definitions for CLI sizes
#define CLI_BUFFER_SIZE 2048
#define CLI_RX_BUFFER_SIZE 16
#define CLI_CMD_BUFFER_SIZE 32
#define CLI_HISTORY_SIZE 32
#define CLI_MAX_BINDING_COUNT 32

// Definitions for CLI UART peripheral
#define UART_CLI_PERIPH &huart1
#define UART_RX_BUFF_SIZE 1		// Char for char is needed for instant char echo, so size 1
#define UART_TX_BUFFER_SIZE 128 // Out-bound can be any size. Chars are send if buff full, or \n is found


// Function to setup the configuration settings for the CLI, based on the definitions from this header file
void setupCli();

// Write function used in 'setupCli()' to route the chars over UART.
void writeCharToCli(EmbeddedCli *embeddedCli, char c);

// Getter function, to keep only one instance of the EmbeddedCli pointer in this file.
EmbeddedCli* getCliPointer();

#endif /* INC_CLI_SETUP_H_ */

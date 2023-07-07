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

/**
 * Definition of the cli_printf() buffer size.
 * Can make smaller to decrease RAM usage,
 * make larger to be able to print longer strings.
 */
#define CLI_PRINT_BUFFER_SIZE 512

// Definitions for CLI UART peripheral
#define UART_CLI_PERIPH &huart1

/**
 * Char for char is needed for instant char echo, so size 1
 */
#define UART_RX_BUFF_SIZE 1

/**
 * Function to setup the configuration settings for the CLI,
 * based on the definitions from this header file
 */
void setupCli();

/**
 * Function to encapsulate the 'embeddedCliPrint()' call with
 * print formatting arguments (act like printf(), but keeps cursor at correct location).
 * The 'embeddedCliPrint()' function does already add a linebreak ('\r\n')
 * to the end of the print statement, so no need to add it yourself.
 * @param format format string
 * @param ... format arguments
 */
void cli_printf(const char *format, ...);

/**
 * Getter function, to keep only one instance of the EmbeddedCli pointer in this file.
 * @return
 */
EmbeddedCli *getCliPointer();

#endif /* INC_CLI_SETUP_H_ */

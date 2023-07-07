/*
 * cli_binding.h
 *
 *  Created on: Jul 7, 2023
 *      Author: NeusAap
 */

#ifndef INC_CLI_BINDING_H_
#define INC_CLI_BINDING_H_

/**
 * Clears the whole terminal.
 */
void onClearCLI(EmbeddedCli *cli, char *args, void *context);

/**
 * Example callback function, that also parses 2 arguments,
 * and has an 'incorrect usage' output.
 */
void onLed(EmbeddedCli *cli, char *args, void *context);

/**
 * Function to bind command bindings you'd like to be added at setup of the CLI.
 */
void initCliBinding();

#endif /* INC_CLI_BINDING_H_ */

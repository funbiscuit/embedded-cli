/*
 * cli_binding.c
 *
 *  Created on: Jul 7, 2023
 *      Author: NeusAap
 */

#include <stdio.h>

#include "cli_setup.h"
#include "cli_binding.h"

// Definition to easily access the CLI pointer.
#define CLI getCliPointer()

// Command binding for the clear command
CliCommandBinding clear_binding = {
	.name = "clear",
	.help = "Clears the console",
	.tokenizeArgs = true,
	.context = NULL,
	.binding = onClearCLI
};

// Command binding for the led command
CliCommandBinding led_binding = {
	.name = "get-led",
	.help = "Get led status",
	.tokenizeArgs = true,
	.context = NULL,
	.binding = onLed
};


// Clears the whole terminal.
void onClearCLI(EmbeddedCli *cli, char *args, void *context) {
	printf("\33[2J\r\n");
}

// Example callback function, that also parses 2 arguments, and has a 'incorrect usage' output.
// Make sure to check if 'args' != NULL, printf's '%s' formatting does not like a null pointer.
void onLed(EmbeddedCli *cli, char *args, void *context) {
	if(args == NULL){
		goto usage;
		return;
	}
	const char * arg1 = embeddedCliGetToken(args, 1);
	const char * arg2 = embeddedCliGetToken(args, 2);
	if (arg1 == NULL || arg2 == NULL){
		goto usage;
		return;
	}

	printf("LED with args: %s and %s\r\n", arg1, arg2);
	return;
	//printf("With arguments: %s\r\n", args);
	usage:
		printf("usage: get-led [arg1] [arg2]\r\n");
}

// Command bindings you'd like to be added at setup of the CLI.
void initCliBinding(){
  embeddedCliAddBinding(CLI, clear_binding);
  embeddedCliAddBinding(CLI, led_binding);

}

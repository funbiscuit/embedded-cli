/*
 * cli_binding.c
 *
 *  Created on: Jul 7, 2023
 *      Author: NeusAap
 */

#include <stdio.h>

#include "cli_setup.h"
#include "cli_binding.h"

#define CLI getCliPointer()

CliCommandBinding clear_binding = {
	.name = "clear",
	.help = "Clears the console",
	.tokenizeArgs = true,
	.context = NULL,
	.binding = onClearCLI
};

CliCommandBinding led_binding = {
	.name = "get-led",
	.help = "Get led status",
	.tokenizeArgs = true,
	.context = NULL,
	.binding = onLed
};


void onClearCLI(EmbeddedCli *cli, char *args, void *context) {
	printf("\33[2J\r\n");
}

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


void initCliBinding(){
  // Add CLI command bindings
  embeddedCliAddBinding(CLI, clear_binding);
  embeddedCliAddBinding(CLI, led_binding);

}

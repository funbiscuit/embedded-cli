/*
 * cli_binding.c
 *
 *  Created on: Jul 7, 2023
 *      Author: NeusAap
 */

#include <stdio.h>

#include "cli_setup.h"
#include "cli_binding.h"

void onClearCLI(EmbeddedCli *cli, char *args, void *context) {
    cli_printf("\33[2J");
}

void onLed(EmbeddedCli *cli, char *args, void *context) {
    const char *arg1 = embeddedCliGetToken(args, 1);
    const char *arg2 = embeddedCliGetToken(args, 2);
    if (arg1 == NULL || arg2 == NULL) {
        cli_printf("usage: get-led [arg1] [arg2]");
        return;
    }
    // Make sure to check if 'args' != NULL, printf's '%s' formatting does not like a null pointer.
    cli_printf("LED with args: %s and %s", arg1, arg2);
}

void initCliBinding() {
    // Define bindings as local variables, so we don't waste static memory

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

    EmbeddedCli *cli = getCliPointer();
    embeddedCliAddBinding(cli, clear_binding);
    embeddedCliAddBinding(cli, led_binding);
}

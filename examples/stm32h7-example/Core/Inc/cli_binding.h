/*
 * cli_binding.h
 *
 *  Created on: Jul 7, 2023
 *      Author: NeusAap
 */

#ifndef INC_CLI_BINDING_H_
#define INC_CLI_BINDING_H_

void initCliBinding();
void onClearCLI(EmbeddedCli *cli, char *args, void *context);
void onLed(EmbeddedCli *cli, char *args, void *context);

#endif /* INC_CLI_BINDING_H_ */

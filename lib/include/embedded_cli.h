#ifndef EMBEDDED_CLI_H
#define EMBEDDED_CLI_H


#ifdef __cplusplus

#include <cstdint>

extern "C" {
#else

#include <stdint.h>
#include <stdbool.h>

#endif

typedef struct CliCommand CliCommand;
typedef struct EmbeddedCli EmbeddedCli;


struct CliCommand {
    /**
     * Name of the command.
     * In command "set led 1 1" "set" is name
     */
    const char *name;

    /**
     * String of arguments of the command.
     * In command "set led 1 1" "led 1 1" is string of arguments
     */
    const char *args;
};


struct EmbeddedCli {
    /**
     * Should write char to connection
     * @param cli - pointer to cli that executed this function
     * @param c   - actual character to write
     */
    void (*writeChar)(EmbeddedCli *cli, char c);

    /**
     * Called when command is received
     * @param cli     - pointer to cli that executed this function
     * @param command - pointer to received command
     */
    void (*onCommand)(EmbeddedCli *cli, CliCommand *command);

    /**
     * Can be used by for any application context
     */
    void *appContext;

    /**
     * Pointer to actual implementation, do not use.
     */
    void *_impl;
};

/**
 * Create new CLI, memory is allocated dynamically, call embeddedCliFree after
 * usage.
 * After CLI is created, override function pointers to start using it
 * @return pointer to created CLI
 */
EmbeddedCli *embeddedCliNew(void);

/**
 * Receive character and put it to internal buffer
 * Actual processing is done inside embeddedCliProcess
 * @param cli
 * @param c   - received char
 */
void embeddedCliReceiveChar(EmbeddedCli *cli, char c);

/**
 * Process rx/tx buffers. Command callbacks are called from here
 * @param cli
 */
void embeddedCliProcess(EmbeddedCli *cli);

/**
 * Free allocated for cli memory
 * @param cli
 */
void embeddedCliFree(EmbeddedCli *cli);

#ifdef __cplusplus
}
#endif


#endif //EMBEDDED_CLI_H

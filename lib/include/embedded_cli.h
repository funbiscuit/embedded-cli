#ifndef EMBEDDED_CLI_H
#define EMBEDDED_CLI_H


#ifdef __cplusplus

extern "C" {
#else

#include <stdbool.h>

#endif

// cstdint is available only since C++11, so use C header
#include <stdint.h>

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
     * Is ended with double 0x00 char
     * Use tokenize functions to easily get individual tokens
     */
    char *args;
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

/**
 * Perform tokenization of arguments string. Original string is modified and
 * should not be used directly (only inside other token functions).
 * Individual tokens are separated by single 0x00 char, double 0x00 is put at
 * the end of token list. After calling this function, you can use other
 * token functions to get individual tokens and token count.
 *
 * Important: Call this function only once. Otherwise information will be lost if
 * more than one token existed
 * @param args - string to tokenize (must have extra writable char after 0x00)
 * @return
 */
void embeddedCliTokenizeArgs(char *args);

/**
 * Return specific token from tokenized string
 * @param tokenizedStr
 * @param pos
 * @return token
 */
const char *embeddedCliGetToken(const char *tokenizedStr, uint8_t pos);

/**
 * Return number of tokens in tokenized string
 * @param tokenizedStr
 * @return number of tokens
 */
uint8_t embeddedCliGetTokenCount(const char *tokenizedStr);

#ifdef __cplusplus
}
#endif


#endif //EMBEDDED_CLI_H

#include <stdlib.h>
#include <string.h>

#include "embedded_cli.h"

#define PREPARE_IMPL(t) \
  EmbeddedCliImpl* impl = (EmbeddedCliImpl*)t->_impl;

#define RX_BUFFER_SIZE 64

typedef struct EmbeddedCliImpl EmbeddedCliImpl;

struct EmbeddedCliImpl {
    /**
     * Buffer for storing received chars. Extra space for extra char that used
     * in tokenization (to indicate end of tokens)
     */
    char rxBuffer[RX_BUFFER_SIZE + 1];

    /**
     * Current size of buffer
     */
    uint16_t rxSize;
};

static void removeUnfinishedCommand(EmbeddedCli *cli);

static bool isWhitespace(char c);


EmbeddedCli *embeddedCliNew(void) {
    EmbeddedCli *cli = malloc(sizeof(EmbeddedCli));

    if (cli == NULL)
        return NULL;
    memset(cli, 0, sizeof(EmbeddedCli));

    cli->_impl = malloc(sizeof(EmbeddedCliImpl));
    if (cli->_impl == NULL) {
        free(cli);
        return NULL;
    }
    memset(cli->_impl, 0, sizeof(EmbeddedCliImpl));

    return cli;
}

void embeddedCliReceiveChar(EmbeddedCli *cli, char c) {
    PREPARE_IMPL(cli)

    // if we can't receive more characters, remove command completely
    if (impl->rxSize >= RX_BUFFER_SIZE) {
        removeUnfinishedCommand(cli);
        return;
    }

    impl->rxBuffer[impl->rxSize] = c;
    impl->rxSize++;
}

void embeddedCliProcess(EmbeddedCli *cli) {
    PREPARE_IMPL(cli)

    char *cmdName = NULL;
    char *cmdArgs = NULL;
    bool nameFinished = false;

    for (int i = 0; i < impl->rxSize; ++i) {
        char c = impl->rxBuffer[i];

        if (c == '\r') {
            impl->rxBuffer[i] = '\0';
            impl->rxBuffer[i + 1] = '\0';
            // send command

            if (cmdName != NULL && cli->onCommand != NULL) {

                CliCommand command;
                command.name = cmdName;
                command.args = cmdArgs;

                cli->onCommand(cli, &command);
            }

            cmdName = NULL;
            cmdArgs = NULL;
            nameFinished = false;

            if (i + 1 < RX_BUFFER_SIZE)
                memmove(impl->rxBuffer, &impl->rxBuffer[i + 1], RX_BUFFER_SIZE - i - 1);

            impl->rxSize -= i + 1;
            i = -1;

            continue;
        }
        if (isWhitespace(c)) {
            if (cmdArgs == NULL)
                impl->rxBuffer[i] = '\0';
            if (cmdName != NULL)
                nameFinished = true;
            continue;
        }

        if (cmdName == NULL)
            cmdName = &impl->rxBuffer[i];
        else if (cmdArgs == NULL && nameFinished)
            cmdArgs = &impl->rxBuffer[i];
    }
}

void embeddedCliFree(EmbeddedCli *cli) {
    free(cli->_impl);
    free(cli);
}

void embeddedCliTokenizeArgs(char *args) {

    // for now only space, but can add more later
    const char *separators = " ";
    size_t len = strlen(args);
    // place extra null char to indicate end of tokens
    args[len + 1] = '\0';

    if (len == 0)
        return;

    // replace all separators with \0 char
    for (int i = 0; i < len; ++i) {
        if (strchr(separators, args[i]) != NULL) {
            args[i] = '\0';
        }
    }

    // compress all sequential null-chars to single ones, starting from end

    size_t nextTokenStartIndex = 0;
    size_t i = len;
    while (i > 0) {
        --i;
        bool isSeparator = strchr(separators, args[i]) != NULL;

        if (!isSeparator && args[i + 1] == '\0') {
            // found end of token, move tokens on the right side of this one
            if (nextTokenStartIndex != 0 && nextTokenStartIndex - i > 2) {
                // will copy all tokens to the right and two null-chars
                memmove(&args[i + 2], &args[nextTokenStartIndex], len - nextTokenStartIndex + 1);
            }
        } else if (isSeparator && args[i + 1] != '\0') {
            nextTokenStartIndex = i + 1;
        }
    }

    // remove null chars from the beginning
    if (args[0] == '\0' && nextTokenStartIndex > 0) {
        memmove(args, &args[nextTokenStartIndex], len - nextTokenStartIndex + 1);
    }
}

const char *embeddedCliGetToken(const char *tokenizedStr, uint8_t pos) {
    int i = 0;
    int tokenCount = 0;
    while (true) {
        if (tokenCount == pos)
            break;

        if (tokenizedStr[i] == '\0') {
            ++tokenCount;
            if (tokenizedStr[i + 1] == '\0')
                break;
        }

        ++i;
    }

    if (tokenizedStr[i] != '\0')
        return &tokenizedStr[i];
    else
        return NULL;
}

uint8_t embeddedCliGetTokenCount(const char *tokenizedStr) {
    if (tokenizedStr[0] == '\0')
        return 0;

    int i = 0;
    int tokenCount = 1;
    while (true) {
        if (tokenizedStr[i] == '\0') {
            if (tokenizedStr[i + 1] == '\0')
                break;
            ++tokenCount;
        }
        ++i;
    }

    return tokenCount;
}

static void removeUnfinishedCommand(EmbeddedCli *cli) {
    PREPARE_IMPL(cli)

    if (impl->rxSize == 0)
        return;

    // find last '\r' and remove everything else
    for (int i = impl->rxSize - 1; i >= 0; --i) {
        if (impl->rxBuffer[i] == '\r') {
            impl->rxSize = i + 1;
            break;
        }
        if (i == 0) {
            impl->rxSize = 0;
            break;
        }
    }
}

static bool isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\0';
}

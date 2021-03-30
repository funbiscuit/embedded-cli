#include <stdlib.h>
#include <string.h>

#include "embedded_cli.h"

#define PREPARE_IMPL(t) \
  EmbeddedCliImpl* impl = (EmbeddedCliImpl*)t->_impl;

#define RX_BUFFER_SIZE 64

typedef struct EmbeddedCliImpl EmbeddedCliImpl;

struct EmbeddedCliImpl {
    char rxBuffer[RX_BUFFER_SIZE];

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
            if(cmdName != NULL)
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

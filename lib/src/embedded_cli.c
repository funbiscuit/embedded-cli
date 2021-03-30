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
    uint8_t rxSize;
};


EmbeddedCli *embeddedCliNew(void) {
    EmbeddedCli *cli = malloc(sizeof(EmbeddedCli));

    if (cli == NULL)
        return NULL;
    memset(cli, 0, sizeof(EmbeddedCli));

    cli->_impl = malloc(sizeof(EmbeddedCliImpl));
    if(cli->_impl == NULL) {
        free(cli);
        return NULL;
    }
    memset(cli->_impl, 0, sizeof(EmbeddedCliImpl));

    return cli;
}

void embeddedCliReceiveChar(EmbeddedCli *cli, char c) {

}

void embeddedCliProcess(EmbeddedCli *cli) {

}

void embeddedCliFree(EmbeddedCli *cli) {
    free(cli->_impl);
    free(cli);
}

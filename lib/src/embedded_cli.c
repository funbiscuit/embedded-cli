#include <stdlib.h>
#include <string.h>

#include "embedded_cli.h"

#define PREPARE_IMPL(t) \
  EmbeddedCliImpl* impl = (EmbeddedCliImpl*)t->_impl;

typedef struct EmbeddedCliImpl EmbeddedCliImpl;

struct EmbeddedCliImpl {
    uint8_t i;
};

EmbeddedCli *embeddedCliNew(void) {
    return NULL;
}

void embeddedCliReceiveChar(EmbeddedCli *cli, char c) {

}

void embeddedCliProcess(EmbeddedCli *cli) {

}

void embeddedCliFree(EmbeddedCli *cli) {

}

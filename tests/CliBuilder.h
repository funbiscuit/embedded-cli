
#ifndef EMBEDDED_CLI_CLIBUILDER_H
#define EMBEDDED_CLI_CLIBUILDER_H

#include "CliWrapper.h"
#include "embedded_cli.h"

class CliBuilder {
public:
    CliBuilder();

    CliBuilder &autocomplete(bool enabled);

    CliWrapper build();

    CliBuilder &invitation(const char *text);

    CliBuilder &staticAllocation();

private:
    EmbeddedCliConfig *config;
    bool useStatic = false;
};


#endif //EMBEDDED_CLI_CLIBUILDER_H

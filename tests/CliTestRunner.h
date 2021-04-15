#ifndef EMBEDDED_CLI_CLITESTS_H
#define EMBEDDED_CLI_CLITESTS_H

#include "embedded_cli.h"
#include "CliMock.h"

class CliTestRunner {
public:

    explicit CliTestRunner(EmbeddedCli *cli);

    void runTests();

private:
    EmbeddedCli *cli;
    CliMock mock;

    void testBase();

    void testHistory();

    void testAutocomplete();

    void testPrinting();

    void testHelp();
};

#endif //EMBEDDED_CLI_CLITESTS_H

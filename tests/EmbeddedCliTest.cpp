#include "embedded_cli.h"
#include "CliMock.h"
#include "CliTestRunner.h"

#include <cstring>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("EmbeddedCli", "[cli]") {
    EmbeddedCli *cli = embeddedCliNewDefault();

    REQUIRE(cli != nullptr);

    CliTestRunner runner(cli);

    runner.runTests();

    embeddedCliFree(cli);
}

TEST_CASE("EmbeddedCli. Static allocation", "[cli]") {
    EmbeddedCliConfig *config = embeddedCliDefaultConfig();
    uint16_t minSize = embeddedCliRequiredSize(config);
    REQUIRE(minSize > 0);

    SECTION("Test creation") {
        INFO("Can't create CLI with small buffer");
        for (uint16_t uintCount = 1; uintCount < (uint16_t) BYTES_TO_CLI_UINTS(minSize); ++uintCount) {
            std::vector<CLI_UINT> data(uintCount);
            config->cliBuffer = data.data();
            config->cliBufferSize = uintCount * CLI_UINT_SIZE;
            REQUIRE(embeddedCliNew(config) == nullptr);
        }
        INFO("Can create CLI with buffer of minimal size");
        std::vector<CLI_UINT> data(BYTES_TO_CLI_UINTS(minSize));
        config->cliBuffer = data.data();
        config->cliBufferSize = (uint16_t) data.size() * CLI_UINT_SIZE;
        EmbeddedCli *cli = embeddedCliNew(config);

        REQUIRE(cli != nullptr);
    }

    std::vector<CLI_UINT> data(BYTES_TO_CLI_UINTS(minSize));
    config->cliBuffer = data.data();
    config->cliBufferSize = (uint16_t) data.size() * CLI_UINT_SIZE;
    EmbeddedCli *cli = embeddedCliNew(config);

    REQUIRE(cli != nullptr);

    CliTestRunner runner(cli);

    runner.runTests();

    embeddedCliFree(cli);
}


TEST_CASE("EmbeddedCli. Disabled autocomplete", "[cli]") {
    EmbeddedCliConfig *config = embeddedCliDefaultConfig();
    config->enableAutoComplete = false;
    EmbeddedCli *cli = embeddedCliNew(config);

    REQUIRE(cli != nullptr);

    CliTestRunner runner(cli);

    runner.testAutocompleteDisabled();

    embeddedCliFree(cli);
}

TEST_CASE("EmbeddedCli. Invitation is changed", "[cli]") {
    EmbeddedCliConfig *config = embeddedCliDefaultConfig();
    const std::string invitation = "test";
    config->invitation = (char*) malloc(invitation.length() + 1);
    strcpy(config->invitation, invitation.c_str());
    EmbeddedCli *cli = embeddedCliNew(config);

    REQUIRE(cli != nullptr);

    CliTestRunner runner(cli);

    runner.testInvitationChanged(invitation);

    free(config->invitation);
    embeddedCliFree(cli);
}
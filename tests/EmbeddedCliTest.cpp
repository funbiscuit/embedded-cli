#include "embedded_cli.h"
#include "CliMock.h"
#include "CliTestRunner.h"

#include <catch.hpp>

TEST_CASE("EmbeddedCli", "[cli]") {
    EmbeddedCli *cli = embeddedCliNewDefault();

    REQUIRE(cli != nullptr);

    CliTestRunner runner(cli);

    runner.runTests();

    embeddedCliFree(cli);
}

TEST_CASE("EmbeddedCli. Static allocation", "[cli]") {
    EmbeddedCliConfig *config = embeddedCliDefaultConfig();
    size_t minSize = embeddedCliRequiredSize(config);
    REQUIRE(minSize > 0);

    SECTION("Test creation") {
        INFO("Can't create CLI with small buffer")
        for (size_t size = 1; size < minSize; ++size) {
            std::vector<uint32_t> data(size/4+1);
            config->cliBuffer32 = data.data();
            config->cliBufferSize = size;
            REQUIRE(embeddedCliNew(config) == nullptr);
        }
        INFO("Can create CLI with buffer of minimal size")
        std::vector<uint32_t> data(minSize/4);
        config->cliBuffer32 = data.data();
        config->cliBufferSize = minSize;
        EmbeddedCli *cli = embeddedCliNew(config);

        REQUIRE(cli != nullptr);
    }

    std::vector<uint32_t> data(minSize/4);
    config->cliBuffer32 = data.data();
    config->cliBufferSize = minSize;
    EmbeddedCli *cli = embeddedCliNew(config);

    REQUIRE(cli != nullptr);

    CliTestRunner runner(cli);

    runner.runTests();

    embeddedCliFree(cli);
}

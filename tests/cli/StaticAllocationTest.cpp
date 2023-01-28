#include "CliWrapper.h"
#include "CliBuilder.h"

#include <catch2/catch_test_macros.hpp>


TEST_CASE("CLI. Static allocation", "[cli]") {
    EmbeddedCliConfig *config = embeddedCliDefaultConfig();
    uint16_t minSize = embeddedCliRequiredSize(config);
    REQUIRE(minSize > 0);

    SECTION("Can't create with small buffer") {
        for (uint16_t uintCount = 1; uintCount < (uint16_t) BYTES_TO_CLI_UINTS(minSize); ++uintCount) {
            std::vector<CLI_UINT> data(uintCount);
            config->cliBuffer = data.data();
            config->cliBufferSize = uintCount * CLI_UINT_SIZE;
            REQUIRE(embeddedCliNew(config) == nullptr);
        }
    }

    SECTION("Successful with minimal size") {
        CliWrapper cli = CliBuilder()
                .staticAllocation()
                .build();

        cli.sendLine("get led");
        cli.process();

        auto &commands = cli.getReceivedCommands();

        REQUIRE(commands.size() == 1);
        REQUIRE(commands.back().name == "get");
        REQUIRE(commands.back().args.size() == 1);
        REQUIRE(commands.back().args[0] == "led");
    }
}

#include "CliWrapper.h"
#include "CliBuilder.h"

#include <catch2/catch_test_macros.hpp>


TEST_CASE("CLI. Help", "[cli]") {
    CliWrapper cli = CliBuilder().build();

    auto &commands = cli.getReceivedCommands();

    SECTION("Calling help with bindings") {
        cli.addBinding("get", "Get specific parameter");
        cli.addBinding("set", "Set specific parameter");

        cli.sendLine("help");
        cli.process();

        REQUIRE(commands.empty());
        REQUIRE(cli.getRawOutput().find("get") != std::string::npos);
        REQUIRE(cli.getRawOutput().find("Get specific parameter") != std::string::npos);
        REQUIRE(cli.getRawOutput().find("set") != std::string::npos);
        REQUIRE(cli.getRawOutput().find("Set specific parameter") != std::string::npos);
    }

    SECTION("Calling help for known command") {
        cli.addBinding("get", "Get specific parameter");
        cli.addBinding("set", "Set specific parameter");

        cli.sendLine("help get");
        cli.process();

        REQUIRE(commands.empty());
        REQUIRE(cli.getRawOutput().find("get") != std::string::npos);
        REQUIRE(cli.getRawOutput().find("Get specific parameter") != std::string::npos);
        REQUIRE(cli.getRawOutput().find("set") == std::string::npos);
        REQUIRE(cli.getRawOutput().find("Set specific parameter") == std::string::npos);
    }

    SECTION("Calling help for unknown command") {
        cli.addBinding("set", "Set specific parameter");

        cli.sendLine("help get");
        cli.process();

        REQUIRE(commands.empty());
        REQUIRE(cli.getRawOutput().find("get") != std::string::npos);
        REQUIRE(cli.getRawOutput().find("Unknown") != std::string::npos);
    }

    SECTION("Calling help for command without help") {
        cli.addBinding("get");

        cli.sendLine("help get");
        cli.process();

        REQUIRE(commands.empty());
        REQUIRE(cli.getRawOutput().find("get") != std::string::npos);
        REQUIRE(cli.getRawOutput().find("Help is not available") != std::string::npos);
    }

    SECTION("Calling help with multiple arguments") {
        cli.addBinding("get");

        cli.sendLine("help get set");
        cli.process();

        auto displayed = cli.getDisplay();

        REQUIRE(commands.empty());
        REQUIRE(displayed.lines.size() == 3);
        REQUIRE(displayed.lines[0] == "> help get set");
        REQUIRE(displayed.lines[1] == "Command \"help\" receives one or zero arguments");
        REQUIRE(displayed.lines[2] == ">");
        REQUIRE(displayed.cursorColumn == 2);
    }
}

#include "CliWrapper.h"
#include "CliBuilder.h"

#include <catch2/catch_test_macros.hpp>


TEST_CASE("CLI. History", "[cli]") {
    CliWrapper cli = CliBuilder().build();

    auto cmdUp = "\x1B[A";
    auto cmdDown = "\x1B[B";

    SECTION("When history is present, can navigate") {
        std::vector<std::string> cmds = {"command", "get", "reset", "exit"};
        for (auto &cmd: cmds) {
            cli.sendLine(cmd);
        }
        cli.process();

        for (size_t i = 0; i < cmds.size(); ++i) {
            cli.send(cmdUp);
            cli.process();
            REQUIRE(cli.getDisplay().lines.back() == ("> " + cmds[cmds.size() - i - 1]));
        }

        for (size_t i = 1; i < cmds.size(); ++i) {
            cli.send(cmdDown);
            cli.process();
            REQUIRE(cli.getDisplay().lines.back() == ("> " + cmds[i]));
        }

        cli.send(cmdDown);
        cli.process();
        REQUIRE(cli.getDisplay().lines.back() == ">");
    }

    SECTION("Sending command multiple times, doesn't create duplicates") {
        std::vector<std::string> cmds = {"command", "get", "command", "command"};
        for (auto &cmd: cmds) {
            cli.sendLine(cmd);
        }
        cli.process();

        cli.send(cmdUp);
        cli.process();
        REQUIRE(cli.getDisplay().lines.back() == "> command");

        cli.send(cmdUp);
        cli.process();
        REQUIRE(cli.getDisplay().lines.back() == "> get");

        cli.send(cmdUp);
        cli.process();
        REQUIRE(cli.getDisplay().lines.back() == "> get");

        cli.send(cmdDown);
        cli.process();
        REQUIRE(cli.getDisplay().lines.back() == "> command");

        cli.send(cmdDown);
        cli.process();
        REQUIRE(cli.getDisplay().lines.back() == ">");
    }

    SECTION("Sending command resets history cursor") {
        std::vector<std::string> cmds = {"command", "get"};
        for (auto &cmd: cmds) {
            cli.sendLine(cmd);
        }
        cli.process();

        cli.send(cmdUp);
        cli.send(cmdUp);
        cli.sendLine("");
        cli.send(cmdUp);

        cli.process();
        REQUIRE(cli.getDisplay().lines.back() == "> command");

        cli.send(cmdUp);
        cli.process();
        REQUIRE(cli.getDisplay().lines.back() == "> get");
    }

    SECTION("Arguments are preserved in history") {
        cli.sendLine("get param 2");
        cli.send(cmdUp);

        cli.process();
        REQUIRE(cli.getDisplay().lines.back() == "> get param 2");
    }
}

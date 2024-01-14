#include "CliWrapper.h"
#include "CliBuilder.h"

#include <catch2/catch_test_macros.hpp>
#include <algorithm>


TEST_CASE("CLI. Base tests", "[cli]") {
    CliWrapper cli = CliBuilder().build();

    auto &commands = cli.getReceivedCommands();

    SECTION("Initial display") {
        cli.process();
        auto display = cli.getDisplay();

        REQUIRE(display.lines.size() == 1);
        REQUIRE(display.lines[0] == ">");
        // there is a space between '>' and cursor
        REQUIRE(display.cursorColumn == 2);
    }

    SECTION("Single command") {
        for (size_t i = 0; i < 50; ++i) {
            cli.sendLine("set led 1 " + std::to_string(i));
            cli.process();

            REQUIRE(commands.size() == i + 1);
            REQUIRE(commands.back().name == "set");
            REQUIRE(commands.back().args.size() == 1);
            REQUIRE(commands.back().args[0] == ("led 1 " + std::to_string(i)));
        }
    }

    SECTION("Sending by parts") {
        cli.send("set ");
        cli.process();
        REQUIRE(commands.empty());

        cli.send("led 1");
        cli.process();
        REQUIRE(commands.empty());

        cli.sendLine(" 1");
        cli.process();
        REQUIRE(!commands.empty());

        REQUIRE(commands.back().name == "set");
        REQUIRE(commands.back().args.size() == 1);
        REQUIRE(commands.back().args[0] == "led 1 1");
    }

    SECTION("Multiple commands") {
        for (int i = 0; i < 3; ++i) {
            cli.sendLine("set led 1 " + std::to_string(i));
        }
        cli.process();

        REQUIRE(commands.size() == 3);
        for (size_t i = 0; i < 3; ++i) {
            REQUIRE(commands[i].name == "set");
            REQUIRE(commands.back().args.size() == 1);
            REQUIRE(commands[i].args[0] == ("led 1 " + std::to_string(i)));
        }
    }

    SECTION("Buffer overflow recovery") {
        for (int i = 0; i < 100; ++i) {
            cli.sendLine("set led 1 " + std::to_string(i));
        }
        cli.process();
        REQUIRE(commands.size() < 100);
        commands.clear();

        cli.sendLine("set led 1 150");
        cli.process();

        REQUIRE(commands.size() == 1);
        REQUIRE(commands.back().name == "set");
        REQUIRE(commands.back().args.size() == 1);
        REQUIRE(commands.back().args[0] == "led 1 150");
    }

    SECTION("Removing some chars") {
        cli.sendLine("s\bget led\b\b\bjack 1\b56\b");
        cli.process();

        auto lines = cli.getDisplay().lines;

        REQUIRE(lines.size() == 2);
        REQUIRE(lines[0] == "> get jack 5");
        REQUIRE(lines[1] == ">");

        REQUIRE(commands.back().name == "get");
        REQUIRE(commands.back().args.size() == 1);
        REQUIRE(commands.back().args[0] == "jack 5");
    }

    SECTION("Removing all chars") {
        cli.sendLine("set\b\b\b\b\bget led");
        cli.process();

        auto lines = cli.getDisplay().lines;

        REQUIRE(lines.size() == 2);
        REQUIRE(lines[0] == "> get led");
        REQUIRE(lines[1] == ">");

        REQUIRE(commands.back().name == "get");
        REQUIRE(commands.back().args.size() == 1);
        REQUIRE(commands.back().args[0] == "led");
    }

    SECTION("Move cursor left") {
        cli.send("get lft\x1B[D\x1B[De");
        cli.process();

        auto displayed = cli.getDisplay();

        REQUIRE(displayed.lines.size() == 1);
        REQUIRE(displayed.lines[0] == "> get left");
        REQUIRE(displayed.cursorColumn == 8);
    }

    SECTION("Move cursor left and remove some chars") {
        cli.send("test");
        cli.send("\x1B[D\x1B[D"); // Move left two characters
        cli.send("\b\b\b\b\b"); // Try to delete more characters than remaining
        cli.send("almo");
        cli.process();

        auto displayed = cli.getDisplay();

        REQUIRE(displayed.lines.size() == 1);
        REQUIRE(displayed.lines[0] == "> almost");
    }

    SECTION("Move cursor right") {
        cli.send("get right\x1B[C\x1B[C");
        cli.process();

        auto displayed = cli.getDisplay();

        REQUIRE(displayed.lines.size() == 1);
        REQUIRE(displayed.lines[0] == "> get right");
        REQUIRE(displayed.cursorColumn == 11);
    }

    SECTION("Move cursor left then right") {
        cli.send("ge oth\x1B[D\x1B[D\x1B[D\x1B[Dt\x1B[Cb");
        cli.process();

        auto displayed = cli.getDisplay();

        REQUIRE(displayed.lines.size() == 1);
        REQUIRE(displayed.lines[0] == "> get both");
        REQUIRE(displayed.cursorColumn == 7);
    }

    SECTION("Command that is too long") {
        size_t cmdMax = embeddedCliDefaultConfig()->cmdBufferSize;
        std::string cmdMaxTest = std::string(cmdMax/2, 'x');

        // Split command into two to prevent getting the fifo full
        cli.send(cmdMaxTest);
        cli.process();
        cli.sendLine(cmdMaxTest);
        cli.process();

        auto displayed = cli.getDisplay();

        std::string cliRawOutput = cli.getRawOutput();

        auto xcount = std::count(cliRawOutput.begin(), cliRawOutput.end(), 'x');

        REQUIRE(displayed.lines.size() == 2);
        // We are only displaying (cmdMax - 2) 'x's, 
        // but two additional characters are the invitation and the space
        REQUIRE(displayed.lines[0].size() == cmdMax); 
        // Check that the two extra x's will be dropped 
        REQUIRE(xcount == cmdMax - 2);
    }

    SECTION("Unknown command") {
        // unknown commands are only possible when onCommand callback is not set
        cli.raw()->onCommand = nullptr;
        cli.sendLine("get led");
        cli.process();

        REQUIRE(cli.getRawOutput().find("Unknown command") != std::string::npos);
    }

    SECTION("Bindings") {
        SECTION("Command without binding") {
            embeddedCliAddBinding(cli.raw(), {
                    .name = "get",
                    .help = nullptr,
                    .tokenizeArgs = false,
                    .context = nullptr,
                    .binding = nullptr
            });

            cli.sendLine("get led");
            cli.process();

            REQUIRE(!commands.empty());
        }

        SECTION("Command with binding") {
            cli.addBinding("get");

            cli.sendLine("get led");
            cli.process();

            REQUIRE(commands.empty());
            auto &cmds = cli.getCalledBindings();
            REQUIRE_FALSE(cmds.empty());
            REQUIRE(cmds.back().name == "get");
            REQUIRE(cmds.back().args.size() == 1);
            REQUIRE(cmds.back().args[0] == "led");
        }
    }

    SECTION("Escape sequences") {
        SECTION("Escape sequences don't show up in output") {
            cli.send("t\x1B[Ae\x1B[10As\x1B[Bt\x1B[D\x1B[C1");
            cli.process();

            auto displayed = cli.getDisplay();
            REQUIRE(displayed.lines.size() == 1);
            REQUIRE(displayed.lines[0] == "> test1");
            REQUIRE(displayed.cursorColumn == 7);
        }
    }
}

TEST_CASE("CLI. Changed invitation", "[cli]") {
    CliWrapper cli = CliBuilder()
            .invitation("inv ")
            .build();

    cli.process();
    auto display = cli.getDisplay();

    REQUIRE(display.lines.size() == 1);
    REQUIRE(display.lines[0] == "inv");
    REQUIRE(display.cursorColumn == 4);

    cli.send("str");
    cli.process();

    display = cli.getDisplay();

    REQUIRE(display.lines.size() == 1);
    REQUIRE(display.lines[0] == "inv str");
    REQUIRE(display.cursorColumn == 7);

    cli.sendLine("");
    cli.process();


    display = cli.getDisplay();

    REQUIRE(display.lines.size() == 2);
    REQUIRE(display.lines[0] == "inv str");
    REQUIRE(display.lines[1] == "inv");
    REQUIRE(display.cursorColumn == 4);
}

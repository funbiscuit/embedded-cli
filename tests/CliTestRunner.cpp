
#include <catch.hpp>

#include "CliTestRunner.h"

CliTestRunner::CliTestRunner(EmbeddedCli *cli) : cli(cli), mock(cli) {

}

void CliTestRunner::runTests() {
    embeddedCliProcess(cli);

    SECTION("Test base functionality") {
        testBase();
    }

    SECTION("Test printing") {
        testPrinting();
    }

    SECTION("History") {
        testHistory();
    }

    SECTION("Help command handling") {
        testHelp();
    }

    SECTION("Autocomplete") {
        testAutocomplete();
    }

    SECTION("Escape sequences") {
        SECTION("Escape sequences don't show up in output") {

            mock.sendStr("t\x1B[Ae\x1B[10As\x1B[Bt\x1B[C\x1B[D1");

            embeddedCliProcess(cli);

            size_t cursor = 0;
            auto lines = mock.getLines(true, &cursor);
            REQUIRE(lines.size() == 1);
            REQUIRE(lines[0] == "> test1");
            REQUIRE(cursor == 7);
        }
    }
}

void CliTestRunner::testBase() {
    auto &commands = mock.getReceivedCommands();

    SECTION("Test single command") {
        for (int i = 0; i < 50; ++i) {
            mock.sendLine("set led 1 " + std::to_string(i));

            embeddedCliProcess(cli);

            REQUIRE(commands.size() == i + 1);
            REQUIRE(commands.back().name == "set");
            REQUIRE(commands.back().args == ("led 1 " + std::to_string(i)));
        }
    }

    SECTION("Test sending by parts") {
        mock.sendStr("set ");
        embeddedCliProcess(cli);
        REQUIRE(commands.empty());

        mock.sendStr("led 1");
        embeddedCliProcess(cli);
        REQUIRE(commands.empty());

        mock.sendLine(" 1");
        embeddedCliProcess(cli);
        REQUIRE(!commands.empty());

        REQUIRE(commands.back().name == "set");
        REQUIRE(commands.back().args == "led 1 1");
    }

    SECTION("Test sending multiple commands") {
        for (int i = 0; i < 3; ++i) {
            mock.sendLine("set led 1 " + std::to_string(i));
        }
        embeddedCliProcess(cli);

        REQUIRE(commands.size() == 3);
        for (int i = 0; i < 3; ++i) {
            REQUIRE(commands[i].name == "set");
            REQUIRE(commands[i].args == ("led 1 " + std::to_string(i)));
        }
    }

    SECTION("Test buffer overflow recovery") {
        for (int i = 0; i < 100; ++i) {
            mock.sendLine("set led 1 " + std::to_string(i));
        }
        embeddedCliProcess(cli);
        REQUIRE(commands.size() < 100);
        commands.clear();

        mock.sendLine("set led 1 150");
        embeddedCliProcess(cli);
        REQUIRE(commands.size() == 1);
        REQUIRE(commands.back().name == "set");
        REQUIRE(commands.back().args == "led 1 150");
    }

    SECTION("Test removing some chars") {
        mock.sendLine("s\bget led\b\b\bjack 1\b56\b");

        embeddedCliProcess(cli);

        REQUIRE(commands.back().name == "get");
        REQUIRE(commands.back().args == "jack 5");
    }

    SECTION("Test removing all chars") {
        mock.sendLine("set\b\b\b\b\bget led");

        embeddedCliProcess(cli);

        auto lines = mock.getLines(false);

        REQUIRE(lines.size() == 2);
        REQUIRE(lines[0] == "> get led");
        REQUIRE(lines[1] == "> ");

        REQUIRE(commands.back().name == "get");
        REQUIRE(commands.back().args == "led");
    }

    SECTION("Unknown command handling") {
        SECTION("Providing unknown command") {
            // unknown commands are only possible when onCommand callback is not set
            cli->onCommand = nullptr;
            mock.sendLine("get led");

            embeddedCliProcess(cli);

            REQUIRE(mock.getRawOutput().find("Unknown command") != std::string::npos);
        }

        SECTION("Providing known command without binding") {
            embeddedCliAddBinding(cli, {
                    "get",
                    nullptr,
                    false,
                    nullptr,
                    nullptr
            });

            mock.sendLine("get led");

            embeddedCliProcess(cli);

            REQUIRE(!commands.empty());
        }

        SECTION("Providing known command with binding") {
            mock.addCommandBinding("get");

            mock.sendLine("get led");

            embeddedCliProcess(cli);

            REQUIRE(commands.empty());
            auto &cmds = mock.getReceivedKnownCommands();
            REQUIRE_FALSE(cmds.empty());
            REQUIRE(cmds.back().name == "get");
            REQUIRE(cmds.back().args == "led");
        }
    }
}

void CliTestRunner::testHistory() {
    auto cmdUp = "\x1B[A";
    auto cmdDown = "\x1B[B";

    SECTION("When history is present, can navigate") {
        std::vector<std::string> cmds = {"command", "get", "reset", "exit"};
        for (auto &cmd : cmds) {
            mock.sendLine(cmd);
        }
        embeddedCliProcess(cli);

        for (int i = 0; i < cmds.size(); ++i) {
            mock.sendStr(cmdUp);
            embeddedCliProcess(cli);
            REQUIRE(mock.getLines().back() == ("> " + cmds[cmds.size() - i - 1]));
        }

        for (int i = 1; i < cmds.size(); ++i) {
            mock.sendStr(cmdDown);
            embeddedCliProcess(cli);
            REQUIRE(mock.getLines().back() == ("> " + cmds[i]));
        }

        mock.sendStr(cmdDown);
        embeddedCliProcess(cli);
        REQUIRE(mock.getLines().back() == ">");
    }

    SECTION("Sending command multiple times, doesn't create duplicates") {
        std::vector<std::string> cmds = {"command", "get", "command", "command"};
        for (auto &cmd : cmds) {
            mock.sendLine(cmd);
        }
        embeddedCliProcess(cli);

        mock.sendStr(cmdUp);
        embeddedCliProcess(cli);
        REQUIRE(mock.getLines().back() == "> command");

        mock.sendStr(cmdUp);
        embeddedCliProcess(cli);
        REQUIRE(mock.getLines().back() == "> get");

        mock.sendStr(cmdUp);
        embeddedCliProcess(cli);
        REQUIRE(mock.getLines().back() == "> get");

        mock.sendStr(cmdDown);
        embeddedCliProcess(cli);
        REQUIRE(mock.getLines().back() == "> command");

        mock.sendStr(cmdDown);
        embeddedCliProcess(cli);
        REQUIRE(mock.getLines().back() == ">");
    }

    SECTION("Sending command resets history cursor") {
        std::vector<std::string> cmds = {"command", "get"};
        for (auto &cmd : cmds) {
            mock.sendLine(cmd);
        }
        embeddedCliProcess(cli);

        mock.sendStr(cmdUp);
        mock.sendStr(cmdUp);
        mock.sendLine("");
        mock.sendStr(cmdUp);

        embeddedCliProcess(cli);
        REQUIRE(mock.getLines().back() == "> command");

        mock.sendStr(cmdUp);
        embeddedCliProcess(cli);
        REQUIRE(mock.getLines().back() == "> get");
    }

    SECTION("Arguments are preserved in history") {
        mock.sendLine("get param 2");
        mock.sendStr(cmdUp);

        embeddedCliProcess(cli);
        REQUIRE(mock.getLines().back() == "> get param 2");
    }
}

void CliTestRunner::testAutocomplete() {
    mock.addCommandBinding("get");
    mock.addCommandBinding("set");
    mock.addCommandBinding("get-new");
    mock.addCommandBinding("reset-first");
    mock.addCommandBinding("reset-second");

    SECTION("Autocomplete when single candidate") {
        mock.sendStr("s\t");

        embeddedCliProcess(cli);

        size_t cursor = 0;
        auto lines = mock.getLines(true, &cursor);

        REQUIRE(lines.size() == 1);
        REQUIRE(lines[0] == "> set");
        REQUIRE(cursor == 6);
    }

    SECTION("Submit autocompleted command") {
        mock.sendLine("s\t");

        embeddedCliProcess(cli);

        REQUIRE(!mock.getReceivedKnownCommands().empty());
        REQUIRE(mock.getReceivedKnownCommands().back().name == "set");
    }

    SECTION("Submit autocompleted command when multiple candidates") {
        mock.sendLine("g\t");

        embeddedCliProcess(cli);

        REQUIRE(!mock.getReceivedKnownCommands().empty());
        REQUIRE(mock.getReceivedKnownCommands().back().name == "get");
    }

    SECTION("Autocomplete help command") {
        mock.sendStr("h\t");

        embeddedCliProcess(cli);

        size_t cursor = 0;
        auto lines = mock.getLines(true, &cursor);

        REQUIRE(lines.size() == 1);
        REQUIRE(lines[0] == "> help");
        REQUIRE(cursor == 7);
    }

    SECTION("Autocomplete when multiple candidates with common prefix") {
        mock.sendStr("g\t");

        embeddedCliProcess(cli);

        size_t cursor = 0;
        auto lines = mock.getLines(true, &cursor);

        REQUIRE(lines.size() == 1);
        REQUIRE(lines[0] == "> get");
        REQUIRE(cursor == 5);
    }

    SECTION("Autocomplete when multiple candidates with common prefix and not common suffix") {
        mock.sendStr("r\t");

        embeddedCliProcess(cli);

        size_t cursor = 0;
        auto lines = mock.getLines(true, &cursor);

        REQUIRE(lines.size() == 1);
        REQUIRE(lines[0] == "> reset-");
        REQUIRE(cursor == 8);
    }

    SECTION("Autocomplete when multiple candidates and one is help") {
        mock.addCommandBinding("hello");

        mock.sendStr("hel\t");

        embeddedCliProcess(cli);

        size_t cursor = 0;
        auto lines = mock.getLines(true, &cursor);

        REQUIRE(lines.size() == 3);
        REQUIRE(lines[0] == "help");
        REQUIRE(lines[1] == "hello");
        REQUIRE(lines[2] == "> hel");
        REQUIRE(cursor == 5);
    }

    SECTION("Autocomplete when multiple candidates without common prefix") {
        mock.sendStr("get\t");

        embeddedCliProcess(cli);

        size_t cursor = 0;
        auto lines = mock.getLines(true, &cursor);

        REQUIRE(lines.size() == 3);
        REQUIRE(lines[0] == "get");
        REQUIRE(lines[1] == "get-new");
        REQUIRE(lines[2] == "> get");
        REQUIRE(cursor == 5);
    }

    SECTION("Autocomplete when no candidates") {
        mock.sendStr("m\t");

        embeddedCliProcess(cli);

        size_t cursor = 0;
        auto lines = mock.getLines(true, &cursor);

        REQUIRE(lines.size() == 1);
        REQUIRE(lines[0] == "> m");
        REQUIRE(cursor == 3);
    }

    SECTION("Live autocomplete when no candidates") {
        mock.sendStr("m");

        embeddedCliProcess(cli);

        size_t cursor = 0;
        auto lines = mock.getLines(true, &cursor);

        REQUIRE(lines.size() == 1);
        REQUIRE(lines[0] == "> m");
        REQUIRE(cursor == 3);
    }

    SECTION("Live autocomplete when more then one candidate") {
        mock.sendStr("r");

        embeddedCliProcess(cli);

        size_t cursor = 0;
        auto lines = mock.getLines(true, &cursor);

        REQUIRE(lines.size() == 1);
        REQUIRE(lines[0] == "> reset-");
        REQUIRE(cursor == 3);
    }

    SECTION("Live autocomplete one candidate") {
        mock.sendStr("s");

        embeddedCliProcess(cli);

        size_t cursor = 0;
        auto lines = mock.getLines(true, &cursor);

        REQUIRE(lines.size() == 1);
        REQUIRE(lines[0] == "> set");
        REQUIRE(cursor == 3);
    }

    SECTION("Live autocomplete when input changed to longer command") {
        mock.sendStr("s");

        embeddedCliProcess(cli);

        mock.sendStr("\br");

        embeddedCliProcess(cli);

        size_t cursor = 0;
        auto lines = mock.getLines(true, &cursor);

        REQUIRE(lines.size() == 1);
        REQUIRE(lines[0] == "> reset-");
        REQUIRE(cursor == 3);
    }

    SECTION("Live autocomplete when input changed to shorter command") {
        mock.sendStr("r");

        embeddedCliProcess(cli);

        mock.sendStr("\bs");

        embeddedCliProcess(cli);

        size_t cursor = 0;
        auto lines = mock.getLines(true, &cursor);

        REQUIRE(lines.size() == 1);
        REQUIRE(lines[0] == "> set");
        REQUIRE(cursor == 3);
    }

    SECTION("Live autocomplete when input changed to no autocompletion") {
        mock.sendStr("r");

        embeddedCliProcess(cli);

        mock.sendStr("\bm");

        embeddedCliProcess(cli);

        size_t cursor = 0;
        auto lines = mock.getLines(true, &cursor);

        REQUIRE(lines.size() == 1);
        REQUIRE(lines[0] == "> m");
        REQUIRE(cursor == 3);
    }

    SECTION("Submit command with live autocompletion submits full command") {
        mock.sendLine("s");

        embeddedCliProcess(cli);

        REQUIRE(mock.getReceivedCommands().empty());
        REQUIRE(!mock.getReceivedKnownCommands().empty());
        REQUIRE(mock.getReceivedKnownCommands().back().name == "set");
    }
}

void CliTestRunner::testPrinting() {
    SECTION("Print with no command input") {
        embeddedCliPrint(cli, "test print");

        size_t cursor = 0;
        auto lines = mock.getLines(true, &cursor);

        REQUIRE(lines.size() == 2);
        REQUIRE(lines[0] == "test print");
        REQUIRE(lines[1] == ">"); // space is trimmed
        INFO("Cursor at the end of invitation line")
        REQUIRE(cursor == 2);
    }

    SECTION("Print with intermediate command") {
        std::string cmd = "some cmd";
        mock.sendStr(cmd);

        embeddedCliProcess(cli);

        embeddedCliPrint(cli, "print");

        size_t cursor = 0;
        auto lines = mock.getLines(true, &cursor);

        REQUIRE(lines.size() == 2);
        REQUIRE(lines[0] == "print");
        REQUIRE(lines[1] == "> " + cmd);
        INFO("Cursor at the end of invitation line")
        REQUIRE(cursor == 2 + cmd.length());
    }

    SECTION("Print with live autocompletion") {
        mock.addCommandBinding("get");
        mock.sendStr("g");

        embeddedCliProcess(cli);

        embeddedCliPrint(cli, "print");

        size_t cursor = 0;
        auto lines = mock.getLines(true, &cursor);

        REQUIRE(lines.size() == 2);
        REQUIRE(lines[0] == "print");
        REQUIRE(lines[1] == "> get");
        REQUIRE(cursor == 3);
    }

    SECTION("Print with live autocompletion when printed text is short") {
        mock.addCommandBinding("get");
        mock.sendStr("g");

        embeddedCliProcess(cli);

        embeddedCliPrint(cli, "j");

        size_t cursor = 0;
        auto lines = mock.getLines(true, &cursor);

        REQUIRE(lines.size() == 2);
        REQUIRE(lines[0] == "j");
        REQUIRE(lines[1] == "> get");
        REQUIRE(cursor == 3);
    }
}

void CliTestRunner::testHelp() {
    auto &commands = mock.getReceivedCommands();

    SECTION("Calling help with bindings") {
        mock.addCommandBinding("get", "Get specific parameter");
        mock.addCommandBinding("set", "Set specific parameter");

        mock.sendLine("help");

        embeddedCliProcess(cli);

        REQUIRE(commands.empty());
        REQUIRE(mock.getRawOutput().find("get") != std::string::npos);
        REQUIRE(mock.getRawOutput().find("Get specific parameter") != std::string::npos);
        REQUIRE(mock.getRawOutput().find("set") != std::string::npos);
        REQUIRE(mock.getRawOutput().find("Set specific parameter") != std::string::npos);
    }

    SECTION("Calling help for known command") {
        mock.addCommandBinding("get", "Get specific parameter");
        mock.addCommandBinding("set", "Set specific parameter");

        mock.sendLine("help get");

        embeddedCliProcess(cli);

        REQUIRE(commands.empty());
        REQUIRE(mock.getRawOutput().find("get") != std::string::npos);
        REQUIRE(mock.getRawOutput().find("Get specific parameter") != std::string::npos);
        REQUIRE(mock.getRawOutput().find("set") == std::string::npos);
        REQUIRE(mock.getRawOutput().find("Set specific parameter") == std::string::npos);
    }

    SECTION("Calling help for unknown command") {
        mock.addCommandBinding("set", "Set specific parameter");

        mock.sendLine("help get");

        embeddedCliProcess(cli);

        REQUIRE(commands.empty());
        REQUIRE(mock.getRawOutput().find("get") != std::string::npos);
        REQUIRE(mock.getRawOutput().find("Unknown") != std::string::npos);
    }

    SECTION("Calling help for command without help") {
        mock.addCommandBinding("get");

        mock.sendLine("help get");

        embeddedCliProcess(cli);

        REQUIRE(commands.empty());
        REQUIRE(mock.getRawOutput().find("get") != std::string::npos);
        REQUIRE(mock.getRawOutput().find("Help is not available") != std::string::npos);
    }

    SECTION("Calling help with multiple arguments") {
        mock.addCommandBinding("get");

        mock.sendLine("help get set");

        embeddedCliProcess(cli);

        auto lines = mock.getLines(false);

        REQUIRE(commands.empty());
        REQUIRE(lines.size() == 3);
        REQUIRE(lines[0] == "> help get set");
        REQUIRE(lines[1] == "Command \"help\" receives one or zero arguments");
        REQUIRE(lines[2] == "> ");
    }
}

#include "CliWrapper.h"
#include "CliBuilder.h"

#include <catch2/catch_test_macros.hpp>


TEST_CASE("CLI. Printing", "[cli]") {
    CliWrapper cli = CliBuilder().build();

    SECTION("Print with no command input") {
        cli.print("test print");

        auto display = cli.getDisplay();

        REQUIRE(display.lines.size() == 2);
        REQUIRE(display.lines[0] == "test print");
        REQUIRE(display.lines[1] == ">"); // space is trimmed
        INFO("Cursor at the end of invitation line");
        REQUIRE(display.cursorColumn == 2);
    }

    SECTION("Print with intermediate command") {
        std::string cmd = "some cmd";
        cli.send(cmd);
        cli.process();
        cli.print("print");

        auto displayed = cli.getDisplay();

        REQUIRE(displayed.lines.size() == 2);
        REQUIRE(displayed.lines[0] == "print");
        REQUIRE(displayed.lines[1] == "> " + cmd);
        INFO("Cursor at the end of invitation line");
        REQUIRE(displayed.cursorColumn == 2 + cmd.length());
    }

    SECTION("Print with live autocompletion") {
        cli.addBinding("get");
        cli.send("g");
        cli.process();
        cli.print("print");

        auto displayed = cli.getDisplay();

        REQUIRE(displayed.lines.size() == 2);
        REQUIRE(displayed.lines[0] == "print");
        REQUIRE(displayed.lines[1] == "> get");
        REQUIRE(displayed.cursorColumn == 3);
    }

    SECTION("Print with live autocompletion when printed text is short") {
        cli.addBinding("get");
        cli.send("g");
        cli.process();
        cli.print("j");

        auto displayed = cli.getDisplay();

        REQUIRE(displayed.lines.size() == 2);
        REQUIRE(displayed.lines[0] == "j");
        REQUIRE(displayed.lines[1] == "> get");
        REQUIRE(displayed.cursorColumn == 3);
    }
}

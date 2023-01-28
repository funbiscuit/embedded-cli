#include "CliWrapper.h"
#include "CliBuilder.h"

#include <catch2/catch_test_macros.hpp>


TEST_CASE("CLI. Autocomplete enabled", "[cli]") {
    CliWrapper cli = CliBuilder().build();

    cli.addBinding("get");
    cli.addBinding("set");
    cli.addBinding("get-new");
    cli.addBinding("reset-first");
    cli.addBinding("reset-second");

    SECTION("Autocomplete when single candidate") {
        cli.send("s\t");
        cli.process();

        auto displayed = cli.getDisplay();

        REQUIRE(displayed.lines.size() == 1);
        REQUIRE(displayed.lines[0] == "> set");
        REQUIRE(displayed.cursorColumn == 6);
    }

    SECTION("Submit autocompleted command") {
        cli.sendLine("s\t");
        cli.process();

        REQUIRE(!cli.getCalledBindings().empty());
        REQUIRE(cli.getCalledBindings().back().name == "set");
    }

    SECTION("Submit autocompleted command when multiple candidates") {
        cli.sendLine("g\t");
        cli.process();

        REQUIRE(!cli.getCalledBindings().empty());
        REQUIRE(cli.getCalledBindings().back().name == "get");
    }

    SECTION("Autocomplete help command") {
        cli.send("h\t");
        cli.process();

        auto displayed = cli.getDisplay();

        REQUIRE(displayed.lines.size() == 1);
        REQUIRE(displayed.lines[0] == "> help");
        REQUIRE(displayed.cursorColumn == 7);
    }

    SECTION("Autocomplete when multiple candidates with common prefix") {
        cli.send("g\t");
        cli.process();

        auto displayed = cli.getDisplay();

        REQUIRE(displayed.lines.size() == 1);
        REQUIRE(displayed.lines[0] == "> get");
        REQUIRE(displayed.cursorColumn == 5);
    }

    SECTION("Autocomplete when multiple candidates with common prefix and not common suffix") {
        cli.send("r\t");
        cli.process();

        auto displayed = cli.getDisplay();

        REQUIRE(displayed.lines.size() == 1);
        REQUIRE(displayed.lines[0] == "> reset-");
        REQUIRE(displayed.cursorColumn == 8);
    }

    SECTION("Autocomplete when multiple candidates and one is help") {
        cli.addBinding("hello");

        cli.send("hel\t");
        cli.process();

        auto displayed = cli.getDisplay();

        REQUIRE(displayed.lines.size() == 3);
        REQUIRE(displayed.lines[0] == "help");
        REQUIRE(displayed.lines[1] == "hello");
        REQUIRE(displayed.lines[2] == "> hel");
        REQUIRE(displayed.cursorColumn == 5);
    }

    SECTION("Autocomplete when multiple candidates without common prefix") {
        cli.send("get\t");
        cli.process();

        auto displayed = cli.getDisplay();

        REQUIRE(displayed.lines.size() == 3);
        REQUIRE(displayed.lines[0] == "get");
        REQUIRE(displayed.lines[1] == "get-new");
        REQUIRE(displayed.lines[2] == "> get");
        REQUIRE(displayed.cursorColumn == 5);
    }

    SECTION("Autocomplete when no candidates") {
        cli.send("m\t");
        cli.process();

        auto displayed = cli.getDisplay();

        REQUIRE(displayed.lines.size() == 1);
        REQUIRE(displayed.lines[0] == "> m");
        REQUIRE(displayed.cursorColumn == 3);
    }

    SECTION("Live autocomplete when no candidates") {
        cli.send("m");
        cli.process();

        auto displayed = cli.getDisplay();

        REQUIRE(displayed.lines.size() == 1);
        REQUIRE(displayed.lines[0] == "> m");
        REQUIRE(displayed.cursorColumn == 3);
    }

    SECTION("Live autocomplete when more then one candidate") {
        cli.send("r");
        cli.process();

        auto displayed = cli.getDisplay();

        REQUIRE(displayed.lines.size() == 1);
        REQUIRE(displayed.lines[0] == "> reset-");
        REQUIRE(displayed.cursorColumn == 3);
    }

    SECTION("Live autocomplete one candidate") {
        cli.send("s");
        cli.process();

        auto displayed = cli.getDisplay();

        REQUIRE(displayed.lines.size() == 1);
        REQUIRE(displayed.lines[0] == "> set");
        REQUIRE(displayed.cursorColumn == 3);
    }

    SECTION("Live autocomplete when input changed to longer command") {
        cli.send("s");
        cli.process();
        cli.send("\br");
        cli.process();

        auto displayed = cli.getDisplay();

        REQUIRE(displayed.lines.size() == 1);
        REQUIRE(displayed.lines[0] == "> reset-");
        REQUIRE(displayed.cursorColumn == 3);
    }

    SECTION("Live autocomplete when input changed to shorter command") {
        cli.send("r");
        cli.process();
        cli.send("\bs");
        cli.process();

        auto displayed = cli.getDisplay();

        REQUIRE(displayed.lines.size() == 1);
        REQUIRE(displayed.lines[0] == "> set");
        REQUIRE(displayed.cursorColumn == 3);
    }

    SECTION("Live autocomplete when input changed to no autocompletion") {
        cli.send("r");
        cli.process();
        cli.send("\bm");
        cli.process();

        auto displayed = cli.getDisplay();

        REQUIRE(displayed.lines.size() == 1);
        REQUIRE(displayed.lines[0] == "> m");
        REQUIRE(displayed.cursorColumn == 3);
    }

    SECTION("Submit command with live autocompletion submits full command") {
        cli.sendLine("s");
        cli.process();

        REQUIRE(cli.getReceivedCommands().empty());
        REQUIRE(!cli.getCalledBindings().empty());
        REQUIRE(cli.getCalledBindings().back().name == "set");
    }
}

TEST_CASE("CLI. Autocomplete disabled", "[cli]") {
    CliWrapper cli = CliBuilder()
            .autocomplete(false)
            .build();

    cli.addBinding("set");

    SECTION("Live autocomplete disabled") {
        cli.send("s");
        cli.process();

        auto displayed = cli.getDisplay();

        REQUIRE(displayed.lines.size() == 1);
        REQUIRE(displayed.lines[0] == "> s");
        REQUIRE(displayed.cursorColumn == 3);

        cli.send("\t");
        cli.process();

        displayed = cli.getDisplay();

        REQUIRE(displayed.lines.size() == 1);
        REQUIRE(displayed.lines[0] == "> set");
        REQUIRE(displayed.cursorColumn == 6);
    }
}
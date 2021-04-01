#include "embedded_cli.h"
#include "CliMock.h"

#include <catch.hpp>
#include <cstring>

TEST_CASE("EmbeddedCli", "[cli]") {
    EmbeddedCli *cli = embeddedCliNew();

    REQUIRE(cli != nullptr);

    CliMock mock(cli);
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

}

TEST_CASE("EmbeddedCli. Tokens", "[cli][token]") {
    const int argsBufLen = 30;

    char args[argsBufLen];
    memset(args, '!', argsBufLen);
    args[argsBufLen - 1] = '\0';

    SECTION("Tokenize simple string") {
        strcpy_s(args, argsBufLen, "a b c");
        embeddedCliTokenizeArgs(args);

        REQUIRE(args[0] == 'a');
        REQUIRE(args[1] == '\0');
        REQUIRE(args[2] == 'b');
        REQUIRE(args[3] == '\0');
        REQUIRE(args[4] == 'c');
        REQUIRE(args[5] == '\0');
        REQUIRE(args[6] == '\0');
    }

    SECTION("Tokenize string with duplicating separators") {
        strcpy_s(args, argsBufLen, "   a  b    c   ");
        embeddedCliTokenizeArgs(args);

        REQUIRE(args[0] == 'a');
        REQUIRE(args[1] == '\0');
        REQUIRE(args[2] == 'b');
        REQUIRE(args[3] == '\0');
        REQUIRE(args[4] == 'c');
        REQUIRE(args[5] == '\0');
        REQUIRE(args[6] == '\0');
    }

    SECTION("Tokenize string with long tokens") {
        strcpy_s(args, argsBufLen, "abcd ef");
        embeddedCliTokenizeArgs(args);

        REQUIRE(args[0] == 'a');
        REQUIRE(args[1] == 'b');
        REQUIRE(args[2] == 'c');
        REQUIRE(args[3] == 'd');
        REQUIRE(args[4] == '\0');
        REQUIRE(args[5] == 'e');
        REQUIRE(args[6] == 'f');
        REQUIRE(args[7] == '\0');
        REQUIRE(args[8] == '\0');
    }

    SECTION("Tokenize string of separators") {
        strcpy_s(args, argsBufLen, "      ");
        embeddedCliTokenizeArgs(args);

        REQUIRE(args[0] == '\0');
        REQUIRE(args[1] == '\0');
    }

    SECTION("Tokenize empty string") {
        strcpy_s(args, argsBufLen, "");
        embeddedCliTokenizeArgs(args);

        REQUIRE(args[0] == '\0');
        REQUIRE(args[1] == '\0');
    }

    SECTION("Get tokens") {
        strcpy_s(args, argsBufLen, "abcd efg");
        embeddedCliTokenizeArgs(args);

        const char* tok0 = embeddedCliGetToken(args, 0);
        const char* tok1 = embeddedCliGetToken(args, 1);
        const char* tok2 = embeddedCliGetToken(args, 2);

        REQUIRE(tok0 != nullptr);
        REQUIRE(tok1 != nullptr);
        REQUIRE(tok2 == nullptr);

        REQUIRE(std::string(tok0) == "abcd");
        REQUIRE(std::string(tok1) == "efg");
    }

    SECTION("Get tokens from empty string") {
        strcpy_s(args, argsBufLen, "");
        embeddedCliTokenizeArgs(args);

        const char* tok0 = embeddedCliGetToken(args, 0);

        REQUIRE(tok0 == nullptr);
    }

    SECTION("Get token count") {
        strcpy_s(args, argsBufLen, "a b c");
        embeddedCliTokenizeArgs(args);

        REQUIRE(embeddedCliGetTokenCount(args) == 3);
    }

    SECTION("Get token count from empty string") {
        strcpy_s(args, argsBufLen, "");
        embeddedCliTokenizeArgs(args);

        REQUIRE(embeddedCliGetTokenCount(args) == 0);
    }
}

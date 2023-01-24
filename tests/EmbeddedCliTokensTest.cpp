#include "embedded_cli.h"

#include <catch2/catch_test_macros.hpp>

static void setVectorString(std::vector<char> &buffer, const std::string &str) {
    buffer.resize(str.size() + 2, '\0');
    std::copy(str.begin(), str.end(), buffer.begin());
    buffer[str.size()] = '\0';
}

TEST_CASE("EmbeddedCli. Tokens", "[cli][token]") {
    std::vector<char> buffer;
    buffer.resize(30, '!');
    buffer.resize(32, '\0');

    SECTION("Tokenize simple string") {
        setVectorString(buffer, "a b c");
        embeddedCliTokenizeArgs(buffer.data());

        REQUIRE(buffer[0] == 'a');
        REQUIRE(buffer[1] == '\0');
        REQUIRE(buffer[2] == 'b');
        REQUIRE(buffer[3] == '\0');
        REQUIRE(buffer[4] == 'c');
        REQUIRE(buffer[5] == '\0');
        REQUIRE(buffer[6] == '\0');
    }

    SECTION("Tokenize string with duplicating separators") {
        setVectorString(buffer, "   a  b    c   ");
        embeddedCliTokenizeArgs(buffer.data());

        REQUIRE(buffer[0] == 'a');
        REQUIRE(buffer[1] == '\0');
        REQUIRE(buffer[2] == 'b');
        REQUIRE(buffer[3] == '\0');
        REQUIRE(buffer[4] == 'c');
        REQUIRE(buffer[5] == '\0');
        REQUIRE(buffer[6] == '\0');
    }

    SECTION("Tokenize string with long tokens") {
        setVectorString(buffer, "abcd ef");
        embeddedCliTokenizeArgs(buffer.data());

        REQUIRE(buffer[0] == 'a');
        REQUIRE(buffer[1] == 'b');
        REQUIRE(buffer[2] == 'c');
        REQUIRE(buffer[3] == 'd');
        REQUIRE(buffer[4] == '\0');
        REQUIRE(buffer[5] == 'e');
        REQUIRE(buffer[6] == 'f');
        REQUIRE(buffer[7] == '\0');
        REQUIRE(buffer[8] == '\0');
    }

    SECTION("Tokenize string of separators") {
        setVectorString(buffer, "      ");
        embeddedCliTokenizeArgs(buffer.data());

        REQUIRE(buffer[0] == '\0');
        REQUIRE(buffer[1] == '\0');
    }

    SECTION("Tokenize empty string") {
        setVectorString(buffer, "");
        embeddedCliTokenizeArgs(buffer.data());

        REQUIRE(buffer[0] == '\0');
        REQUIRE(buffer[1] == '\0');
    }

    SECTION("Tokenize null") {
        embeddedCliTokenizeArgs(nullptr);
    }

    SECTION("Get tokens") {
        setVectorString(buffer, "abcd efg");
        embeddedCliTokenizeArgs(buffer.data());

        const char *tok0 = embeddedCliGetToken(buffer.data(), 0);
        const char *tok1 = embeddedCliGetToken(buffer.data(), 1);
        const char *tok2 = embeddedCliGetToken(buffer.data(), 2);
        const char *tok3 = embeddedCliGetToken(buffer.data(), 3);

        REQUIRE(tok0 == nullptr);
        REQUIRE(tok1 != nullptr);
        REQUIRE(tok2 != nullptr);
        REQUIRE(tok3 == nullptr);

        REQUIRE(std::string(tok1) == "abcd");
        REQUIRE(std::string(tok2) == "efg");
    }


    SECTION("Get tokens with quoted args") {
        setVectorString(buffer, R"("abcd efg" te\\st "test\"2 3" "test4 5 6")");
        embeddedCliTokenizeArgs(buffer.data());

        const char *tok1 = embeddedCliGetToken(buffer.data(), 1);
        const char *tok2 = embeddedCliGetToken(buffer.data(), 2);
        const char *tok3 = embeddedCliGetToken(buffer.data(), 3);
        const char *tok4 = embeddedCliGetToken(buffer.data(), 4);

        REQUIRE(tok1 != nullptr);
        REQUIRE(tok2 != nullptr);
        REQUIRE(tok3 != nullptr);
        REQUIRE(tok4 != nullptr);

        REQUIRE(std::string(tok1) == "abcd efg");
        REQUIRE(std::string(tok2) == "te\\st");
        REQUIRE(std::string(tok3) == "test\"2 3");
        REQUIRE(std::string(tok4) == "test4 5 6");
    }

    SECTION("Quoted args without spaces") {
        setVectorString(buffer, R"("abcd efg"test"test\"2 3")");
        embeddedCliTokenizeArgs(buffer.data());

        const char *tok1 = embeddedCliGetToken(buffer.data(), 1);
        const char *tok2 = embeddedCliGetToken(buffer.data(), 2);
        const char *tok3 = embeddedCliGetToken(buffer.data(), 3);

        REQUIRE(tok1 != nullptr);
        REQUIRE(tok2 != nullptr);
        REQUIRE(tok3 != nullptr);

        REQUIRE(std::string(tok1) == "abcd efg");
        REQUIRE(std::string(tok2) == "test");
        REQUIRE(std::string(tok3) == "test\"2 3");
    }

    SECTION("Get tokens from empty string") {
        setVectorString(buffer, "");
        embeddedCliTokenizeArgs(buffer.data());

        const char *tok0 = embeddedCliGetToken(buffer.data(), 1);

        REQUIRE(tok0 == nullptr);
    }

    SECTION("Get token from null string") {
        const char *tok0 = embeddedCliGetToken(nullptr, 1);

        REQUIRE(tok0 == nullptr);
    }

    SECTION("Get token count") {
        setVectorString(buffer, "a b c");
        embeddedCliTokenizeArgs(buffer.data());

        REQUIRE(embeddedCliGetTokenCount(buffer.data()) == 3);
    }

    SECTION("Get token count from empty string") {
        setVectorString(buffer, "");
        embeddedCliTokenizeArgs(buffer.data());

        REQUIRE(embeddedCliGetTokenCount(buffer.data()) == 0);
    }

    SECTION("Get token count for null string") {
        REQUIRE(embeddedCliGetTokenCount(nullptr) == 0);
    }

    SECTION("Find tokens") {
        SECTION("Find token in empty list") {
            REQUIRE(embeddedCliFindToken("\0\0", "tok") == 0);
            REQUIRE(embeddedCliFindToken(nullptr, "tok") == 0);
        }

        SECTION("Find not existing token") {
            setVectorString(buffer, "test abc");
            embeddedCliTokenizeArgs(buffer.data());

            REQUIRE(embeddedCliFindToken(buffer.data(), "tok") == 0);
        }

        SECTION("Find existing token") {
            setVectorString(buffer, "test tok abc");
            embeddedCliTokenizeArgs(buffer.data());

            REQUIRE(embeddedCliFindToken(buffer.data(), "tok") == 2);
        }
    }
}

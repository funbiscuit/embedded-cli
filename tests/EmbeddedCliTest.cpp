#include <random>
#include <iostream>

#include <catch.hpp>

#include "embedded_cli.h"


TEST_CASE( "EmbeddedCli", "[cli]" )
{
    EmbeddedCli *cli = embeddedCliNew();

    REQUIRE(cli != nullptr);

}


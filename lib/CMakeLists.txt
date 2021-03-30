cmake_minimum_required(VERSION 3.8...3.17)

add_library(embedded_cli_lib STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/embedded_cli.c
        )

target_include_directories(embedded_cli_lib
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        )

add_library(EmbeddedCLI::EmbeddedCLI ALIAS embedded_cli_lib)

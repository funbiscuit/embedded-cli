cmake_minimum_required(VERSION 3.8...3.17)

add_library(embedded_cli_lib STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/embedded_cli.c
        )

target_include_directories(embedded_cli_lib
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        )

if (MSVC)
    # maybe should add more
    target_compile_options(embedded_cli_lib PRIVATE /Wall /WX)
else ()
    target_compile_options(embedded_cli_lib PRIVATE
            -Werror
            -Wall
            -Wextra
            -Wpedantic
            -Wcast-align=strict
            -Wcast-qual
            -Wconversion
            -Wduplicated-branches
            -Wduplicated-cond
            -Wfloat-equal
            -Wlogical-op
            -Wredundant-decls
            -Wsign-conversion
            -pedantic-errors)
endif ()

add_library(EmbeddedCLI::EmbeddedCLI ALIAS embedded_cli_lib)

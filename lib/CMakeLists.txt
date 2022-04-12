cmake_minimum_required(VERSION 3.8...3.17)

# Build single-header version
find_package(PythonInterp 3.0 REQUIRED)
execute_process(COMMAND ${PYTHON_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/build-shl.py
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_library(embedded_cli_lib STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/embedded_cli.c
        )

target_include_directories(embedded_cli_lib
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        )

add_library(embedded_cli_shl INTERFACE)
target_include_directories(embedded_cli_shl INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/shl)

if (MSVC)
    # maybe should add more
    # /WX gives too picky results
    target_compile_options(embedded_cli_lib PRIVATE /Wall)
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
add_library(EmbeddedCLI::SingleHeader ALIAS embedded_cli_shl)

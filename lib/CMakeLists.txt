cmake_minimum_required(VERSION 3.8...3.17)

# Build single-header version
if (${BUILD_SINGLE_HEADER})
    find_package(PythonInterp 3.0 REQUIRED)
    execute_process(COMMAND ${PYTHON_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/build-shl.py
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    add_library(embedded_cli_shl INTERFACE)
    target_include_directories(embedded_cli_shl INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}/shl)
    add_library(EmbeddedCLI::SingleHeader ALIAS embedded_cli_shl)
endif ()

add_library(embedded_cli_lib STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/embedded_cli.c
        )

target_include_directories(embedded_cli_lib
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        )

if (CMAKE_C_COMPILER_ID MATCHES "Clang")
    MESSAGE("Enable extra flags for Clang")
    target_compile_options(embedded_cli_lib PRIVATE
            -Werror
            -Wall
            -Wextra
            -Wpedantic
            -Wcast-qual
            -Wconversion
            -Wfloat-equal
            -Wredundant-decls
            -Wsign-conversion
            -Wwrite-strings
            -pedantic-errors)
elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    MESSAGE("Enable extra flags for GNU")
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
            -Wwrite-strings
            -pedantic-errors)
elseif (CMAKE_C_COMPILER_ID MATCHES "MSVC")
    MESSAGE("Enable extra flags for MSVC")
    # maybe should add more
    # /WX gives too picky results
    target_compile_options(embedded_cli_lib PRIVATE /Wall)
else ()
    MESSAGE(WARNING "Can't enable extra flags for compiler ${CMAKE_CXX_COMPILER_ID}")
endif ()

add_library(EmbeddedCLI::EmbeddedCLI ALIAS embedded_cli_lib)

cmake_minimum_required(VERSION 3.8...3.17)
project(embedded_cli C)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake-modules")

set(CMAKE_C_STANDARD 11)

option(BUILD_TESTS "Build and run tests" OFF)
option(TESTS_COV "Run coverage on tests" OFF)
option(BUILD_SINGLE_HEADER "Build single-header version" OFF)

if (${BUILD_TESTS})
    # C++ is only used in tests
    enable_language(CXX)
    set(CMAKE_CXX_STANDARD 20)
endif ()

add_subdirectory(lib)

if (WIN32)
    add_subdirectory(examples/win32-example)
endif (WIN32)

if (${BUILD_TESTS})
    include(CTest)
    add_subdirectory(deps/catch2)
    add_subdirectory(tests)
    add_test(CliTests tests/embedded_cli_tests)
    if (${TESTS_COV})
        include(CodeCoverage)
        append_coverage_compiler_flags()

        setup_target_for_coverage_gcovr_xml(
                NAME coverage_xml
                DEPENDENCIES embedded_cli_tests embedded_cli_lib
                EXCLUDE "tests/*"
                EXECUTABLE embedded_cli_tests)
        setup_target_for_coverage_gcovr_html(
                NAME coverage_html
                DEPENDENCIES embedded_cli_tests embedded_cli_lib
                EXCLUDE "tests/*"
                EXECUTABLE embedded_cli_tests)
    endif ()
endif ()

cmake_minimum_required(VERSION 3.8...3.17)
project(embedded_cli C CXX)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake-modules")

set(CMAKE_C_STANDARD 11)

option(TESTS_COV "Run coverage on tests" OFF)

if (${TESTS_COV})
    include(CodeCoverage)
    append_coverage_compiler_flags()
endif ()

add_subdirectory(lib)

if (WIN32)
    add_subdirectory(examples/win32-example)
endif (WIN32)

include(CTest)
add_subdirectory(tests)
add_test(CliTests tests/embedded_cli_tests)
if (${TESTS_COV})
    setup_target_for_coverage_gcovr_xml(
            NAME coverage_xml
            DEPENDENCIES embedded_cli_tests embedded_cli_lib
            EXCLUDE "tests/*"
            #            "${PROJECT_BINARY_DIR}/app-gui/*"
            EXECUTABLE embedded_cli_tests)
    setup_target_for_coverage_gcovr_html(
            NAME coverage_html
            DEPENDENCIES embedded_cli_tests embedded_cli_lib
            EXCLUDE "tests/*"
            #            "${PROJECT_BINARY_DIR}/app-gui/*"
            EXECUTABLE embedded_cli_tests)
endif ()

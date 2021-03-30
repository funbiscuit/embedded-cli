cmake_minimum_required(VERSION 3.8...3.17)
project(embedded_cli C CXX)

set(CMAKE_C_STANDARD 11)

add_subdirectory(lib)

include(CTest)
add_subdirectory(tests)
add_test(CliTests tests/embedded_cli_tests)

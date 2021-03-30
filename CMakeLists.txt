cmake_minimum_required(VERSION 3.8...3.17)
project(embedded_cli C CXX)

set(CMAKE_C_STANDARD 11)

add_subdirectory(lib)

add_subdirectory(tests)

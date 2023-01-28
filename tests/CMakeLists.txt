add_executable(embedded_cli_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/CliBuilder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/CliWrapper.cpp
        )

target_include_directories(embedded_cli_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        )

# tests
target_sources(embedded_cli_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/cli/AutocompleteTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/cli/BaseTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/cli/HelpTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/cli/HistoryTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/cli/PrintTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/cli/StaticAllocationTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/TokensTest.cpp
        )

target_link_libraries(embedded_cli_tests PRIVATE EmbeddedCLI::EmbeddedCLI)
target_link_libraries(embedded_cli_tests PRIVATE Catch2WithMain)

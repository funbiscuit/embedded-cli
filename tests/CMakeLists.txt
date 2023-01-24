add_executable(embedded_cli_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/EmbeddedCliTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/EmbeddedCliTokensTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/CliMock.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/CliTestRunner.cpp
        )

target_include_directories(embedded_cli_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        )

target_link_libraries(embedded_cli_tests PRIVATE EmbeddedCLI::EmbeddedCLI)
target_link_libraries(embedded_cli_tests PRIVATE Catch2WithMain)

add_executable(embedded_cli_win32 main.cpp)

target_include_directories(embedded_cli_win32 PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        )

target_link_libraries(embedded_cli_win32 PRIVATE EmbeddedCLI::EmbeddedCLI)

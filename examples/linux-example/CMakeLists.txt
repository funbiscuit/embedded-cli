add_executable(embedded_cli_linux main.cpp)

target_include_directories(embedded_cli_linux PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        )

if (${BUILD_SINGLE_HEADER})
    target_link_libraries(embedded_cli_linux PRIVATE EmbeddedCLI::SingleHeader)
else ()
    target_link_libraries(embedded_cli_linux PRIVATE EmbeddedCLI::EmbeddedCLI)
endif ()

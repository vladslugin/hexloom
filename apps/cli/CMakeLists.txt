add_executable(hexloom main.cpp)
target_link_libraries(
    hexloom
    PRIVATE
        Hexloom::Generation
)

if(HEXLOOM_BUILD_TESTS)
    add_test(
        NAME hexloom_cli_validate
        COMMAND
            hexloom
            validate
            ${PROJECT_SOURCE_DIR}/games/material-lab/game.yaml
    )
    add_test(
        NAME hexloom_cli_generate
        COMMAND
            ${CMAKE_COMMAND}
            -DHEXLOOM_CLI=$<TARGET_FILE:hexloom>
            -DHEXLOOM_SPEC=${PROJECT_SOURCE_DIR}/engine/core/tests/fixtures/valid_game.yaml
            -DHEXLOOM_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/cli-generation-output
            -P
            ${PROJECT_SOURCE_DIR}/cmake/run-generation-cli-test.cmake
    )
endif()

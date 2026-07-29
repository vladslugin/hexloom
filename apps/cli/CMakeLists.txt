add_executable(hexloom main.cpp)
target_link_libraries(hexloom PRIVATE Hexloom::Core)

if(HEXLOOM_BUILD_TESTS)
    add_test(
        NAME hexloom_cli_validate
        COMMAND
            hexloom
            validate
            ${PROJECT_SOURCE_DIR}/games/material-lab/game.yaml
    )
endif()

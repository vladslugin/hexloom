add_executable(hexloom_agent_tests
    agent_cli_test.cpp
)

target_link_libraries(
    hexloom_agent_tests
    PRIVATE
        Hexloom::Agents
)

add_test(NAME hexloom_agent_tests COMMAND hexloom_agent_tests)

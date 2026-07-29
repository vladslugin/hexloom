add_executable(hexloom_generation_tests
    deterministic_texture_generator_test.cpp
)

target_link_libraries(
    hexloom_generation_tests
    PRIVATE
        Hexloom::Generation
)

add_test(NAME hexloom_generation_tests COMMAND hexloom_generation_tests)

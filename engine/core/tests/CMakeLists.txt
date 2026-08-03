add_executable(hexloom_core_tests
    material_request_test.cpp
    material_spec_loader_test.cpp
    project_memory_test.cpp
)

target_link_libraries(hexloom_core_tests PRIVATE Hexloom::Core)
target_compile_definitions(
    hexloom_core_tests
    PRIVATE
        HEXLOOM_TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
)

add_test(NAME hexloom_core_tests COMMAND hexloom_core_tests)

add_executable(hexloom_core_tests
    material_request_test.cpp
)

target_link_libraries(hexloom_core_tests PRIVATE Hexloom::Core)

add_test(NAME hexloom_core_tests COMMAND hexloom_core_tests)

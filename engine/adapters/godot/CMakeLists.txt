add_library(hexloom_godot SHARED
    src/hexloom_agent_bridge.cpp
    src/hexloom_material_bridge.cpp
    src/register_types.cpp
)

target_include_directories(hexloom_godot
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(hexloom_godot
    PRIVATE
        Hexloom::Core
        Hexloom::Agents
        godot-cpp
)

target_compile_features(hexloom_godot PRIVATE cxx_std_20)

set_target_properties(hexloom_godot PROPERTIES
    OUTPUT_NAME "hexloom_godot"
    LIBRARY_OUTPUT_DIRECTORY
        "${PROJECT_SOURCE_DIR}/godot/material-lab/bin"
    RUNTIME_OUTPUT_DIRECTORY
        "${PROJECT_SOURCE_DIR}/godot/material-lab/bin"
)

add_custom_command(
    TARGET hexloom_godot
    POST_BUILD
    COMMAND
        ${CMAKE_COMMAND} -E make_directory
        "${PROJECT_SOURCE_DIR}/godot/studio/bin"
    COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:hexloom_godot>"
        "${PROJECT_SOURCE_DIR}/godot/studio/bin/$<TARGET_FILE_NAME:hexloom_godot>"
    VERBATIM
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(
        hexloom_godot
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wno-gnu-anonymous-struct
            -Wno-nested-anon-types
            -Wno-unused-parameter
    )
elseif(MSVC)
    target_compile_options(hexloom_godot PRIVATE /W4)
endif()

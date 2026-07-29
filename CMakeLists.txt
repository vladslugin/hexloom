cmake_minimum_required(VERSION 3.25)

project(
    Hexloom
    VERSION 0.1.0
    DESCRIPTION "AI-first game creation studio"
    LANGUAGES CXX
)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(HEXLOOM_BUILD_TESTS "Build Hexloom tests" ON)
option(HEXLOOM_BUILD_GODOT_ADAPTER "Build the Godot GDExtension adapter" OFF)

set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    yaml_cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG 56e3bb550c91fd7005566f19c079cb7a503223cf
    GIT_SHALLOW FALSE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(yaml_cpp)

if(HEXLOOM_BUILD_TESTS)
    enable_testing()
endif()

add_subdirectory(engine/core)
add_subdirectory(engine/generation)
add_subdirectory(apps/cli)

if(HEXLOOM_BUILD_GODOT_ADAPTER)
    # Pinned for reproducible Godot 4.7 builds. Update intentionally after
    # validating the extension against a new stable Godot release.
    set(
        GODOTCPP_BUILD_PROFILE
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/godot-cpp-profile.json"
        CACHE FILEPATH
        "Minimal Godot API surface required by Hexloom"
        FORCE
    )
    FetchContent_Declare(
        godot_cpp
        GIT_REPOSITORY https://github.com/godotengine/godot-cpp.git
        GIT_TAG eb006b6276e89fa7d7c26cc23cb3abe43e90442f
        GIT_SHALLOW FALSE
        GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(godot_cpp)
    add_subdirectory(engine/adapters/godot)
endif()

if(HEXLOOM_BUILD_TESTS)
    add_subdirectory(engine/core/tests)
    add_subdirectory(engine/generation/tests)

    if(HEXLOOM_BUILD_GODOT_ADAPTER)
        find_program(HEXLOOM_GODOT_EXECUTABLE NAMES godot godot4)
        if(HEXLOOM_GODOT_EXECUTABLE)
            add_test(
                NAME hexloom_godot_smoke
                COMMAND
                    ${CMAKE_COMMAND}
                    -DHEXLOOM_CLI=$<TARGET_FILE:hexloom>
                    -DHEXLOOM_GODOT=${HEXLOOM_GODOT_EXECUTABLE}
                    -DHEXLOOM_PROJECT=${CMAKE_CURRENT_SOURCE_DIR}/godot/material-lab
                    -DHEXLOOM_SPEC=${CMAKE_CURRENT_SOURCE_DIR}/engine/core/tests/fixtures/valid_game.yaml
                    -DHEXLOOM_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/godot-smoke-artifact
                    -P
                    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run-godot-smoke-test.cmake
            )
            set_tests_properties(hexloom_godot_smoke PROPERTIES TIMEOUT 15)
        endif()
    endif()
endif()

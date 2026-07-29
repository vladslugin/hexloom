add_library(hexloom_agents STATIC
    src/agent_cli.cpp
)

if(UNIX)
    target_sources(hexloom_agents PRIVATE src/process_runner_posix.cpp)
elseif(WIN32)
    target_sources(hexloom_agents PRIVATE src/process_runner_windows.cpp)
endif()

add_library(Hexloom::Agents ALIAS hexloom_agents)

target_include_directories(hexloom_agents
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(hexloom_agents PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(
        hexloom_agents
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
elseif(MSVC)
    target_compile_options(hexloom_agents PRIVATE /W4)
endif()

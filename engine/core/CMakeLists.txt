add_library(hexloom_core STATIC
    src/art_style.cpp
    src/material_request.cpp
    src/material_spec_loader.cpp
    src/project_memory.cpp
)

add_library(Hexloom::Core ALIAS hexloom_core)

target_include_directories(hexloom_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(hexloom_core PUBLIC cxx_std_20)
target_link_libraries(hexloom_core PUBLIC yaml-cpp::yaml-cpp)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(hexloom_core PRIVATE -Wall -Wextra -Wpedantic)
elseif(MSVC)
    target_compile_options(hexloom_core PRIVATE /W4)
endif()

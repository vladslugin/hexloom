add_library(hexloom_core STATIC
    src/material_request.cpp
)

add_library(Hexloom::Core ALIAS hexloom_core)

target_include_directories(hexloom_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(hexloom_core PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(hexloom_core PRIVATE -Wall -Wextra -Wpedantic)
elseif(MSVC)
    target_compile_options(hexloom_core PRIVATE /W4)
endif()

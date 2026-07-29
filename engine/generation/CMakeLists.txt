add_library(hexloom_generation STATIC
    src/deterministic_texture_generator.cpp
)

add_library(Hexloom::Generation ALIAS hexloom_generation)

target_include_directories(hexloom_generation
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(hexloom_generation PUBLIC cxx_std_20)
target_link_libraries(
    hexloom_generation
    PUBLIC
        Hexloom::Core
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(
        hexloom_generation
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
elseif(MSVC)
    target_compile_options(hexloom_generation PRIVATE /W4)
endif()

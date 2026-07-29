#include "hexloom/generation/texture_prompt.hpp"

#include <iostream>
#include <string>

int run_texture_prompt_tests() {
    const hexloom::MaterialRequest request{
        .id = "ancient_stone",
        .category = "stone",
        .style_id = "hexloom_lowpoly",
        .resolution = 256,
        .physical_size_meters = 2.0F,
        .seamless = true,
        .mobile_optimized = true,
        .maps = {
            hexloom::TextureMap::albedo,
            hexloom::TextureMap::normal,
        },
    };
    const hexloom::ArtStyleProfile style{
        .id = "hexloom_lowpoly",
        .geometry = "low_poly",
        .silhouettes = "exaggerated",
        .edges = "soft_beveled",
        .texture_style = "stylized_pbr",
        .lighting = "atmospheric",
        .realism = 0.25,
        .base_color = {0x57, 0x6A, 0x91},
        .secondary_color = {0x26, 0x31, 0x47},
        .accent_color = {0x49, 0xD6, 0xFF},
        .forbidden = {"photorealism", "visible_text"},
    };

    const auto prompt =
        hexloom::generation::compile_texture_prompt(request, style);
    int failures = 0;
    const auto check = [&failures](bool condition, const char* message) {
        if (!condition) {
            std::cerr << "FAIL: " << message << '\n';
            ++failures;
        }
    };

    check(
        prompt.positive.find("ancient_stone") != std::string::npos,
        "positive prompt should name the material"
    );
    check(
        prompt.positive.find("#576A91") != std::string::npos,
        "positive prompt should encode the palette"
    );
    check(
        prompt.positive.find("albedo normal") != std::string::npos,
        "positive prompt should list requested maps"
    );
    check(
        prompt.negative == "photorealism, visible text",
        "negative prompt should be derived from forbidden traits"
    );
    return failures;
}

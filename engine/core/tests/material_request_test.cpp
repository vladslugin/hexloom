#include "hexloom/core/material_request.hpp"

#include <iostream>

int material_request_tests() {
    int failures = 0;
    const auto check = [&failures](bool condition, const char* message) {
        if (!condition) {
            std::cerr << "FAIL: " << message << '\n';
            ++failures;
        }
    };

    const hexloom::MaterialRequest valid_request{
        .id = "ancient_stone_floor",
        .category = "stone",
        .style_id = "soft_neon_scifi",
        .resolution = 2048,
        .physical_size_meters = 2.0F,
        .seamless = true,
        .mobile_optimized = true,
        .maps = {
            hexloom::TextureMap::albedo,
            hexloom::TextureMap::normal,
            hexloom::TextureMap::roughness,
        },
    };

    check(
        hexloom::validate(valid_request).empty(),
        "valid material request should pass"
    );

    auto invalid_request = valid_request;
    invalid_request.id.clear();
    invalid_request.resolution = 3000;
    invalid_request.physical_size_meters = 0.0F;

    const auto issues = hexloom::validate(invalid_request);
    check(issues.size() == 4, "invalid request should report four issues");

    return failures;
}

#include "hexloom/core/material_request.hpp"

#include <cassert>
#include <iostream>

int main() {
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

    assert(hexloom::validate(valid_request).empty());

    auto invalid_request = valid_request;
    invalid_request.id.clear();
    invalid_request.resolution = 3000;
    invalid_request.physical_size_meters = 0.0F;

    const auto issues = hexloom::validate(invalid_request);
    assert(issues.size() == 4);

    std::cout << "Hexloom core tests passed.\n";
    return 0;
}

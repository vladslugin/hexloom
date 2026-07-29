#include "hexloom/core/material_request.hpp"

#include <iostream>

int main() {
    const hexloom::MaterialRequest request{
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
            hexloom::TextureMap::ambient_occlusion,
        },
    };

    const auto issues = hexloom::validate(request);

    std::cout << "Hexloom material request: " << request.id << '\n';
    if (!issues.empty()) {
        for (const auto& issue : issues) {
            std::cerr << "error [" << issue.field << "]: "
                      << issue.message << '\n';
        }
        return 1;
    }

    std::cout << "status: ready\nmaps:";
    for (const auto map : request.maps) {
        std::cout << ' ' << hexloom::to_string(map);
    }
    std::cout << '\n';

    return 0;
}

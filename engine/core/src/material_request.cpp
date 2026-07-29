#include "hexloom/core/material_request.hpp"

#include <algorithm>

namespace hexloom {
namespace {

[[nodiscard]] bool is_power_of_two(std::uint32_t value) {
    return value != 0 && (value & (value - 1)) == 0;
}

}  // namespace

std::vector<ValidationIssue> validate(const MaterialRequest& request) {
    std::vector<ValidationIssue> issues;

    if (request.id.empty()) {
        issues.push_back({"id", "Material id must not be empty."});
    }

    if (request.category.empty()) {
        issues.push_back({"category", "Material category must not be empty."});
    }

    if (request.style_id.empty()) {
        issues.push_back({"style_id", "A material must reference an art style."});
    }

    if (!is_power_of_two(request.resolution)) {
        issues.push_back({
            "resolution",
            "Texture resolution must be a power of two.",
        });
    }

    if (request.resolution < 256 || request.resolution > 4096) {
        issues.push_back({
            "resolution",
            "Texture resolution must be between 256 and 4096.",
        });
    }

    if (request.mobile_optimized && request.resolution > 2048) {
        issues.push_back({
            "resolution",
            "Mobile materials may not exceed 2048 pixels.",
        });
    }

    if (request.physical_size_meters <= 0.0F) {
        issues.push_back({
            "physical_size_meters",
            "Physical material size must be greater than zero.",
        });
    }

    if (request.maps.empty()) {
        issues.push_back({"maps", "At least one texture map is required."});
    }

    const auto has_albedo =
        std::ranges::find(request.maps, TextureMap::albedo) !=
        request.maps.end();
    if (!has_albedo) {
        issues.push_back({"maps", "A generated material requires an albedo map."});
    }

    return issues;
}

std::string_view to_string(TextureMap map) {
    switch (map) {
        case TextureMap::albedo:
            return "albedo";
        case TextureMap::normal:
            return "normal";
        case TextureMap::roughness:
            return "roughness";
        case TextureMap::metallic:
            return "metallic";
        case TextureMap::ambient_occlusion:
            return "ambient_occlusion";
        case TextureMap::height:
            return "height";
    }

    return "unknown";
}

}  // namespace hexloom

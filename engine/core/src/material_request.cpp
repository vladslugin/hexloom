#include "hexloom/core/material_request.hpp"

#include <algorithm>
#include <array>
#include <cctype>

namespace hexloom {
namespace {

[[nodiscard]] bool is_power_of_two(std::uint32_t value) {
    return value != 0 && (value & (value - 1)) == 0;
}

[[nodiscard]] bool is_portable_identifier(std::string_view value) {
    if (value.empty() || value.size() > 64) {
        return false;
    }

    return std::ranges::all_of(value, [](unsigned char character) {
        return std::islower(character) || std::isdigit(character) ||
            character == '_' || character == '-';
    });
}

[[nodiscard]] std::size_t map_index(TextureMap map) {
    return static_cast<std::size_t>(map);
}

}  // namespace

std::vector<ValidationIssue> validate(const MaterialRequest& request) {
    std::vector<ValidationIssue> issues;

    if (request.id.empty()) {
        issues.push_back({"id", "Material id must not be empty."});
    } else if (!is_portable_identifier(request.id)) {
        issues.push_back({
            "id",
            "Material id may contain only lowercase letters, digits, "
            "underscores, and hyphens.",
        });
    }

    if (request.category.empty()) {
        issues.push_back({"category", "Material category must not be empty."});
    }

    if (request.style_id.empty()) {
        issues.push_back({"style_id", "A material must reference an art style."});
    } else if (!is_portable_identifier(request.style_id)) {
        issues.push_back({
            "style_id",
            "Style id may contain only lowercase letters, digits, "
            "underscores, and hyphens.",
        });
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

    std::array<bool, 6> seen_maps{};
    for (const auto map : request.maps) {
        const auto index = map_index(map);
        if (seen_maps[index]) {
            issues.push_back({
                "maps",
                "Texture map '" + std::string(to_string(map)) +
                    "' is listed more than once.",
            });
        }
        seen_maps[index] = true;
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

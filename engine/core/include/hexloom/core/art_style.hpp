#pragma once

#include "hexloom/core/material_request.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace hexloom {

struct RgbColor {
    std::uint8_t red = 0;
    std::uint8_t green = 0;
    std::uint8_t blue = 0;

    [[nodiscard]] bool operator==(const RgbColor&) const = default;
};

struct ArtStyleProfile {
    std::string id;
    std::string geometry;
    std::string silhouettes;
    std::string edges;
    std::string texture_style;
    std::string lighting;
    double realism = 0.0;
    RgbColor base_color;
    RgbColor secondary_color;
    RgbColor accent_color;
    std::vector<std::string> forbidden;
};

[[nodiscard]] std::vector<ValidationIssue> validate(
    const ArtStyleProfile& profile
);

}  // namespace hexloom

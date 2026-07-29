#include "hexloom/core/art_style.hpp"

#include <algorithm>
#include <array>
#include <string_view>

namespace hexloom {
namespace {

template <std::size_t Size>
[[nodiscard]] bool is_one_of(
    std::string_view value,
    const std::array<std::string_view, Size>& supported
) {
    return std::ranges::find(supported, value) != supported.end();
}

void require_supported(
    std::vector<ValidationIssue>& issues,
    std::string field,
    std::string_view value,
    const auto& supported
) {
    if (value.empty()) {
        issues.push_back({std::move(field), "Required field must not be empty."});
    } else if (!is_one_of(value, supported)) {
        issues.push_back({
            std::move(field),
            "Value '" + std::string(value) + "' is not supported.",
        });
    }
}

}  // namespace

std::vector<ValidationIssue> validate(const ArtStyleProfile& profile) {
    std::vector<ValidationIssue> issues;

    if (profile.id.empty()) {
        issues.push_back({"id", "Art style id must not be empty."});
    }

    require_supported(
        issues,
        "geometry",
        profile.geometry,
        std::array<std::string_view, 2>{"low_poly", "high_poly"}
    );
    require_supported(
        issues,
        "silhouettes",
        profile.silhouettes,
        std::array<std::string_view, 2>{"exaggerated", "natural"}
    );
    require_supported(
        issues,
        "edges",
        profile.edges,
        std::array<std::string_view, 3>{
            "soft_beveled",
            "hard",
            "hand_painted",
        }
    );
    require_supported(
        issues,
        "texture_style",
        profile.texture_style,
        std::array<std::string_view, 3>{
            "stylized_pbr",
            "hand_painted",
            "flat_color",
        }
    );
    require_supported(
        issues,
        "lighting",
        profile.lighting,
        std::array<std::string_view, 3>{
            "atmospheric",
            "high_contrast",
            "soft",
        }
    );

    if (profile.realism < 0.0 || profile.realism > 1.0) {
        issues.push_back({
            "realism",
            "Realism must be between 0.0 and 1.0.",
        });
    }

    if (profile.forbidden.empty()) {
        issues.push_back({
            "forbidden",
            "At least one forbidden visual trait is required.",
        });
    }

    return issues;
}

}  // namespace hexloom

#pragma once

#include "hexloom/core/art_style.hpp"
#include "hexloom/core/material_request.hpp"

#include <filesystem>
#include <optional>
#include <vector>

namespace hexloom {

struct MaterialLoadResult {
    std::optional<MaterialRequest> request;
    std::optional<ArtStyleProfile> style;
    std::vector<ValidationIssue> issues;

    [[nodiscard]] bool ok() const {
        return request.has_value() && style.has_value() && issues.empty();
    }
};

[[nodiscard]] MaterialLoadResult load_material_request(
    const std::filesystem::path& specification_path
);

}  // namespace hexloom

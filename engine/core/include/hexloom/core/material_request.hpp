#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace hexloom {

enum class TextureMap {
    albedo,
    normal,
    roughness,
    metallic,
    ambient_occlusion,
    height,
};

struct MaterialRequest {
    std::string id;
    std::string category;
    std::string style_id;
    std::uint32_t resolution = 1024;
    float physical_size_meters = 1.0F;
    bool seamless = true;
    bool mobile_optimized = true;
    std::vector<TextureMap> maps;
};

struct ValidationIssue {
    std::string field;
    std::string message;
};

[[nodiscard]] std::vector<ValidationIssue> validate(
    const MaterialRequest& request
);

[[nodiscard]] std::string_view to_string(TextureMap map);

}  // namespace hexloom

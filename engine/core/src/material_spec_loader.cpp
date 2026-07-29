#include "hexloom/core/material_spec_loader.hpp"

#include "hexloom/core/art_style.hpp"

#include <yaml-cpp/yaml.h>

#include <cstdint>
#include <charconv>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

namespace hexloom {
namespace {

void add_issue(
    std::vector<ValidationIssue>& issues,
    std::string field,
    std::string message
) {
    issues.push_back({std::move(field), std::move(message)});
}

template <typename T>
bool read_required(
    const YAML::Node& parent,
    std::string_view key,
    std::string field,
    T& destination,
    std::vector<ValidationIssue>& issues
) {
    const YAML::Node value = parent[std::string(key)];
    if (!value || value.IsNull()) {
        add_issue(issues, std::move(field), "Required field is missing.");
        return false;
    }

    try {
        destination = value.as<T>();
        return true;
    } catch (const YAML::Exception&) {
        add_issue(issues, std::move(field), "Field has an invalid type.");
        return false;
    }
}

[[nodiscard]] std::optional<TextureMap> parse_texture_map(
    std::string_view name
) {
    if (name == "albedo") {
        return TextureMap::albedo;
    }
    if (name == "normal") {
        return TextureMap::normal;
    }
    if (name == "roughness") {
        return TextureMap::roughness;
    }
    if (name == "metallic") {
        return TextureMap::metallic;
    }
    if (name == "ambient_occlusion") {
        return TextureMap::ambient_occlusion;
    }
    if (name == "height") {
        return TextureMap::height;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<RgbColor> parse_hex_color(
    const YAML::Node& node,
    std::string field,
    std::vector<ValidationIssue>& issues
) {
    if (!node || node.IsNull()) {
        add_issue(issues, std::move(field), "Required color is missing.");
        return std::nullopt;
    }

    std::string value;
    try {
        value = node.as<std::string>();
    } catch (const YAML::Exception&) {
        add_issue(issues, std::move(field), "Color must be a string.");
        return std::nullopt;
    }

    if (value.size() != 7 || value.front() != '#') {
        add_issue(
            issues,
            std::move(field),
            "Color must use #RRGGBB format."
        );
        return std::nullopt;
    }

    std::uint32_t packed = 0;
    const auto parsed = std::from_chars(
        value.data() + 1,
        value.data() + value.size(),
        packed,
        16
    );
    if (parsed.ec != std::errc{} ||
        parsed.ptr != value.data() + value.size()) {
        add_issue(
            issues,
            std::move(field),
            "Color contains invalid hexadecimal digits."
        );
        return std::nullopt;
    }

    return RgbColor{
        .red = static_cast<std::uint8_t>((packed >> 16) & 0xFF),
        .green = static_cast<std::uint8_t>((packed >> 8) & 0xFF),
        .blue = static_cast<std::uint8_t>(packed & 0xFF),
    };
}

void read_art_style(
    const YAML::Node& node,
    ArtStyleProfile& style,
    std::vector<ValidationIssue>& issues
) {
    read_required(node, "id", "art_style.id", style.id, issues);
    read_required(
        node,
        "geometry",
        "art_style.geometry",
        style.geometry,
        issues
    );
    read_required(
        node,
        "silhouettes",
        "art_style.silhouettes",
        style.silhouettes,
        issues
    );
    read_required(node, "edges", "art_style.edges", style.edges, issues);
    read_required(
        node,
        "texture_style",
        "art_style.texture_style",
        style.texture_style,
        issues
    );
    read_required(
        node,
        "lighting",
        "art_style.lighting",
        style.lighting,
        issues
    );
    read_required(
        node,
        "realism",
        "art_style.realism",
        style.realism,
        issues
    );

    const YAML::Node palette = node["palette"];
    if (!palette || !palette.IsMap()) {
        add_issue(
            issues,
            "art_style.palette",
            "Required section must be a map."
        );
    } else {
        if (const auto color = parse_hex_color(
                palette["base"],
                "art_style.palette.base",
                issues
            )) {
            style.base_color = *color;
        }
        if (const auto color = parse_hex_color(
                palette["secondary"],
                "art_style.palette.secondary",
                issues
            )) {
            style.secondary_color = *color;
        }
        if (const auto color = parse_hex_color(
                palette["accent"],
                "art_style.palette.accent",
                issues
            )) {
            style.accent_color = *color;
        }
    }

    const YAML::Node forbidden = node["forbidden"];
    if (!forbidden || !forbidden.IsSequence()) {
        add_issue(
            issues,
            "art_style.forbidden",
            "Required field must be a sequence."
        );
    } else {
        for (std::size_t index = 0; index < forbidden.size(); ++index) {
            try {
                style.forbidden.push_back(
                    forbidden[index].as<std::string>()
                );
            } catch (const YAML::Exception&) {
                add_issue(
                    issues,
                    "art_style.forbidden[" + std::to_string(index) + "]",
                    "Forbidden trait must be a string."
                );
            }
        }
    }

    for (const auto& issue : validate(style)) {
        add_issue(issues, "art_style." + issue.field, issue.message);
    }
}

void read_maps(
    const YAML::Node& material,
    MaterialRequest& request,
    std::vector<ValidationIssue>& issues
) {
    const YAML::Node maps = material["maps"];
    if (!maps || !maps.IsSequence()) {
        add_issue(
            issues,
            "material.maps",
            "Required field must be a sequence."
        );
        return;
    }

    for (std::size_t index = 0; index < maps.size(); ++index) {
        const std::string field =
            "material.maps[" + std::to_string(index) + "]";
        try {
            const std::string name = maps[index].as<std::string>();
            const auto parsed = parse_texture_map(name);
            if (!parsed.has_value()) {
                add_issue(
                    issues,
                    field,
                    "Unknown texture map '" + name + "'."
                );
                continue;
            }
            request.maps.push_back(*parsed);
        } catch (const YAML::Exception&) {
            add_issue(issues, field, "Texture map name must be a string.");
        }
    }
}

[[nodiscard]] MaterialLoadResult parse_root(const YAML::Node& root) {
    MaterialLoadResult result;
    MaterialRequest request;
    ArtStyleProfile style;

    if (!root.IsMap()) {
        add_issue(result.issues, "$", "Specification root must be a map.");
        return result;
    }

    const YAML::Node art_style = root["art_style"];
    if (!art_style || !art_style.IsMap()) {
        add_issue(
            result.issues,
            "art_style",
            "Required section must be a map."
        );
    } else {
        read_art_style(art_style, style, result.issues);
        request.style_id = style.id;
    }

    const YAML::Node material = root["material"];
    if (!material || !material.IsMap()) {
        add_issue(
            result.issues,
            "material",
            "Required section must be a map."
        );
        return result;
    }

    read_required(
        material,
        "id",
        "material.id",
        request.id,
        result.issues
    );
    read_required(
        material,
        "category",
        "material.category",
        request.category,
        result.issues
    );

    std::uint64_t resolution = 0;
    if (read_required(
            material,
            "resolution",
            "material.resolution",
            resolution,
            result.issues
        )) {
        if (resolution > std::numeric_limits<std::uint32_t>::max()) {
            add_issue(
                result.issues,
                "material.resolution",
                "Resolution is outside the supported integer range."
            );
        } else {
            request.resolution = static_cast<std::uint32_t>(resolution);
        }
    }

    read_required(
        material,
        "physical_size_meters",
        "material.physical_size_meters",
        request.physical_size_meters,
        result.issues
    );
    read_required(
        material,
        "seamless",
        "material.seamless",
        request.seamless,
        result.issues
    );
    read_required(
        material,
        "mobile_optimized",
        "material.mobile_optimized",
        request.mobile_optimized,
        result.issues
    );
    read_maps(material, request, result.issues);

    const auto validation_issues = validate(request);
    for (const auto& issue : validation_issues) {
        add_issue(
            result.issues,
            "material." + issue.field,
            issue.message
        );
    }
    result.request = std::move(request);
    result.style = std::move(style);
    return result;
}

}  // namespace

MaterialLoadResult load_material_request(
    const std::filesystem::path& specification_path
) {
    try {
        return parse_root(YAML::LoadFile(specification_path.string()));
    } catch (const YAML::BadFile&) {
        MaterialLoadResult result;
        add_issue(
            result.issues,
            "$file",
            "Could not open specification: " + specification_path.string()
        );
        return result;
    } catch (const YAML::ParserException& error) {
        MaterialLoadResult result;
        add_issue(
            result.issues,
            "$yaml",
            "Invalid YAML near line " +
                std::to_string(error.mark.line + 1) +
                ", column " +
                std::to_string(error.mark.column + 1) +
                "."
        );
        return result;
    } catch (const YAML::Exception& error) {
        MaterialLoadResult result;
        add_issue(
            result.issues,
            "$yaml",
            "Could not read specification: " + std::string(error.what())
        );
        return result;
    }
}

}  // namespace hexloom

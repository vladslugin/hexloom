#include "hexloom/godot/hexloom_material_bridge.hpp"

#include "hexloom/core/material_request.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <optional>
#include <string>
#include <string_view>

namespace hexloom::godot_adapter {
namespace {

[[nodiscard]] std::string to_std_string(const godot::Variant& value) {
    return godot::String(value).utf8().get_data();
}

[[nodiscard]] std::optional<TextureMap> parse_map(std::string_view name) {
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

[[nodiscard]] MaterialRequest parse_request(const godot::Dictionary& input) {
    MaterialRequest request;
    request.id = to_std_string(input.get("id", ""));
    request.category = to_std_string(input.get("category", ""));
    request.style_id = to_std_string(input.get("style_id", ""));
    request.resolution = static_cast<std::uint32_t>(
        static_cast<std::int64_t>(input.get("resolution", 0))
    );
    request.physical_size_meters = static_cast<float>(
        static_cast<double>(input.get("physical_size_meters", 0.0))
    );
    request.seamless = static_cast<bool>(input.get("seamless", true));
    request.mobile_optimized =
        static_cast<bool>(input.get("mobile_optimized", true));

    const godot::Array maps = input.get("maps", godot::Array{});
    for (const godot::Variant& value : maps) {
        const auto parsed = parse_map(to_std_string(value));
        if (parsed.has_value()) {
            request.maps.push_back(*parsed);
        }
    }

    return request;
}

}  // namespace

godot::Dictionary HexloomMaterialBridge::validate_material(
    const godot::Dictionary& input
) const {
    const auto request = parse_request(input);
    const auto issues = hexloom::validate(request);

    godot::Array serialized_issues;
    for (const auto& issue : issues) {
        godot::Dictionary serialized_issue;
        serialized_issue["field"] = godot::String(issue.field.c_str());
        serialized_issue["message"] = godot::String(issue.message.c_str());
        serialized_issues.push_back(serialized_issue);
    }

    godot::Dictionary result;
    result["valid"] = issues.empty();
    result["material_id"] = godot::String(request.id.c_str());
    result["style_id"] = godot::String(request.style_id.c_str());
    result["issues"] = serialized_issues;

    godot::UtilityFunctions::print(
        "Hexloom C++ validation: material=",
        result["material_id"],
        " valid=",
        result["valid"]
    );

    return result;
}

void HexloomMaterialBridge::_bind_methods() {
    godot::ClassDB::bind_method(
        godot::D_METHOD("validate_material", "request"),
        &HexloomMaterialBridge::validate_material
    );
}

}  // namespace hexloom::godot_adapter

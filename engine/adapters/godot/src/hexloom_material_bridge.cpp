#include "hexloom/godot/hexloom_material_bridge.hpp"

#include "hexloom/core/material_request.hpp"
#include "hexloom/core/material_spec_loader.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <optional>
#include <filesystem>
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

[[nodiscard]] godot::Dictionary serialize_result(
    const MaterialRequest* request,
    const std::vector<ValidationIssue>& issues
) {
    godot::Array serialized_issues;
    for (const auto& issue : issues) {
        godot::Dictionary serialized_issue;
        serialized_issue["field"] = godot::String(issue.field.c_str());
        serialized_issue["message"] = godot::String(issue.message.c_str());
        serialized_issues.push_back(serialized_issue);
    }

    godot::Dictionary result;
    result["valid"] = request != nullptr && issues.empty();
    result["material_id"] =
        request == nullptr ? godot::String{} : godot::String(request->id.c_str());
    result["style_id"] = request == nullptr
        ? godot::String{}
        : godot::String(request->style_id.c_str());
    result["resolution"] =
        request == nullptr ? 0 : static_cast<std::int64_t>(request->resolution);
    godot::Array maps;
    if (request != nullptr) {
        for (const auto map : request->maps) {
            maps.push_back(godot::String(to_string(map).data()));
        }
    }
    result["maps"] = maps;
    result["issues"] = serialized_issues;
    return result;
}

}  // namespace

godot::Dictionary HexloomMaterialBridge::validate_material(
    const godot::Dictionary& input
) const {
    const auto request = parse_request(input);
    const auto issues = hexloom::validate(request);
    auto result = serialize_result(&request, issues);

    godot::UtilityFunctions::print(
        "Hexloom C++ validation: material=",
        result["material_id"],
        " valid=",
        result["valid"]
    );

    return result;
}

godot::Dictionary HexloomMaterialBridge::validate_material_file(
    const godot::String& path
) const {
    const std::filesystem::path specification_path =
        path.utf8().get_data();
    const auto loaded = load_material_request(specification_path);
    const MaterialRequest* request =
        loaded.request.has_value() ? &*loaded.request : nullptr;
    auto result = serialize_result(request, loaded.issues);
    if (loaded.style.has_value()) {
        godot::Dictionary style;
        style["id"] = godot::String(loaded.style->id.c_str());
        style["geometry"] =
            godot::String(loaded.style->geometry.c_str());
        style["texture_style"] =
            godot::String(loaded.style->texture_style.c_str());
        style["lighting"] =
            godot::String(loaded.style->lighting.c_str());
        style["realism"] = loaded.style->realism;
        result["art_style"] = style;
    }

    godot::UtilityFunctions::print(
        "Hexloom C++ specification: path=",
        path,
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
    godot::ClassDB::bind_method(
        godot::D_METHOD("validate_material_file", "path"),
        &HexloomMaterialBridge::validate_material_file
    );
}

}  // namespace hexloom::godot_adapter

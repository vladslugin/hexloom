#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace hexloom::godot_adapter {

class HexloomMaterialBridge : public godot::RefCounted {
    GDCLASS(HexloomMaterialBridge, godot::RefCounted)

public:
    [[nodiscard]] godot::Dictionary validate_material(
        const godot::Dictionary& input
    ) const;
    [[nodiscard]] godot::Dictionary validate_material_file(
        const godot::String& path
    ) const;

protected:
    static void _bind_methods();
};

}  // namespace hexloom::godot_adapter

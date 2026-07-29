#include "hexloom/godot/hexloom_agent_bridge.hpp"
#include "hexloom/godot/hexloom_material_bridge.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

namespace {

void initialize_hexloom_module(
    godot::ModuleInitializationLevel initialization_level
) {
    if (initialization_level != godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    godot::ClassDB::register_class<
        hexloom::godot_adapter::HexloomMaterialBridge
    >();
    godot::ClassDB::register_class<
        hexloom::godot_adapter::HexloomAgentBridge
    >();
}

void uninitialize_hexloom_module(
    godot::ModuleInitializationLevel initialization_level
) {
    if (initialization_level != godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}

}  // namespace

extern "C" {

GDExtensionBool GDE_EXPORT hexloom_library_init(
    GDExtensionInterfaceGetProcAddress get_proc_address,
    GDExtensionClassLibraryPtr library,
    GDExtensionInitialization* initialization
) {
    godot::GDExtensionBinding::InitObject init_object(
        get_proc_address,
        library,
        initialization
    );

    init_object.register_initializer(initialize_hexloom_module);
    init_object.register_terminator(uninitialize_hexloom_module);
    init_object.set_minimum_library_initialization_level(
        godot::MODULE_INITIALIZATION_LEVEL_SCENE
    );

    return init_object.init();
}

}

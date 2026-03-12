#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include <fstream>
#include <string>
#ifdef _WIN32
#include <cstdlib>
#endif

#include "firered_node.hpp"

using namespace godot;

static void write_extension_trace(const char *message) {
#ifdef _WIN32
    const char *temp_dir = std::getenv("TEMP");
    std::string path = (temp_dir != nullptr ? temp_dir : ".");
    path += "\\pokefirered_gdextension.log";
#else
    std::string path = "/tmp/pokefirered_gdextension.log";
#endif

    std::ofstream out(path.c_str(), std::ios::app);
    if (out.is_open()) {
        out << message << '\n';
    }
}

static void initialize_firered_module(ModuleInitializationLevel level) {
    write_extension_trace("initialize_firered_module: entered");
    if (level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        write_extension_trace("initialize_firered_module: skipped level");
        return;
    }
    write_extension_trace("initialize_firered_module: registering FireRedNode");
    ClassDB::register_class<FireRedNode>();
    write_extension_trace("initialize_firered_module: registered FireRedNode");
}

static void uninitialize_firered_module(ModuleInitializationLevel level) {
    write_extension_trace("uninitialize_firered_module: entered");
    if (level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        write_extension_trace("uninitialize_firered_module: skipped level");
        return;
    }
    write_extension_trace("uninitialize_firered_module: finished");
}

extern "C" GDExtensionBool GDE_EXPORT pokefirered_library_init(
    GDExtensionInterfaceGetProcAddress get_proc_address,
    const GDExtensionClassLibraryPtr library,
    GDExtensionInitialization *initialization) {
    write_extension_trace("pokefirered_library_init: entered");
    GDExtensionBinding::InitObject init_obj(get_proc_address, library, initialization);
    init_obj.register_initializer(initialize_firered_module);
    init_obj.register_terminator(uninitialize_firered_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
    GDExtensionBool result = init_obj.init();
    write_extension_trace(result ? "pokefirered_library_init: success" : "pokefirered_library_init: failure");
    return result;
}

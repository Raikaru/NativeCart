/**
 * @file register_types.cpp
 * @brief Godot GDExtension entry point and type registration
 */

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

// ── Our custom classes ────────────────────────────────────────────────────────
#include "patch_manager.hpp"
#include "gba_renderer.hpp"
#include "fire_red_node.hpp"
// ─────────────────────────────────────────────────────────────────────────────

using namespace godot;

// ---------------------------------------------------------------------------
// Forward declarations for the C-side runtime initialiser
// (implemented in gba_emulator.cpp / gba_c_bridge.c)
// ---------------------------------------------------------------------------
extern "C" void gba_c_runtime_init();
extern "C" void gba_c_runtime_shutdown();

// ---------------------------------------------------------------------------
// Module lifecycle
// ---------------------------------------------------------------------------

void initialize_gba_emulator_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    // Boot the GBA memory subsystem before any Godot objects are created.
    gba_c_runtime_init();

    // ── Register classes with Godot's ClassDB ────────────────────────────────
    //
    // Every class the GDScript layer references by name MUST appear here.
    // Forgetting this registration is the sole cause of the
    //   "Could not find type 'X' in the current scope" errors.
    //
    ClassDB::register_class<PatchManager>();
    ClassDB::register_class<GBARenderer>();
    ClassDB::register_class<FireRedNode>();
    // ─────────────────────────────────────────────────────────────────────────

    UtilityFunctions::print("PokeFireRed GDExtension initialised — PatchManager, GBARenderer & FireRedNode registered");
}

void uninitialize_gba_emulator_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    gba_c_runtime_shutdown();

    UtilityFunctions::print("PokeFireRed GDExtension uninitialised");
}

// ---------------------------------------------------------------------------
// Library entry point
// ---------------------------------------------------------------------------

extern "C" {

GDExtensionBool GDE_EXPORT pokefirered_library_init(
    GDExtensionInterfaceGetProcAddress p_get_proc_address,
    const GDExtensionClassLibraryPtr   p_library,
    GDExtensionInitialization         *r_initialization
) {
    GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

    init_obj.register_initializer(initialize_gba_emulator_module);
    init_obj.register_terminator(uninitialize_gba_emulator_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_obj.init();
}

} // extern "C"

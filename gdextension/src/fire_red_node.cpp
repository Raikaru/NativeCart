/**
 * @file fire_red_node.cpp
 * @brief Implementation of FireRedNode class
 */

// 1. NOMINMAX guard
#define NOMINMAX

// 2. godot-cpp headers (or standard C++ headers)
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/file_access.hpp>

// 3. Custom headers (after godot-cpp)
#include "fire_red_node.hpp"

// 4. Restore min/max macros AFTER godot-cpp headers
// (no pokefirered headers in this file)

// 5. After all pokefirered headers, undefine conflicting macros
#undef min
#undef max
#undef SWAP

namespace godot {

// ============================================================================
// GDExtension Method Bindings
// ============================================================================

void FireRedNode::_bind_methods() {
    // Core functionality
    ClassDB::bind_method(D_METHOD("load_rom", "path"), &FireRedNode::load_rom);
    ClassDB::bind_method(D_METHOD("start_engine"), &FireRedNode::start_engine);
    ClassDB::bind_method(D_METHOD("stop_engine"), &FireRedNode::stop_engine);
    ClassDB::bind_method(D_METHOD("is_running"), &FireRedNode::is_running);
    ClassDB::bind_method(D_METHOD("get_framebuffer"), &FireRedNode::get_framebuffer);
    ClassDB::bind_method(D_METHOD("get_framebuffer_texture"), &FireRedNode::get_framebuffer_texture);
    
    // Configuration
    ClassDB::bind_method(D_METHOD("set_emulation_speed", "speed"), &FireRedNode::set_emulation_speed);
    ClassDB::bind_method(D_METHOD("get_emulation_speed"), &FireRedNode::get_emulation_speed);
    ClassDB::bind_method(D_METHOD("has_rom_loaded"), &FireRedNode::has_rom_loaded);
    ClassDB::bind_method(D_METHOD("reset_engine"), &FireRedNode::reset_engine);
    
    // Property bindings
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "emulation_speed", PROPERTY_HINT_RANGE, "0.1,10.0,0.1"), 
                 "set_emulation_speed", "get_emulation_speed");
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

FireRedNode::FireRedNode() {
    // Allocate local framebuffer
    local_framebuffer = new uint8_t[FRAMEBUFFER_SIZE];
    std::memset(local_framebuffer, 0, FRAMEBUFFER_SIZE);
    
    UtilityFunctions::print("FireRedNode: Created");
}

FireRedNode::~FireRedNode() {
    // Cleanup
    if (engine_running) {
        stop_engine();
    }
    
    // Free framebuffer
    if (local_framebuffer != nullptr) {
        delete[] local_framebuffer;
        local_framebuffer = nullptr;
    }
    
    // Release Godot resources
    framebuffer_texture.unref();
    framebuffer_image.unref();
    
    UtilityFunctions::print("FireRedNode: Destroyed");
}

// ============================================================================
// Godot Lifecycle
// ============================================================================

void FireRedNode::_ready() {
    UtilityFunctions::print("FireRedNode: _ready() called");
    
    // Initialize subsystems
    // Note: gba_c_runtime_init() is called in register_types.cpp during module init
    
    // Initialize framebuffer
    initialize_framebuffer();
    
    // Try to initialize memory and hardware subsystems
    // These are optional - if they don't exist, we'll work with what's available
    #ifdef HAS_GBA_MEMORY
    if (!gba_memory_init()) {
        UtilityFunctions::push_warning("FireRedNode: Memory subsystem initialization failed");
    }
    #endif
    
    #ifdef HAS_GBA_HARDWARE
    if (!gba_hardware_init()) {
        UtilityFunctions::push_warning("FireRedNode: Hardware subsystem initialization failed");
    }
    #endif
}

void FireRedNode::_process(double delta) {
    // Accumulate time for frame timing
    accumulated_time += delta;
    
    // Run GBA frame(s) if engine is running
    if (engine_running) {
        // Run at target frame rate
        while (accumulated_time >= target_frame_time) {
            run_single_frame();
            accumulated_time -= target_frame_time;
        }
        
        // Update framebuffer texture
        update_framebuffer();
    }
}

void FireRedNode::_exit_tree() {
    UtilityFunctions::print("FireRedNode: _exit_tree() called");
    
    // Stop engine if running
    if (engine_running) {
        stop_engine();
    }
    
    // Shutdown subsystems
    #ifdef HAS_GBA_MEMORY
    gba_memory_shutdown();
    #endif
    
    #ifdef HAS_GBA_HARDWARE
    gba_hardware_shutdown();
    #endif
}

// ============================================================================
// Public API
// ============================================================================

bool FireRedNode::load_rom(String path) {
    UtilityFunctions::print("FireRedNode: Loading ROM from ", path);
    
    // Stop engine if running
    if (engine_running) {
        stop_engine();
    }
    
    // Open file using Godot's FileAccess
    Ref<FileAccess> file = FileAccess::open(path, FileAccess::ModeFlags::READ);
    
    if (file.is_null()) {
        UtilityFunctions::push_error("FireRedNode: Failed to open ROM file: ", path);
        return false;
    }
    
    // Get file size
    size_t file_size = file->get_length();
    if (file_size == 0) {
        UtilityFunctions::push_error("FireRedNode: ROM file is empty");
        return false;
    }
    
    UtilityFunctions::print("FireRedNode: ROM size: ", String::num_int64(file_size), " bytes");
    
    // Read file data
    PackedByteArray rom_data = file->get_buffer(file_size);
    file->close();
    
    if (rom_data.size() == 0) {
        UtilityFunctions::push_error("FireRedNode: Failed to read ROM data");
        return false;
    }
    
    // Load ROM into GBA memory
    bool success = gba_load_rom(rom_data.ptr(), rom_data.size());
    
    if (success) {
        rom_loaded = true;
        UtilityFunctions::print("FireRedNode: ROM loaded successfully");
        
        // Initialize assets from ROM
        gba_assets_init(rom_data.ptr(), rom_data.size());
    } else {
        rom_loaded = false;
        UtilityFunctions::push_error("FireRedNode: Failed to load ROM into GBA memory");
    }
    
    return success;
}

void FireRedNode::start_engine() {
    UtilityFunctions::print("FireRedNode: Starting engine");
    
    if (!rom_loaded) {
        UtilityFunctions::push_warning("FireRedNode: Cannot start engine - no ROM loaded");
        return;
    }
    
    engine_running = true;
    accumulated_time = 0.0;
    
    UtilityFunctions::print("FireRedNode: Engine started");
}

void FireRedNode::stop_engine() {
    UtilityFunctions::print("FireRedNode: Stopping engine");
    
    engine_running = false;
    
    UtilityFunctions::print("FireRedNode: Engine stopped");
}

bool FireRedNode::is_running() {
    // Check both local flag and hardware status (if available)
    bool hw_running = false;
    
    #ifdef HAS_GBA_HARDWARE
    hw_running = gba_is_running();
    #endif
    
    return engine_running || hw_running;
}

Ref<Image> FireRedNode::get_framebuffer() {
    if (!engine_running || local_framebuffer == nullptr) {
        return Ref<Image>();
    }
    
    // Create Image from framebuffer data
    // GBA framebuffer is RGBA8 format
    PackedByteArray pixel_data;
    pixel_data.resize(FRAMEBUFFER_SIZE);
    
    // Copy framebuffer data
    std::memcpy(pixel_data.ptrw(), local_framebuffer, FRAMEBUFFER_SIZE);
    
    // Create Image
    Ref<Image> image = Image::create_from_data(
        GBA_WIDTH, 
        GBA_HEIGHT, 
        false,  // no mipmaps
        Image::FORMAT_RGBA8,
        pixel_data
    );
    
    return image;
}

Ref<ImageTexture> FireRedNode::get_framebuffer_texture() {
    return framebuffer_texture;
}

// ============================================================================
// Configuration
// ============================================================================

void FireRedNode::set_emulation_speed(double speed) {
    if (speed <= 0.0) {
        speed = 0.1;
    }
    
    target_frame_time = 1.0 / (60.0 * speed);
}

double FireRedNode::get_emulation_speed() const {
    return 1.0 / (target_frame_time * 60.0);
}

void FireRedNode::reset_engine() {
    UtilityFunctions::print("FireRedNode: Resetting engine");
    
    // Stop engine
    stop_engine();
    
    // Reset framebuffer
    if (local_framebuffer != nullptr) {
        std::memset(local_framebuffer, 0, FRAMEBUFFER_SIZE);
    }
    
    // Clear accumulated time
    accumulated_time = 0.0;
    
    UtilityFunctions::print("FireRedNode: Engine reset");
}

// ============================================================================
// Private Helpers
// ============================================================================

void FireRedNode::initialize_framebuffer() {
    UtilityFunctions::print("FireRedNode: Initializing framebuffer");
    
    // Create initial black image
    PackedByteArray pixel_data;
    pixel_data.resize(FRAMEBUFFER_SIZE);
    std::memset(pixel_data.ptrw(), 0, FRAMEBUFFER_SIZE);
    
    framebuffer_image = Image::create_from_data(
        GBA_WIDTH,
        GBA_HEIGHT,
        false,
        Image::FORMAT_RGBA8,
        pixel_data
    );
    
    // Create texture from image
    framebuffer_texture = ImageTexture::create_from_image(framebuffer_image);
    
    UtilityFunctions::print("FireRedNode: Framebuffer initialized (", 
                           String::num_int64(GBA_WIDTH), "x", 
                           String::num_int64(GBA_HEIGHT), ")");
}

void FireRedNode::update_framebuffer() {
    if (local_framebuffer == nullptr || framebuffer_texture.is_null()) {
        return;
    }
    
    // Get framebuffer from hardware if available
    uint8_t* hw_framebuffer = nullptr;
    
    #ifdef HAS_GBA_HARDWARE
    hw_framebuffer = gba_get_framebuffer();
    #endif
    
    if (hw_framebuffer != nullptr) {
        // Copy hardware framebuffer to local buffer
        std::memcpy(local_framebuffer, hw_framebuffer, FRAMEBUFFER_SIZE);
    }
    
    // Update image with new data
    PackedByteArray pixel_data;
    pixel_data.resize(FRAMEBUFFER_SIZE);
    std::memcpy(pixel_data.ptrw(), local_framebuffer, FRAMEBUFFER_SIZE);
    
    framebuffer_image->set_data(
        GBA_WIDTH,
        GBA_HEIGHT,
        false,
        Image::FORMAT_RGBA8,
        pixel_data
    );
    
    // Update texture
    framebuffer_texture->update(framebuffer_image);
}

void FireRedNode::run_single_frame() {
    // Run one frame of emulation
    #ifdef HAS_GBA_HARDWARE
    gba_run_frame();
    #else
    // Fallback: if hardware subsystem not available, 
    // we rely on other mechanisms to update the framebuffer
    
    // Try to get framebuffer directly from VRAM/palette
    // TODO: Implement gba_get_vram() and gba_get_palette() functions
    // uint8_t* vram = gba_get_vram();
    // uint8_t* palette = gba_get_palette();
    
    // For now, just render a test pattern
    render_simple_framebuffer(nullptr, nullptr);
    #endif
}

// Simple framebuffer renderer (fallback when hardware subsystem unavailable)
void FireRedNode::render_simple_framebuffer(uint8_t* vram, uint8_t* palette) {
    // This is a basic renderer that creates a test pattern
    // In production, this would use the full GBA rendering pipeline
    
    // NOTE: vram and palette are currently unused - this is a placeholder
    // that generates a test pattern until the real GBA renderer is implemented
    (void)vram;
    (void)palette;
    
    for (int y = 0; y < GBA_HEIGHT; y++) {
        for (int x = 0; x < GBA_WIDTH; x++) {
            int idx = (y * GBA_WIDTH + x) * 4;
            
            // Simple test pattern (will be replaced by real renderer)
            local_framebuffer[idx + 0] = (x * 255) / GBA_WIDTH;     // R
            local_framebuffer[idx + 1] = (y * 255) / GBA_HEIGHT;    // G
            local_framebuffer[idx + 2] = 128;                        // B
            local_framebuffer[idx + 3] = 255;                        // A
        }
    }
}

} // namespace godot

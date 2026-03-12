/**
 * @file fire_red_node.hpp
 * @brief Main Godot Node for running the FireRed GBA runtime
 * 
 * This class provides the bridge between Godot's scene tree and the
 * GBA emulation runtime. It handles ROM loading, frame execution,
 * and framebuffer access for GDScript.
 * 
 * Example GDScript usage:
 * ```gdscript
 * extends Node2D
 * 
 * @onready var fire_red = $FireRedNode
 * @onready var sprite = $Sprite2D
 * 
 * func _ready():
 *     # Load the ROM file
 *     if fire_red.load_rom("res://roms/pokefirered.gba"):
 *         print("ROM loaded successfully!")
 *         fire_red.start_engine()
 *     else:
 *         print("Failed to load ROM")
 * 
 * func _process(delta):
 *     # The engine runs automatically in _process, but you can check status
 *     if fire_red.is_running():
 *         # Get the current framebuffer as a Godot Image
 *         var fb = fire_red.get_framebuffer()
 *         if fb:
 *             # Update a Sprite2D texture
 *             var tex = ImageTexture.create_from_image(fb)
 *             sprite.texture = tex
 * ```
 */

#ifndef FIRE_RED_NODE_HPP
#define FIRE_RED_NODE_HPP

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/variant/string.hpp>
#include <cstdint>

// Forward declarations for C bridge functions
extern "C" {
    // Memory initialization (Agent 3)
    bool gba_memory_init(void);
    void gba_memory_shutdown(void);
    uint8_t* gba_memory_get_ewram(void);
    uint8_t* gba_memory_get_iwram(void);
    uint8_t* gba_memory_get_vram(void);
    uint8_t* gba_memory_get_palette(void);
    uint8_t* gba_memory_get_oam(void);
    
    // Hardware initialization (Agent 4)
    bool gba_hardware_init(void);
    void gba_hardware_shutdown(void);
    void gba_run_frame(void);
    bool gba_is_running(void);
    uint8_t* gba_get_framebuffer(void);
    int gba_get_framebuffer_width(void);
    int gba_get_framebuffer_height(void);
    
    // Asset initialization (from gba_c_bridge.cpp)
    void gba_assets_init(const uint8_t* rom_buffer, size_t rom_size);
    void gba_assets_shutdown(void);
    
    // ROM loading (from gba_c_bridge.cpp)
    bool gba_load_rom(const uint8_t* romData, size_t size);
}

namespace godot {

/**
 * FireRedNode - Main interface between Godot and the GBA runtime
 * 
 * This node handles:
 * - ROM loading and initialization
 * - Frame-by-frame execution
 * - Framebuffer retrieval for rendering
 */
class FireRedNode : public Node {
    GDCLASS(FireRedNode, Node)

private:
    // Engine state
    bool engine_running = false;
    bool rom_loaded = false;
    
    // Framebuffer data (240x160 RGBA8)
    static constexpr int GBA_WIDTH = 240;
    static constexpr int GBA_HEIGHT = 160;
    static constexpr int FRAMEBUFFER_SIZE = GBA_WIDTH * GBA_HEIGHT * 4;
    
    uint8_t* local_framebuffer = nullptr;
    Ref<Image> framebuffer_image;
    Ref<ImageTexture> framebuffer_texture;
    
    // Target framerate (GBA runs at 60 FPS)
    double target_frame_time = 1.0 / 60.0;
    double accumulated_time = 0.0;

protected:
    /**
     * @brief Bind methods for GDScript access
     */
    static void _bind_methods();

public:
    FireRedNode();
    ~FireRedNode();

    // Godot lifecycle overrides
    void _ready() override;
    void _process(double delta) override;
    void _exit_tree() override;

    // =========================================================================
    // GDScript-exposed methods
    // =========================================================================
    
    /**
     * @brief Load a ROM file from disk
     * @param path Path to the .gba ROM file (can be res:// or user://)
     * @return true if ROM loaded successfully, false otherwise
     */
    bool load_rom(String path);
    
    /**
     * @brief Start the GBA engine execution
     * Call this after load_rom() to begin emulation
     */
    void start_engine();
    
    /**
     * @brief Stop the GBA engine execution
     */
    void stop_engine();
    
    /**
     * @brief Check if the engine is currently running
     * @return true if running, false otherwise
     */
    bool is_running();
    
    /**
     * @brief Get the current framebuffer as a Godot Image
     * @return Ref<Image> containing the current frame (240x160 RGBA8)
     *         Returns null reference if engine not running
     */
    Ref<Image> get_framebuffer();
    
    /**
     * @brief Get the framebuffer as an ImageTexture for direct use
     * @return Ref<ImageTexture> containing the current frame
     */
    Ref<ImageTexture> get_framebuffer_texture();

    // =========================================================================
    // Configuration methods
    // =========================================================================
    
    /**
     * @brief Set the target emulation speed
     * @param speed Speed multiplier (1.0 = normal, 2.0 = double speed)
     */
    void set_emulation_speed(double speed);
    
    /**
     * @brief Get the current emulation speed
     * @return Current speed multiplier
     */
    double get_emulation_speed() const;
    
    /**
     * @brief Check if a ROM is currently loaded
     * @return true if ROM loaded, false otherwise
     */
    bool has_rom_loaded() const { return rom_loaded; }
    
    /**
     * @brief Reset the GBA engine (reloads ROM if available)
     */
    void reset_engine();

private:
    // Internal helper methods
    void initialize_framebuffer();
    void update_framebuffer();
    void run_single_frame();
    void render_simple_framebuffer(uint8_t* vram, uint8_t* palette);
};

} // namespace godot

#endif // FIRE_RED_NODE_HPP

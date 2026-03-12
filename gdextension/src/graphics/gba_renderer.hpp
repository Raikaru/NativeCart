#ifndef GBA_RENDERER_HPP
#define GBA_RENDERER_HPP

// Undefine min/max macros if they were defined by C headers
// (these conflict with C++ std::min/max)
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <cstdint>
#include <array>
#include <vector>

namespace godot {

/**
 * GBA Graphics Renderer Bridge for Godot 4.x
 * 
 * Converts GBA tiled rendering + sprites into Godot-compatible RGBA8 framebuffer.
 * Integrates with pokefirered's rendering system.
 */
class GBARenderer : public Node {
    GDCLASS(GBARenderer, Node)

public:
    // GBA Display Constants
    static constexpr int GBA_WIDTH = 240;
    static constexpr int GBA_HEIGHT = 160;
    static constexpr int GBA_PIXEL_COUNT = GBA_WIDTH * GBA_HEIGHT;
    
    // GBA Memory Sizes
    static constexpr size_t GBA_VRAM_SIZE = 96 * 1024;        // 96KB VRAM
    static constexpr size_t GBA_PALETTE_SIZE = 1024;          // 1KB Palette RAM (512 × 16-bit)
    static constexpr size_t GBA_OAM_SIZE = 128 * 8;           // 128 sprites × 8 bytes
    static constexpr int MAX_SPRITES = 128;
    static constexpr int MAX_BG_LAYERS = 4;
    static constexpr int PALETTE_ENTRIES = 512;           // 256 BG + 256 OBJ
    
    // Background Modes
    enum class BGMode {
        MODE_0 = 0,  // 4 tiled backgrounds
        MODE_1 = 1,  // 3 backgrounds, one affine
        MODE_2 = 2,  // 2 affine backgrounds
        MODE_3 = 3,  // Bitmap mode 1 (not used by pokefirered typically)
        MODE_4 = 4,  // Bitmap mode 2
        MODE_5 = 5   // Bitmap mode 3
    };

    // Background Control Register bits
    enum class BGControl : uint16_t {
        PRIORITY_MASK = 0x0003,
        CHAR_BASE_MASK = 0x000C,      // Character base block (bits 2-3)
        MOSAIC = 0x0040,
        COLOR_256 = 0x0080,           // 0=16 colors, 1=256 colors
        SCREEN_BASE_MASK = 0x1F00,    // Screen base block (bits 8-12)
        WRAP = 0x2000,                // Affine wrap
        SIZE_MASK = 0xC000            // Screen size (bits 14-15)
    };

    // OAM Attribute bits
    enum class OAMAttr : uint16_t {
        Y_COORD_MASK = 0x00FF,        // Y coordinate (0-255)
        ROT_SCALE = 0x0100,           // Rotation/Scaling on
        DISABLE_DOUBLE = 0x0200,      // OBJ disable or Double-size
        OBJ_MODE_MASK = 0x0C00,       // OBJ mode (bits 10-11)
        MOSAIC = 0x1000,
        COLOR_256 = 0x2000,           // 0=16 colors, 1=256 colors
        SHAPE_MASK = 0xC000           // OBJ shape (bits 14-15)
    };

    // Sprite shapes
    enum class SpriteShape : uint8_t {
        SQUARE = 0,
        HORIZONTAL = 1,
        VERTICAL = 2
    };

    // Sprite sizes (combined with shape)
    enum class SpriteSize : uint8_t {
        SIZE_8x8 = 0,      // Square: 8x8, H: 16x8, V: 8x16
        SIZE_16x16 = 1,    // Square: 16x16, H: 32x8, V: 8x32
        SIZE_32x32 = 2,    // Square: 32x32, H: 32x16, V: 16x32
        SIZE_64x64 = 3     // Square: 64x64, H: 64x32, V: 32x64
    };

    // Display Control Register
    struct DisplayControl {
        uint16_t mode : 3;            // BG mode (0-5)
        uint16_t frame_select : 1;    // Frame select for bitmap modes
        uint16_t hblank_free : 1;     // HBlank interval free
        uint16_t obj_char_mapping : 1;// 0=2D, 1=1D
        uint16_t forced_blank : 1;    // Forced blank
        uint16_t bg0_enable : 1;
        uint16_t bg1_enable : 1;
        uint16_t bg2_enable : 1;
        uint16_t bg3_enable : 1;
        uint16_t obj_enable : 1;
        uint16_t win0_enable : 1;
        uint16_t win1_enable : 1;
        uint16_t obj_win_enable : 1;
    };

    // Background Control Register structure
    struct BGControlReg {
        uint16_t priority : 2;        // Priority (0-3, lower=higher)
        uint16_t char_base : 2;       // Character base block
        uint16_t reserved : 2;
        uint16_t mosaic : 1;
        uint16_t color_256 : 1;       // Color depth
        uint16_t screen_base : 5;     // Screen base block
        uint16_t affine_wrap : 1;     // Affine wrapping
        uint16_t size : 2;            // Screen size
        int16_t x_offset;             // Horizontal scroll offset
        int16_t y_offset;             // Vertical scroll offset
    };

    // OAM Entry (8 bytes)
    struct OAMEntry {
        union {
            struct {
                uint16_t attr0;       // Y coord, rot/scale, mode, etc.
                uint16_t attr1;       // X coord, rot/scale params, size
                uint16_t attr2;       // Tile number, priority, palette
                int16_t affine_param; // Affine parameter or padding
            };
            uint8_t raw[8];
        };
    };

    // Background reference structure
    struct Background {
        BGControlReg control;
        int16_t x_offset;
        int16_t y_offset;
        bool enabled;
        int priority;
    };

private:
    // Godot resources
    Ref<ImageTexture> frame_texture;
    Ref<Image> frame_image;
    RID texture_rid;
    
    // GBA memory pointers (to be set from pokefirered)
    const uint8_t* vram_ptr = nullptr;          // 96KB VRAM
    const uint16_t* palette_ptr = nullptr;      // 512 palette entries
    const OAMEntry* oam_ptr = nullptr;          // 128 OAM entries
    const DisplayControl* dispcnt_ptr = nullptr;// Display control
    const BGControlReg* bgcnt_ptr = nullptr;    // 4 BG control registers
    
    // Local framebuffer (RGBA8)
    std::vector<uint8_t> framebuffer;
    
    // Palette cache (RGBA8)
    std::array<uint32_t, PALETTE_ENTRIES> palette_cache;
    bool palette_dirty = true;
    
    // VBlank tracking
    uint64_t frame_count = 0;
    bool vblank_pending = false;
    
    // Rendering state
    bool initialized = false;
    bool rendering_enabled = true;

protected:
    static void _bind_methods();

public:
    GBARenderer();
    ~GBARenderer();

    // Node lifecycle
    void _ready() override;
    void _process(double delta) override;
    void _exit_tree() override;

    // Initialization
    void initialize();
    bool is_initialized() const { return initialized; }
    
    // Memory pointer setup (called from pokefirered)
    void set_vram_pointer(const uint8_t* ptr);
    void set_palette_pointer(const uint16_t* ptr);
    void set_oam_pointer(const OAMEntry* ptr);
    void set_display_control_pointer(const DisplayControl* ptr);
    void set_bg_control_pointer(const BGControlReg* ptr);
    
    // VBlank handling
    void signal_vblank();
    bool is_vblank_pending() const { return vblank_pending; }
    uint64_t get_frame_count() const { return frame_count; }
    
    // Main rendering entry point
    void render_frame();
    void update_godot_texture();
    void push_frame(const uint8_t* data, int width, int height);
    
    // Individual layer rendering
    void render_background_mode0();
    void render_background_mode1();
    void render_background_mode2();
    void render_sprites();
    
    // Palette management
    void update_palette_cache();
    void invalidate_palette() { palette_dirty = true; }
    
    // Color conversion
    static uint32_t gba_to_rgba8(uint16_t gba_color);
    
    // Tile rendering helpers
    void render_4bpp_tile(int bg_id, int screen_x, int screen_y, 
                          uint16_t tile_index, uint16_t palette_bank);
    void render_8bpp_tile(int bg_id, int screen_x, int screen_y, 
                          uint16_t tile_index);
    void render_sprite_4bpp(int screen_x, int screen_y, int width, int height,
                           uint16_t tile_index, uint16_t palette_bank, 
                           bool hflip, bool vflip);
    void render_sprite_8bpp(int screen_x, int screen_y, int width, int height,
                           uint16_t tile_index, bool hflip, bool vflip);
    
    // Priority sorting
    void sort_bg_by_priority();
    void sort_sprites_by_priority();
    
    // Texture access
    Ref<ImageTexture> get_frame_texture() const { return frame_texture; }
    RID get_texture_rid() const { return texture_rid; }
    
    // Control
    void set_rendering_enabled(bool enabled) { rendering_enabled = enabled; }
    bool get_rendering_enabled() const { return rendering_enabled; }
    
    // Debug utilities
    void debug_dump_vram(const String& filename);
    void debug_dump_palette(const String& filename);
    void debug_dump_oam(const String& filename);
    
    // Constants getters for GDScript
    int get_gba_width() const { return GBA_WIDTH; }
    int get_gba_height() const { return GBA_HEIGHT; }
};

} // namespace godot

#endif // GBA_RENDERER_HPP

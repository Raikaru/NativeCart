#include "gba_renderer.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <cstring>
#include <algorithm>

namespace godot {

void GBARenderer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("initialize"), &GBARenderer::initialize);
    ClassDB::bind_method(D_METHOD("is_initialized"), &GBARenderer::is_initialized);

    ClassDB::bind_method(D_METHOD("signal_vblank"), &GBARenderer::signal_vblank);
    ClassDB::bind_method(D_METHOD("is_vblank_pending"), &GBARenderer::is_vblank_pending);
    ClassDB::bind_method(D_METHOD("get_frame_count"), &GBARenderer::get_frame_count);

    ClassDB::bind_method(D_METHOD("render_frame"), &GBARenderer::render_frame);
    ClassDB::bind_method(D_METHOD("update_godot_texture"), &GBARenderer::update_godot_texture);

    ClassDB::bind_method(D_METHOD("get_frame_texture"), &GBARenderer::get_frame_texture);
    ClassDB::bind_method(D_METHOD("get_texture_rid"), &GBARenderer::get_texture_rid);

    ClassDB::bind_method(D_METHOD("set_rendering_enabled", "enabled"), &GBARenderer::set_rendering_enabled);
    ClassDB::bind_method(D_METHOD("get_rendering_enabled"), &GBARenderer::get_rendering_enabled);

    ClassDB::bind_method(D_METHOD("invalidate_palette"), &GBARenderer::invalidate_palette);

    ClassDB::bind_method(D_METHOD("get_gba_width"), &GBARenderer::get_gba_width);
    ClassDB::bind_method(D_METHOD("get_gba_height"), &GBARenderer::get_gba_height);

    ClassDB::bind_method(D_METHOD("debug_dump_vram", "filename"), &GBARenderer::debug_dump_vram);
    ClassDB::bind_method(D_METHOD("debug_dump_palette", "filename"), &GBARenderer::debug_dump_palette);
    ClassDB::bind_method(D_METHOD("debug_dump_oam", "filename"), &GBARenderer::debug_dump_oam);
}

GBARenderer::GBARenderer() {
    framebuffer.resize(GBA_WIDTH * GBA_HEIGHT * 4); // RGBA8
    palette_cache.fill(0);
}

GBARenderer::~GBARenderer() {
    if (texture_rid.is_valid()) {
        RenderingServer::get_singleton()->free_rid(texture_rid);
    }
}

void GBARenderer::_ready() {
    initialize();
}

void GBARenderer::_process(double delta) {
    // If VBlank signal received, render and update
    if (vblank_pending && rendering_enabled) {
        vblank_pending = false;
        render_frame();
        update_godot_texture();
        frame_count++;
    }
}

void GBARenderer::_exit_tree() {
    if (texture_rid.is_valid()) {
        RenderingServer::get_singleton()->free_rid(texture_rid);
        texture_rid = RID();
    }
    initialized = false;
}

void GBARenderer::initialize() {
    if (initialized) {
        return;
    }

    // Create Image for framebuffer
    PackedByteArray image_data;
    image_data.resize(GBA_WIDTH * GBA_HEIGHT * 4);
    
    frame_image = Image::create_from_data(
        GBA_WIDTH,
        GBA_HEIGHT,
        false,
        Image::FORMAT_RGBA8,
        image_data
    );

    // Create texture via RenderingServer for optimal performance
    RenderingServer* rs = RenderingServer::get_singleton();
    
    texture_rid = rs->texture_2d_create(frame_image);
    
    // Also create ImageTexture wrapper for GDScript access
    frame_texture = ImageTexture::create_from_image(frame_image);

    // Initialize framebuffer to black
    std::fill(framebuffer.begin(), framebuffer.end(), 0);

    initialized = true;
    UtilityFunctions::print("GBARenderer: Initialized GBA renderer bridge");
}

void GBARenderer::set_vram_pointer(const uint8_t* ptr) {
    vram_ptr = ptr;
}

void GBARenderer::set_palette_pointer(const uint16_t* ptr) {
    palette_ptr = ptr;
    palette_dirty = true;
}

void GBARenderer::set_oam_pointer(const OAMEntry* ptr) {
    oam_ptr = ptr;
}

void GBARenderer::set_display_control_pointer(const DisplayControl* ptr) {
    dispcnt_ptr = ptr;
}

void GBARenderer::set_bg_control_pointer(const BGControlReg* ptr) {
    bgcnt_ptr = ptr;
}

void GBARenderer::signal_vblank() {
    vblank_pending = true;
}

void GBARenderer::render_frame() {
    if (!initialized || !dispcnt_ptr || !vram_ptr) {
        return;
    }

    // Update palette cache if dirty
    if (palette_dirty && palette_ptr) {
        update_palette_cache();
    }

    // Clear framebuffer to backdrop color (palette entry 0)
    uint32_t backdrop_color = palette_cache[0];
    uint8_t* fb = framebuffer.data();
    for (int i = 0; i < GBA_WIDTH * GBA_HEIGHT; ++i) {
        std::memcpy(&fb[i * 4], &backdrop_color, 4);
    }

    // Check forced blank
    if (dispcnt_ptr->forced_blank) {
        return;
    }

    // Render based on mode
    BGMode mode = static_cast<BGMode>(dispcnt_ptr->mode);
    
    // Render backgrounds first (lower priority = higher z-index, rendered first)
    switch (mode) {
        case BGMode::MODE_0:
            render_background_mode0();
            break;
        case BGMode::MODE_1:
            render_background_mode1();
            break;
        case BGMode::MODE_2:
            render_background_mode2();
            break;
        default:
            // Bitmap modes - not typically used by pokefirered
            break;
    }

    // Render sprites on top (with priority handling)
    if (dispcnt_ptr->obj_enable && oam_ptr) {
        render_sprites();
    }
}

void GBARenderer::render_background_mode0() {
    if (!bgcnt_ptr) return;

    // Mode 0: 4 tiled backgrounds
    // Render backgrounds in priority order (3 to 0, higher priority rendered later)
    for (int priority = 3; priority >= 0; --priority) {
        for (int bg = 0; bg < 4; ++bg) {
            const BGControlReg& bgcnt = bgcnt_ptr[bg];
            
            // Check if this BG is enabled and has current priority
            if (bgcnt.priority != priority) continue;
            
            switch (bg) {
                case 0: if (!dispcnt_ptr->bg0_enable) continue; break;
                case 1: if (!dispcnt_ptr->bg1_enable) continue; break;
                case 2: if (!dispcnt_ptr->bg2_enable) continue; break;
                case 3: if (!dispcnt_ptr->bg3_enable) continue; break;
            }

            // Calculate base addresses
            uint32_t char_base = bgcnt.char_base * 0x4000;      // 16KB blocks
            uint32_t screen_base = bgcnt.screen_base * 0x800;    // 2KB blocks
            
            bool use_256_colors = bgcnt.color_256;
            int screen_size = bgcnt.size;
            
            // Screen dimensions in tiles
            int screen_width_tiles = 32;
            int screen_height_tiles = 32;
            
            switch (screen_size) {
                case 1: screen_width_tiles = 64; screen_height_tiles = 32; break;
                case 2: screen_width_tiles = 32; screen_height_tiles = 64; break;
                case 3: screen_width_tiles = 64; screen_height_tiles = 64; break;
            }

            // Render this background
            // Simple implementation: render visible portion
            for (int tile_y = 0; tile_y < 21; ++tile_y) {  // 160/8 + 1
                for (int tile_x = 0; tile_x < 31; ++tile_x) {  // 240/8 + 1
                    // Calculate screen position with scroll
                    int screen_tile_x = (tile_x + (bgcnt_ptr[bg].x_offset >> 3)) & (screen_width_tiles - 1);
                    int screen_tile_y = (tile_y + (bgcnt_ptr[bg].y_offset >> 3)) & (screen_height_tiles - 1);
                    
                    // Get tilemap entry
                    uint32_t tilemap_offset = screen_base + (screen_tile_y * screen_width_tiles + screen_tile_x) * 2;
                    if (tilemap_offset < GBA_VRAM_SIZE - 1) {
                        uint16_t tile_entry = *reinterpret_cast<const uint16_t*>(vram_ptr + tilemap_offset);
                        
                        uint16_t tile_index = tile_entry & 0x3FF;
                        uint16_t hflip = tile_entry & 0x0400;
                        uint16_t vflip = tile_entry & 0x0800;
                        uint16_t palette_bank = (tile_entry >> 12) & 0xF;
                        
                        int screen_x = tile_x * 8 - (bgcnt_ptr[bg].x_offset & 7);
                        int screen_y = tile_y * 8 - (bgcnt_ptr[bg].y_offset & 7);
                        
                        if (use_256_colors) {
                            render_8bpp_tile(bg, screen_x, screen_y, tile_index);
                        } else {
                            render_4bpp_tile(bg, screen_x, screen_y, tile_index, palette_bank);
                        }
                    }
                }
            }
        }
    }
}

void GBARenderer::render_background_mode1() {
    // Mode 1: 3 backgrounds (0,1=tiled, 2=affine)
    // For now, render BG0 and BG1 as in mode 0
    // BG2 would need affine transformation
    render_background_mode0();  // Simplified
}

void GBARenderer::render_background_mode2() {
    // Mode 2: 2 affine backgrounds
    // Not fully implemented - would require matrix math
    // For now, render nothing or minimal implementation
}

void GBARenderer::render_4bpp_tile(int bg_id, int screen_x, int screen_y, 
                                   uint16_t tile_index, uint16_t palette_bank) {
    if (!vram_ptr || !bgcnt_ptr) return;
    
    uint32_t char_base = bgcnt_ptr[bg_id].char_base * 0x4000;
    uint32_t tile_offset = char_base + tile_index * 32;  // 4bpp = 32 bytes per tile
    
    if (tile_offset >= GBA_VRAM_SIZE - 32) return;
    
    uint8_t* fb = framebuffer.data();
    const uint8_t* tile_data = vram_ptr + tile_offset;
    
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 4; ++x) {
            uint8_t pixel_pair = tile_data[y * 4 + x];
            uint8_t pixel_low = pixel_pair & 0xF;
            uint8_t pixel_high = pixel_pair >> 4;
            
            int dst_x = screen_x + x * 2;
            int dst_y = screen_y + y;
            
            if (dst_x >= 0 && dst_x < GBA_WIDTH && dst_y >= 0 && dst_y < GBA_HEIGHT) {
                if (pixel_low != 0) {  // Color 0 is transparent
                    uint32_t color = palette_cache[pixel_low + palette_bank * 16];
                    std::memcpy(&fb[(dst_y * GBA_WIDTH + dst_x) * 4], &color, 4);
                }
                if (pixel_high != 0) {
                    uint32_t color = palette_cache[pixel_high + palette_bank * 16];
                    std::memcpy(&fb[(dst_y * GBA_WIDTH + dst_x + 1) * 4], &color, 4);
                }
            }
        }
    }
}

void GBARenderer::render_8bpp_tile(int bg_id, int screen_x, int screen_y, 
                                   uint16_t tile_index) {
    if (!vram_ptr || !bgcnt_ptr) return;
    
    uint32_t char_base = bgcnt_ptr[bg_id].char_base * 0x4000;
    uint32_t tile_offset = char_base + tile_index * 64;  // 8bpp = 64 bytes per tile
    
    if (tile_offset >= GBA_VRAM_SIZE - 64) return;
    
    uint8_t* fb = framebuffer.data();
    const uint8_t* tile_data = vram_ptr + tile_offset;
    
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            uint8_t pixel = tile_data[y * 8 + x];
            
            int dst_x = screen_x + x;
            int dst_y = screen_y + y;
            
            if (dst_x >= 0 && dst_x < GBA_WIDTH && dst_y >= 0 && dst_y < GBA_HEIGHT) {
                if (pixel != 0) {  // Color 0 is transparent
                    uint32_t color = palette_cache[pixel];
                    std::memcpy(&fb[(dst_y * GBA_WIDTH + dst_x) * 4], &color, 4);
                }
            }
        }
    }
}

void GBARenderer::render_sprites() {
    if (!oam_ptr || !dispcnt_ptr) return;
    
    // Render sprites by priority (3 to 0, higher priority last = on top)
    for (int priority = 3; priority >= 0; --priority) {
        for (int i = 0; i < MAX_SPRITES; ++i) {
            const OAMEntry& sprite = oam_ptr[i];
            
            // Parse OAM attributes
            uint16_t attr0 = sprite.attr0;
            uint16_t attr1 = sprite.attr1;
            uint16_t attr2 = sprite.attr2;
            
            // Check if sprite should be drawn
            if (attr0 & 0x0200) continue;  // OBJ disable
            
            int sprite_priority = (attr2 >> 10) & 0x3;
            if (sprite_priority != priority) continue;
            
            // Extract position
            int y_pos = attr0 & 0xFF;
            int x_pos = attr1 & 0x1FF;
            
            // Handle coordinate wrapping
            if (y_pos >= 160) y_pos -= 256;
            if (x_pos >= 240) x_pos -= 512;
            
            // Extract size and shape
            int shape = (attr0 >> 14) & 0x3;
            int size = (attr1 >> 14) & 0x3;
            
            // Determine dimensions based on shape and size
            int width, height;
            switch (shape) {
                case 0: // Square
                    switch (size) {
                        case 0: width = height = 8; break;
                        case 1: width = height = 16; break;
                        case 2: width = height = 32; break;
                        case 3: width = height = 64; break;
                    }
                    break;
                case 1: // Horizontal
                    switch (size) {
                        case 0: width = 16; height = 8; break;
                        case 1: width = 32; height = 8; break;
                        case 2: width = 32; height = 16; break;
                        case 3: width = 64; height = 32; break;
                    }
                    break;
                case 2: // Vertical
                    switch (size) {
                        case 0: width = 8; height = 16; break;
                        case 1: width = 8; height = 32; break;
                        case 2: width = 16; height = 32; break;
                        case 3: width = 32; height = 64; break;
                    }
                    break;
                default:
                    continue;
            }
            
            bool hflip = attr1 & 0x1000;
            bool vflip = attr1 & 0x2000;
            bool use_256_colors = attr0 & 0x2000;
            
            uint16_t tile_index = attr2 & 0x3FF;
            uint16_t palette_bank = (attr2 >> 12) & 0xF;
            
            // Render sprite
            if (use_256_colors) {
                render_sprite_8bpp(x_pos, y_pos, width, height, tile_index, hflip, vflip);
            } else {
                render_sprite_4bpp(x_pos, y_pos, width, height, tile_index, palette_bank, hflip, vflip);
            }
        }
    }
}

void GBARenderer::render_sprite_4bpp(int screen_x, int screen_y, int width, int height,
                                     uint16_t tile_index, uint16_t palette_bank,
                                     bool hflip, bool vflip) {
    if (!vram_ptr) return;
    
    uint8_t* fb = framebuffer.data();
    
    // OBJ tiles are at 0x10000 in VRAM (or different based on mapping mode)
    uint32_t obj_tile_base = dispcnt_ptr->obj_char_mapping ? 0x10000 : 0x10000;
    
    int tiles_per_row = dispcnt_ptr->obj_char_mapping ? (width / 8) : 32;
    
    for (int y = 0; y < height; ++y) {
        int src_y = vflip ? (height - 1 - y) : y;
        int tile_row = src_y / 8;
        int sub_y = src_y % 8;
        
        for (int x = 0; x < width; ++x) {
            int src_x = hflip ? (width - 1 - x) : x;
            int tile_col = src_x / 8;
            int sub_x = src_x % 8;
            
            uint32_t tile_num = tile_index + tile_row * tiles_per_row + tile_col;
            uint32_t tile_offset = obj_tile_base + tile_num * 32 + sub_y * 4 + (sub_x / 2);
            
            if (tile_offset >= GBA_VRAM_SIZE) continue;
            
            uint8_t pixel_pair = vram_ptr[tile_offset];
            uint8_t pixel;
            if (sub_x & 1) {
                pixel = pixel_pair >> 4;
            } else {
                pixel = pixel_pair & 0xF;
            }
            
            int dst_x = screen_x + x;
            int dst_y = screen_y + y;
            
            if (dst_x >= 0 && dst_x < GBA_WIDTH && dst_y >= 0 && dst_y < GBA_HEIGHT) {
                if (pixel != 0) {  // Transparent color
                    uint32_t color = palette_cache[256 + pixel + palette_bank * 16];  // OBJ palette
                    std::memcpy(&fb[(dst_y * GBA_WIDTH + dst_x) * 4], &color, 4);
                }
            }
        }
    }
}

void GBARenderer::render_sprite_8bpp(int screen_x, int screen_y, int width, int height,
                                     uint16_t tile_index, bool hflip, bool vflip) {
    if (!vram_ptr) return;
    
    uint8_t* fb = framebuffer.data();
    
    uint32_t obj_tile_base = 0x10000;
    int tiles_per_row = dispcnt_ptr->obj_char_mapping ? (width / 8) : 32;
    
    for (int y = 0; y < height; ++y) {
        int src_y = vflip ? (height - 1 - y) : y;
        int tile_row = src_y / 8;
        int sub_y = src_y % 8;
        
        for (int x = 0; x < width; ++x) {
            int src_x = hflip ? (width - 1 - x) : x;
            int tile_col = src_x / 8;
            int sub_x = src_x % 8;
            
            uint32_t tile_num = tile_index + tile_row * tiles_per_row + tile_col;
            uint32_t tile_offset = obj_tile_base + tile_num * 64 + sub_y * 8 + sub_x;
            
            if (tile_offset >= GBA_VRAM_SIZE) continue;
            
            uint8_t pixel = vram_ptr[tile_offset];
            
            int dst_x = screen_x + x;
            int dst_y = screen_y + y;
            
            if (dst_x >= 0 && dst_x < GBA_WIDTH && dst_y >= 0 && dst_y < GBA_HEIGHT) {
                if (pixel != 0) {  // Transparent color
                    uint32_t color = palette_cache[256 + pixel];  // OBJ palette 256-511
                    std::memcpy(&fb[(dst_y * GBA_WIDTH + dst_x) * 4], &color, 4);
                }
            }
        }
    }
}

void GBARenderer::update_palette_cache() {
    if (!palette_ptr) return;
    
    for (int i = 0; i < PALETTE_ENTRIES; ++i) {
        palette_cache[i] = gba_to_rgba8(palette_ptr[i]);
    }
    
    palette_dirty = false;
}

uint32_t GBARenderer::gba_to_rgba8(uint16_t gba_color) {
    // GBA BGR555 format: 0bbbbbgggggrrrrr
    uint8_t r = (gba_color & 0x1F) << 3;
    uint8_t g = ((gba_color >> 5) & 0x1F) << 3;
    uint8_t b = ((gba_color >> 10) & 0x1F) << 3;
    uint8_t a = 0xFF;
    
    // Scale 5-bit to 8-bit (duplicate top bits)
    r |= r >> 5;
    g |= g >> 5;
    b |= b >> 5;
    
    return (a << 24) | (b << 16) | (g << 8) | r;
}

void GBARenderer::update_godot_texture() {
    if (!initialized || !frame_image.is_valid()) {
        return;
    }

    // Update Image data
    PackedByteArray image_data;
    image_data.resize(framebuffer.size());
    memcpy(image_data.ptrw(), framebuffer.data(), framebuffer.size());
    
    frame_image->set_data(
        GBA_WIDTH,
        GBA_HEIGHT,
        false,
        Image::FORMAT_RGBA8,
        image_data
    );
    
    // Update RenderingServer texture directly for efficiency
    if (texture_rid.is_valid()) {
        RenderingServer::get_singleton()->texture_2d_update(
            texture_rid,
            frame_image,
            0
        );
    }
    
    // Update ImageTexture wrapper
    if (frame_texture.is_valid()) {
        frame_texture->update(frame_image);
    }
}

void GBARenderer::debug_dump_vram(const String& filename) {
    if (!vram_ptr) return;
    
    Ref<FileAccess> file = FileAccess::open(filename, FileAccess::WRITE);
    if (file.is_valid()) {
        file->store_buffer(vram_ptr, GBA_VRAM_SIZE);
        file->close();
        UtilityFunctions::print("GBARenderer: VRAM dumped to ", filename);
    }
}

void GBARenderer::debug_dump_palette(const String& filename) {
    if (!palette_ptr) return;
    
    Ref<FileAccess> file = FileAccess::open(filename, FileAccess::WRITE);
    if (file.is_valid()) {
        file->store_buffer(reinterpret_cast<const uint8_t*>(palette_ptr), GBA_PALETTE_SIZE);
        file->close();
        UtilityFunctions::print("GBARenderer: Palette dumped to ", filename);
    }
}

void GBARenderer::debug_dump_oam(const String& filename) {
    if (!oam_ptr) return;
    
    Ref<FileAccess> file = FileAccess::open(filename, FileAccess::WRITE);
    if (file.is_valid()) {
        file->store_buffer(reinterpret_cast<const uint8_t*>(oam_ptr), GBA_OAM_SIZE);
        file->close();
        UtilityFunctions::print("GBARenderer: OAM dumped to ", filename);
    }
}

void GBARenderer::sort_bg_by_priority() {
    // Not implemented - backgrounds are rendered in priority order inline
}

void GBARenderer::sort_sprites_by_priority() {
    // Not implemented - sprites are rendered by priority in render_sprites()
}

void GBARenderer::push_frame(const uint8_t* data, int width, int height) {
    if (!initialized || !data || width <= 0 || height <= 0) {
        return;
    }

    // Ensure dimensions match GBA resolution
    if (width != GBA_WIDTH || height != GBA_HEIGHT) {
        // Could add scaling here, but for now just use top-left portion
        width = std::min(width, GBA_WIDTH);
        height = std::min(height, GBA_HEIGHT);
    }

    // Copy data to framebuffer
    size_t row_size = width * 4;  // RGBA8 = 4 bytes per pixel
    uint8_t* fb = framebuffer.data();

    for (int y = 0; y < height; ++y) {
        memcpy(&fb[y * GBA_WIDTH * 4], &data[y * width * 4], row_size);
    }

    // Update the Godot texture
    update_godot_texture();
}

} // namespace godot

#include "engine_internal.h"

#include "global.h"
#include "palette.h"
#include "scanline_effect.h"

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern void engine_backend_trace_external(const char *message);
extern unsigned long engine_backend_get_completed_frame_external(void);
extern uint16_t gPlttBufferFaded[];

#ifdef NDEBUG
#define ENGINE_RENDERER_TRACE_MSG(msg) ((void)0)
#define ENGINE_RENDERER_TRACEF(...) ((void)0)
#else
#define ENGINE_RENDERER_TRACE_MSG(msg) engine_backend_trace_external(msg)
static void engine_renderer_tracef_impl(const char *fmt, ...)
{
    char buffer[128];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    ENGINE_RENDERER_TRACE_MSG(buffer);
}
#define ENGINE_RENDERER_TRACEF(...) engine_renderer_tracef_impl(__VA_ARGS__)
#endif

#define ENGINE_PALETTE_ENTRIES 512
#define ENGINE_MAX_SPRITES 128

enum {
    ENGINE_LAYER_BG0,
    ENGINE_LAYER_BG1,
    ENGINE_LAYER_BG2,
    ENGINE_LAYER_BG3,
    ENGINE_LAYER_OBJ,
    ENGINE_LAYER_BACKDROP,
};

typedef struct EngineLayerPixel {
    uint32_t color;
    uint8_t kind;
    uint8_t valid;
    uint8_t semi_transparent;
} EngineLayerPixel;

#define ENGINE_PIXEL_LAYER_CAPACITY 8

typedef struct EnginePixelLayers {
    EngineLayerPixel layers[ENGINE_PIXEL_LAYER_CAPACITY];
    uint8_t count;
} EnginePixelLayers;

static uint8_t g_framebuffer[ENGINE_GBA_RGBA_SIZE];
static uint32_t g_palette_cache[ENGINE_PALETTE_ENTRIES];
static uint16_t g_palette_snapshot[ENGINE_PALETTE_ENTRIES];
static EnginePixelLayers g_pixel_layers[ENGINE_GBA_WIDTH * ENGINE_GBA_HEIGHT];
static uint8_t g_obj_window_mask_cache[ENGINE_GBA_WIDTH * ENGINE_GBA_HEIGHT];
static uint8_t g_window_mask_cache[ENGINE_GBA_WIDTH * ENGINE_GBA_HEIGHT];
static uint8_t g_has_semi_transparent_obj;
#ifdef PORTABLE
static unsigned long g_trainer_card_trace_start_frame = (unsigned long)-1;
static unsigned long g_trainer_card_mask_logged_frame = (unsigned long)-1;
static uint8_t g_trainer_card_mask_logged_points = 0;
#endif

static volatile uint16_t *engine_reg16(uint32_t addr) {
    return (volatile uint16_t *)(uintptr_t)addr;
}

static uint8_t *engine_vram(void) {
    return (uint8_t *)(uintptr_t)ENGINE_VRAM_ADDR;
}

static uint16_t *engine_palette(void) {
    return (uint16_t *)(uintptr_t)ENGINE_PALETTE_ADDR;
}

static EngineOAMEntry *engine_oam(void) {
    return (EngineOAMEntry *)(uintptr_t)ENGINE_OAM_ADDR;
}

static uint16_t engine_bgcnt_value(int bg) {
    return *engine_reg16(ENGINE_REG_BG0CNT + (uint32_t)(bg * 2));
}

static int engine_bg_hofs(int bg) {
    return (int)(*engine_reg16(ENGINE_REG_BG0HOFS + (uint32_t)(bg * 4)) & 0x1FFu);
}

static int engine_bg_vofs(int bg) {
    return (int)(*engine_reg16(ENGINE_REG_BG0VOFS + (uint32_t)(bg * 4)) & 0x1FFu);
}

static void engine_put_color(int x, int y, uint32_t rgba) {
    size_t index;

    if (x < 0 || x >= ENGINE_GBA_WIDTH || y < 0 || y >= ENGINE_GBA_HEIGHT) {
        return;
    }

    index = (size_t)(y * ENGINE_GBA_WIDTH + x) * 4u;
    g_framebuffer[index + 0] = (uint8_t)(rgba & 0xFFu);
    g_framebuffer[index + 1] = (uint8_t)((rgba >> 8) & 0xFFu);
    g_framebuffer[index + 2] = (uint8_t)((rgba >> 16) & 0xFFu);
    g_framebuffer[index + 3] = (uint8_t)((rgba >> 24) & 0xFFu);
}

static uint32_t engine_gba_to_rgba(uint16_t color) {
    uint8_t r = (uint8_t)((color & 0x1Fu) << 3);
    uint8_t g = (uint8_t)(((color >> 5) & 0x1Fu) << 3);
    uint8_t b = (uint8_t)(((color >> 10) & 0x1Fu) << 3);

    r = (uint8_t)(r | (r >> 5));
    g = (uint8_t)(g | (g >> 5));
    b = (uint8_t)(b | (b >> 5));

    return ((uint32_t)0xFFu << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | (uint32_t)r;
}

static void engine_update_palette_cache(void) {
    int i;
#ifdef PORTABLE
    uint16_t *palette = gPlttBufferFaded;
#else
    uint16_t *palette = engine_palette();
#endif

    ENGINE_RENDERER_TRACE_MSG("palette_cache: enter");

    if (memcmp(g_palette_snapshot, palette, sizeof(g_palette_snapshot)) == 0) {
        ENGINE_RENDERER_TRACE_MSG("palette_cache: exit");
        return;
    }

    memcpy(g_palette_snapshot, palette, sizeof(g_palette_snapshot));

    for (i = 0; i < ENGINE_PALETTE_ENTRIES; ++i) {
        g_palette_cache[i] = engine_gba_to_rgba(palette[i]);
    }

    ENGINE_RENDERER_TRACE_MSG("palette_cache: exit");
}

static void engine_clear_framebuffer(uint32_t rgba) {
    uint32_t *fb = (uint32_t *)(void *)g_framebuffer;
    size_t i;

    ENGINE_RENDERER_TRACE_MSG("clear_framebuffer: enter");

    for (i = 0; i < ENGINE_GBA_WIDTH * ENGINE_GBA_HEIGHT; ++i) {
        fb[i] = rgba;
    }

    ENGINE_RENDERER_TRACE_MSG("clear_framebuffer: exit");
}

static uint32_t engine_blend_alpha(uint32_t top, uint32_t bottom, int eva, int evb) {
    uint32_t r = (((top & 0xFFu) * (uint32_t)eva) + ((bottom & 0xFFu) * (uint32_t)evb)) >> 4;
    uint32_t g = ((((top >> 8) & 0xFFu) * (uint32_t)eva) + (((bottom >> 8) & 0xFFu) * (uint32_t)evb)) >> 4;
    uint32_t b = ((((top >> 16) & 0xFFu) * (uint32_t)eva) + (((bottom >> 16) & 0xFFu) * (uint32_t)evb)) >> 4;
    if (r > 255u) r = 255u;
    if (g > 255u) g = 255u;
    if (b > 255u) b = 255u;
    return 0xFF000000u | (b << 16) | (g << 8) | r;
}

static uint32_t engine_blend_brighten(uint32_t color, int evy) {
    uint32_t r = (color & 0xFFu) + (((255u - (color & 0xFFu)) * (uint32_t)evy) >> 4);
    uint32_t g = ((color >> 8) & 0xFFu) + (((255u - ((color >> 8) & 0xFFu)) * (uint32_t)evy) >> 4);
    uint32_t b = ((color >> 16) & 0xFFu) + (((255u - ((color >> 16) & 0xFFu)) * (uint32_t)evy) >> 4);
    if (r > 255u) r = 255u;
    if (g > 255u) g = 255u;
    if (b > 255u) b = 255u;
    return 0xFF000000u | (b << 16) | (g << 8) | r;
}

static uint32_t engine_blend_darken(uint32_t color, int evy) {
    uint32_t r = (color & 0xFFu) - (((color & 0xFFu) * (uint32_t)evy) >> 4);
    uint32_t g = ((color >> 8) & 0xFFu) - ((((color >> 8) & 0xFFu) * (uint32_t)evy) >> 4);
    uint32_t b = ((color >> 16) & 0xFFu) - ((((color >> 16) & 0xFFu) * (uint32_t)evy) >> 4);
    return 0xFF000000u | (b << 16) | (g << 8) | r;
}

static void engine_sprite_dimensions(int shape, int size, int *width, int *height) {
    switch (shape) {
    case 0:
        *width = (size == 0) ? 8 : (size == 1) ? 16 : (size == 2) ? 32 : 64;
        *height = *width;
        break;
    case 1:
        *width = (size == 0) ? 16 : (size == 1) ? 32 : (size == 2) ? 32 : 64;
        *height = (size == 0) ? 8 : (size == 1) ? 8 : (size == 2) ? 16 : 32;
        break;
    case 2:
        *width = (size == 0) ? 8 : (size == 1) ? 8 : (size == 2) ? 16 : 32;
        *height = (size == 0) ? 16 : (size == 1) ? 32 : (size == 2) ? 32 : 64;
        break;
    default:
        *width = 0;
        *height = 0;
        break;
    }
}

static int engine_sprite_is_affine(uint16_t attr0) {
    return (attr0 & 0x0100u) != 0;
}

static int engine_sprite_is_disabled(uint16_t attr0) {
    return !engine_sprite_is_affine(attr0) && (attr0 & 0x0200u) != 0;
}

static int engine_sprite_is_double_size(uint16_t attr0) {
    return (attr0 & 0x0300u) == 0x0300u;
}

static int engine_sprite_hflip(uint16_t attr0, uint16_t attr1) {
    if (engine_sprite_is_affine(attr0)) {
        return 0;
    }

    return (attr1 & 0x1000u) != 0;
}

static int engine_sprite_vflip(uint16_t attr0, uint16_t attr1) {
    if (engine_sprite_is_affine(attr0)) {
        return 0;
    }

    return (attr1 & 0x2000u) != 0;
}

static void engine_sprite_draw_bounds(uint16_t attr0, int x_pos, int y_pos, int width, int height,
                                      int *draw_x, int *draw_y, int *draw_width, int *draw_height) {
    *draw_x = x_pos;
    *draw_y = y_pos;
    *draw_width = width;
    *draw_height = height;

    if (engine_sprite_is_affine(attr0) && engine_sprite_is_double_size(attr0)) {
        *draw_x -= width / 2;
        *draw_y -= height / 2;
        *draw_width *= 2;
        *draw_height *= 2;
    }
}

static void engine_load_sprite_affine_matrix(const EngineOAMEntry *oam, uint16_t attr1,
                                             int32_t *pa, int32_t *pb, int32_t *pc, int32_t *pd) {
    int matrix_num = (attr1 >> 9) & 0x1F;
    int base = matrix_num * 4;

    *pa = oam[base + 0].affine_param;
    *pb = oam[base + 1].affine_param;
    *pc = oam[base + 2].affine_param;
    *pd = oam[base + 3].affine_param;
}

static int engine_sprite_source_coords_for_pixel(const EngineOAMEntry *oam,
                                                 uint16_t attr0,
                                                 uint16_t attr1,
                                                 int x_pos,
                                                 int y_pos,
                                                 int width,
                                                 int height,
                                                 int screen_x,
                                                 int screen_y,
                                                 int *src_x,
                                                 int *src_y) {
    int draw_x;
    int draw_y;
    int draw_width;
    int draw_height;

    engine_sprite_draw_bounds(attr0, x_pos, y_pos, width, height, &draw_x, &draw_y, &draw_width, &draw_height);
    if (screen_x < draw_x || screen_x >= draw_x + draw_width || screen_y < draw_y || screen_y >= draw_y + draw_height) {
        return 0;
    }

    if (!engine_sprite_is_affine(attr0)) {
        int hflip = engine_sprite_hflip(attr0, attr1);
        int vflip = engine_sprite_vflip(attr0, attr1);
        int local_x = screen_x - draw_x;
        int local_y = screen_y - draw_y;

        *src_x = hflip ? (width - 1 - local_x) : local_x;
        *src_y = vflip ? (height - 1 - local_y) : local_y;
        return 1;
    }

    {
        int32_t pa;
        int32_t pb;
        int32_t pc;
        int32_t pd;
        int center_x = x_pos + width / 2;
        int center_y = y_pos + height / 2;
        int rel_x = screen_x - center_x;
        int rel_y = screen_y - center_y;
        int32_t tex_x;
        int32_t tex_y;

        engine_load_sprite_affine_matrix(oam, attr1, &pa, &pb, &pc, &pd);

        tex_x = pa * rel_x + pb * rel_y + ((int32_t)width << 7);
        tex_y = pc * rel_x + pd * rel_y + ((int32_t)height << 7);
        if (tex_x < 0 || tex_y < 0 || tex_x >= ((int32_t)width << 8) || tex_y >= ((int32_t)height << 8)) {
            return 0;
        }

        *src_x = (int)(tex_x >> 8);
        *src_y = (int)(tex_y >> 8);
        return 1;
    }
}

static int engine_sample_sprite_pixel(int obj_1d,
                                      int use_256_colors,
                                      uint16_t tile_index,
                                      uint16_t palette_bank,
                                      int width,
                                      int height,
                                      int src_x,
                                      int src_y,
                                      uint32_t *rgba_out) {
    uint8_t *vram = engine_vram();
    int tiles_per_row = obj_1d ? (width / 8) : (use_256_colors ? 16 : 32);

    if (src_x < 0 || src_y < 0 || src_x >= width || src_y >= height) {
        return 0;
    }
    int tile_stride = use_256_colors ? 2 : 1;
    int tile_col = src_x / 8;
    int tile_row = src_y / 8;
    int sub_x = src_x % 8;
    int sub_y = src_y % 8;
    uint32_t tile_num = (uint32_t)tile_index + (uint32_t)((tile_row * tiles_per_row + tile_col) * tile_stride);

    if (use_256_colors) {
        uint32_t tile_offset = 0x10000u + tile_num * 32u + (uint32_t)(sub_y * 8 + sub_x);
        uint8_t pixel;

        if (tile_offset >= ENGINE_VRAM_SIZE) {
            return 0;
        }

        pixel = vram[tile_offset];
        if (pixel == 0) {
            return 0;
        }

        if (rgba_out != NULL) {
            *rgba_out = g_palette_cache[256u + pixel];
        }
        return 1;
    }

    {
        uint32_t tile_offset = 0x10000u + tile_num * 32u + (uint32_t)(sub_y * 4 + (sub_x / 2));
        uint8_t pair;
        uint8_t pixel;

        if (tile_offset >= ENGINE_VRAM_SIZE) {
            return 0;
        }

        pair = vram[tile_offset];
        pixel = (sub_x & 1) ? (uint8_t)(pair >> 4) : (uint8_t)(pair & 0x0Fu);
        if (pixel == 0) {
            return 0;
        }

        if (rgba_out != NULL) {
            *rgba_out = g_palette_cache[256u + palette_bank * 16u + pixel];
        }
        return 1;
    }
}

static int engine_point_in_window_rect(int x, int y, uint16_t winh, uint16_t winv) {
    int left = (winh >> 8) & 0xFF;
    int right = winh & 0xFF;
    int top = (winv >> 8) & 0xFF;
    int bottom = winv & 0xFF;
    int inside_x;
    int inside_y;

    if (left <= right)
        inside_x = (x >= left && x < right);
    else
        inside_x = (x >= left || x < right);

    if (top <= bottom)
        inside_y = (y >= top && y < bottom);
    else
        inside_y = (y >= top || y < bottom);

    return inside_x && inside_y;
}

static void engine_precompute_obj_window_mask(uint16_t dispcnt) {
    EngineOAMEntry *oam = engine_oam();
    int obj_1d = (dispcnt & ENGINE_DISPCNT_OBJ_1D_MAP) != 0;
    int i;
    int x;
    int y;

    memset(g_obj_window_mask_cache, 0, sizeof(g_obj_window_mask_cache));
    if ((dispcnt & ENGINE_DISPCNT_OBJWIN_ON) == 0) {
        return;
    }

    for (i = 0; i < ENGINE_MAX_SPRITES; ++i) {
        EngineOAMEntry sprite = oam[i];
        uint16_t attr0 = sprite.attr0;
        uint16_t attr1 = sprite.attr1;
        uint16_t attr2 = sprite.attr2;
        int x_pos;
        int y_pos;
        int shape;
        int size;
        int width;
        int height;
        int src_x;
        int src_y;
        int draw_x;
        int draw_y;
        int draw_width;
        int draw_height;
        int start_x;
        int start_y;
        int end_x;
        int end_y;
        int use_256_colors;
        uint16_t tile_index;
        uint16_t palette_bank;

        if (engine_sprite_is_disabled(attr0))
            continue;
        if ((attr0 & 0x0C00u) != 0x0800u)
            continue;
        if (attr0 == 0u && attr1 == 0u && attr2 == 0u)
            continue;
        y_pos = attr0 & 0xFF;
        x_pos = (int)(attr1 & 0x1FFu);
        if (y_pos >= 160)
            y_pos -= 256;
        /* GBA OBJ X is 9-bit; bit 8 selects negative coordinates (256..511 -> -256..-1). */
        if (x_pos >= 256) {
            x_pos -= 512;
        }

        shape = (attr0 >> 14) & 0x3;
        size = (attr1 >> 14) & 0x3;
        engine_sprite_dimensions(shape, size, &width, &height);
        if (width == 0 || height == 0)
            continue;

        use_256_colors = (attr0 & 0x2000u) != 0;
        tile_index = (uint16_t)(attr2 & 0x03FFu);
        palette_bank = (uint16_t)((attr2 >> 12) & 0x0Fu);

        engine_sprite_draw_bounds(attr0, x_pos, y_pos, width, height, &draw_x, &draw_y, &draw_width, &draw_height);
        start_x = (draw_x < 0) ? 0 : draw_x;
        start_y = (draw_y < 0) ? 0 : draw_y;
        end_x = draw_x + draw_width;
        end_y = draw_y + draw_height;
        if (end_x > ENGINE_GBA_WIDTH)
            end_x = ENGINE_GBA_WIDTH;
        if (end_y > ENGINE_GBA_HEIGHT)
            end_y = ENGINE_GBA_HEIGHT;

        if (start_x >= end_x || start_y >= end_y)
            continue;

        for (y = start_y; y < end_y; ++y) {
            for (x = start_x; x < end_x; ++x) {
                size_t index;

                if (!engine_sprite_source_coords_for_pixel(oam, attr0, attr1, x_pos, y_pos, width, height, x, y, &src_x, &src_y)) {
                    continue;
                }
                if (!engine_sample_sprite_pixel(obj_1d, use_256_colors, tile_index, palette_bank, width, height, src_x, src_y, NULL)) {
                    continue;
                }

                index = (size_t)y * ENGINE_GBA_WIDTH + (size_t)x;
                g_obj_window_mask_cache[index] = 1;
            }
        }
    }
}

#ifdef PORTABLE
static int engine_should_trace_trainer_card_samples(unsigned long frame) {
    if (g_trainer_card_trace_start_frame == (unsigned long)-1)
        return 0;

    return frame + 1u >= g_trainer_card_trace_start_frame && frame < g_trainer_card_trace_start_frame + 8u;
}

static int engine_is_trainer_card_sample_point(int x, int y) {
    return (x == 8 && y == 8)
        || (x == 9 && y == 15)
        || (x == 80 && y == 80)
        || (x == 120 && y == 80)
        || (x == 180 && y == 80)
        || (x == 192 && y == 52)
        || (x == 200 && y == 120);
}

static uint8_t engine_trainer_card_point_bit(int x, int y) {
    if (x == 8 && y == 8)
        return 1u << 0;
    if (x == 9 && y == 15)
        return 1u << 1;
    if (x == 80 && y == 80)
        return 1u << 2;
    if (x == 120 && y == 80)
        return 1u << 3;
    if (x == 180 && y == 80)
        return 1u << 4;
    if (x == 192 && y == 52)
        return 1u << 5;
    if (x == 200 && y == 120)
        return 1u << 6;
    return 0;
}

static void engine_trace_trainer_card_window_mask(int x, int y,
                                                   uint16_t dispcnt,
                                                   uint16_t win0h,
                                                   uint16_t win1h,
                                                   uint16_t win0v,
                                                   uint16_t win1v,
                                                   uint16_t winin,
                                                   uint16_t winout,
                                                   const char *source,
                                                   uint8_t mask) {
    unsigned long frame = engine_backend_get_completed_frame_external();
    uint16_t bg0cnt = *engine_reg16(ENGINE_REG_BG0CNT);
    char buffer[256];
    uint8_t point_bit;

    if (dispcnt != 0x3F40u || bg0cnt != 0x9B02u)
        return;
    if (!engine_should_trace_trainer_card_samples(frame))
        return;
    point_bit = engine_trainer_card_point_bit(x, y);
    if (point_bit == 0)
        return;
    if (frame != g_trainer_card_mask_logged_frame) {
        g_trainer_card_mask_logged_frame = frame;
        g_trainer_card_mask_logged_points = 0;
    }
    if (g_trainer_card_mask_logged_points & point_bit)
        return;
    g_trainer_card_mask_logged_points |= point_bit;

    snprintf(buffer, sizeof(buffer),
             "TWMASK frame=%lu x=%d y=%d src=%s mask=%02X win0h=%04X win0v=%04X win1h=%04X win1v=%04X winin=%04X winout=%04X",
             frame, x, y, source, mask, win0h, win0v, win1h, win1v, winin, winout);
    ENGINE_RENDERER_TRACE_MSG(buffer);
}
#endif

static uint8_t engine_compute_window_mask_for_pixel(int x, int y,
                                                    uint16_t dispcnt,
                                                    uint16_t win0h,
                                                    uint16_t win1h,
                                                    uint16_t win0v,
                                                    uint16_t win1v,
                                                    uint16_t winin,
                                                    uint16_t winout) {
    int win0_enabled = (dispcnt & ENGINE_DISPCNT_WIN0_ON) != 0;
    int win1_enabled = (dispcnt & ENGINE_DISPCNT_WIN1_ON) != 0;
    int objwin_enabled = (dispcnt & ENGINE_DISPCNT_OBJWIN_ON) != 0;

    if (!win0_enabled && !win1_enabled && !objwin_enabled) {
#ifdef PORTABLE
        engine_trace_trainer_card_window_mask(x, y, dispcnt, win0h, win1h, win0v, win1v, winin, winout, "NONE", 0x3Fu);
#endif
        return 0x3Fu;
    }

    if (win0_enabled && engine_point_in_window_rect(x, y, win0h, win0v)) {
        uint8_t mask = (uint8_t)(winin & 0x3Fu);
#ifdef PORTABLE
        engine_trace_trainer_card_window_mask(x, y, dispcnt, win0h, win1h, win0v, win1v, winin, winout, "WIN0", mask);
#endif
        return mask;
    }

    if (win1_enabled && engine_point_in_window_rect(x, y, win1h, win1v)) {
        uint8_t mask = (uint8_t)((winin >> 8) & 0x3Fu);
#ifdef PORTABLE
        engine_trace_trainer_card_window_mask(x, y, dispcnt, win0h, win1h, win0v, win1v, winin, winout, "WIN1", mask);
#endif
        return mask;
    }

    if (objwin_enabled && g_obj_window_mask_cache[(size_t)y * ENGINE_GBA_WIDTH + (size_t)x]) {
        uint8_t mask = (uint8_t)((winout >> 8) & 0x3Fu);
#ifdef PORTABLE
        engine_trace_trainer_card_window_mask(x, y, dispcnt, win0h, win1h, win0v, win1v, winin, winout, "OBJWIN", mask);
#endif
        return mask;
    }

    {
        uint8_t mask = (uint8_t)(winout & 0x3Fu);
#ifdef PORTABLE
        engine_trace_trainer_card_window_mask(x, y, dispcnt, win0h, win1h, win0v, win1v, winin, winout, "WINOUT", mask);
#endif
        return mask;
    }
}

static uint8_t engine_target_bit_for_kind(uint8_t kind) {
    switch (kind) {
    case ENGINE_LAYER_BG0: return 1u << 0;
    case ENGINE_LAYER_BG1: return 1u << 1;
    case ENGINE_LAYER_BG2: return 1u << 2;
    case ENGINE_LAYER_BG3: return 1u << 3;
    case ENGINE_LAYER_OBJ: return 1u << 4;
    default: return 1u << 5;
    }
}

static void engine_trace_trainer_card_layer_pixel(int x, int y, uint32_t color, uint8_t kind) {
#ifdef PORTABLE
    static unsigned long s_logged_frame = (unsigned long)-1;
    static uint8_t s_logged_kind_mask = 0;
    static const char *const s_layer_names[] = {
        "BG0",
        "BG1",
        "BG2",
        "BG3",
        "OBJ",
        "BACKDROP",
    };
    unsigned long frame = engine_backend_get_completed_frame_external();
    uint16_t dispcnt = *engine_reg16(ENGINE_REG_DISPCNT);
    uint16_t bg0cnt = *engine_reg16(ENGINE_REG_BG0CNT);
    char buffer[160];

    if (dispcnt != 0x3F40u || bg0cnt != 0x9B02u)
        return;
    if (g_trainer_card_trace_start_frame == (unsigned long)-1 && kind == ENGINE_LAYER_BG0 && x == 8 && y == 8)
        g_trainer_card_trace_start_frame = frame;
    if (kind > ENGINE_LAYER_BACKDROP)
        return;
    if (frame != s_logged_frame) {
        s_logged_frame = frame;
        s_logged_kind_mask = 0;
    }
    if (s_logged_kind_mask & (1u << kind))
        return;
    if (x < 8 || x >= 232 || y < 8 || y >= 152)
        return;

    s_logged_kind_mask |= (uint8_t)(1u << kind);
    snprintf(buffer, sizeof(buffer), "TCPIX frame=%lu layer=%s x=%d y=%d color=%08lX",
             frame,
             s_layer_names[kind],
             x,
             y,
             (unsigned long)color);
    ENGINE_RENDERER_TRACE_MSG(buffer);
#else
    (void)x;
    (void)y;
    (void)color;
    (void)kind;
#endif
}

static void engine_insert_layer_pixel(int x, int y, uint32_t color, uint8_t kind, uint8_t semi_transparent) {
    size_t index;
    EnginePixelLayers *pixel_layers;

    if (x < 0 || y < 0 || x >= ENGINE_GBA_WIDTH || y >= ENGINE_GBA_HEIGHT) {
        return;
    }

    index = (size_t)y * ENGINE_GBA_WIDTH + (size_t)x;
    pixel_layers = &g_pixel_layers[index];
    EngineLayerPixel *layer;

    engine_trace_trainer_card_layer_pixel(x, y, color, kind);

    if (pixel_layers->count >= ENGINE_PIXEL_LAYER_CAPACITY) {
        memmove(&pixel_layers->layers[0],
                &pixel_layers->layers[1],
                sizeof(pixel_layers->layers[0]) * (ENGINE_PIXEL_LAYER_CAPACITY - 1));
        pixel_layers->count = ENGINE_PIXEL_LAYER_CAPACITY - 1;
    }

    layer = &pixel_layers->layers[pixel_layers->count++];
    layer->color = color;
    layer->kind = kind;
    layer->valid = 1;
    layer->semi_transparent = semi_transparent;
}

/* GBA BG2X/BG2Y: 19.8 fixed point, 27-bit signed payload in bits 0-26 (sign bit 26). */
static int32_t engine_read_bg_affine_ref(uint32_t reg_lo_addr) {
    uint32_t lo = (uint32_t)*engine_reg16(reg_lo_addr);
    uint32_t hi = (uint32_t)*engine_reg16(reg_lo_addr + 2u);
    uint32_t raw = (lo | (hi << 16)) & 0x07FFFFFFu;

    return (int32_t)(raw << 5) >> 5;
}

static int engine_affine_bg_map_tiles(int screen_size) {
    switch (screen_size & 3) {
    case 0:
        return 16;
    case 1:
        return 32;
    case 2:
        return 64;
    case 3:
    default:
        return 128;
    }
}

static int engine_wrap_affine_map_px(int coord, int dim_px) {
    int m = coord % dim_px;

    if (m < 0) {
        m += dim_px;
    }
    return m;
}

/* Mode 1/2 affine BG: byte tilemap; 8bpp or 4bpp tiles (same char layout as text BG). */
static int engine_sample_affine_rotoscale_pixel(uint16_t bgcnt, uint32_t screen_base, int map_tiles,
                                                int map_x_px, int map_y_px, int use_256_colors, int wrap,
                                                uint32_t *rgba_out) {
    uint8_t *vram = engine_vram();
    uint32_t char_base = (uint32_t)(((bgcnt >> 2) & 0x3u) * 0x4000u);
    int dim_px = map_tiles * 8;
    int wx;
    int wy;

    if (!wrap) {
        if (map_x_px < 0 || map_y_px < 0 || map_x_px >= dim_px || map_y_px >= dim_px) {
            return 0;
        }
        wx = map_x_px;
        wy = map_y_px;
    } else {
        wx = engine_wrap_affine_map_px(map_x_px, dim_px);
        wy = engine_wrap_affine_map_px(map_y_px, dim_px);
    }
    int tile_x = wx / 8;
    int tile_y = wy / 8;
    int pixel_x = wx & 7;
    int pixel_y = wy & 7;
    uint32_t map_off = screen_base + (uint32_t)(tile_y * map_tiles + tile_x);
    uint8_t tile_index;
    uint32_t tile_data_off;

    if (map_off > ENGINE_VRAM_SIZE - 1u) {
        return 0;
    }

    tile_index = vram[map_off];

    if (use_256_colors) {
        uint8_t pixel;

        tile_data_off = char_base + (uint32_t)tile_index * 64u + (uint32_t)(pixel_y * 8 + pixel_x);
        if (tile_data_off > ENGINE_VRAM_SIZE - 1u) {
            return 0;
        }

        pixel = vram[tile_data_off];
        if (pixel == 0) {
            return 0;
        }

        *rgba_out = g_palette_cache[pixel];
        return 1;
    }

    {
        uint8_t pair;
        uint8_t pixel;

        tile_data_off = char_base + (uint32_t)tile_index * 32u + (uint32_t)(pixel_y * 4 + (pixel_x / 2));
        if (tile_data_off > ENGINE_VRAM_SIZE - 1u) {
            return 0;
        }

        pair = vram[tile_data_off];
        pixel = (pixel_x & 1) ? (uint8_t)(pair >> 4) : (uint8_t)(pair & 0x0Fu);
        if (pixel == 0) {
            return 0;
        }

        /* Affine 16-color BGs index the first background palette bank (hardware has no per-map palette field). */
        *rgba_out = g_palette_cache[pixel];
        return 1;
    }
}

/*
 * GBA mode 1 BG2 or mode 2 BG2/BG3: affine (BGnX/BGnY + PA–PD), byte tilemap, optional mosaic.
 * Mode 2 is 8bpp-only on hardware; mode 1 may be 4bpp or 8bpp.
 */
static void engine_render_affine_rotoscale_bg(int bg, int priority, uint16_t dispcnt, uint16_t bgcnt,
                                                int force_mode2) {
    uint32_t reg_pa;
    uint32_t reg_x_l;
    uint32_t reg_y_l;
    uint32_t screen_base = (uint32_t)(((bgcnt >> 8) & 0x1Fu) * 0x800u);
    int use_256_colors = force_mode2 != 0 || (bgcnt & 0x0080u) != 0;
    int screen_size = (bgcnt >> 14) & 0x3;
    int map_tiles = engine_affine_bg_map_tiles(screen_size);
    int pa;
    int pb;
    int pc;
    int pd;
    int32_t lx0;
    int32_t ly0;
    int wrap = (bgcnt & BGCNT_WRAP) != 0;
    uint16_t mosaic_reg = *engine_reg16(ENGINE_REG_MOSAIC);
    int bg_mosaic = (bgcnt & BGCNT_MOSAIC) != 0;
    int mosaic_h_raw = (int)(mosaic_reg & 0xFu);
    int mosaic_v_raw = (int)((mosaic_reg >> 4) & 0xFu);
    int mosaic_h = mosaic_h_raw + 1;
    int mosaic_v = mosaic_v_raw + 1;
    int screen_y;
    int screen_x;
    uint8_t layer_kind;

    if (bg == 2) {
        reg_pa = ENGINE_REG_BG2PA;
        reg_x_l = ENGINE_REG_BG2X_L;
        reg_y_l = ENGINE_REG_BG2Y_L;
    } else if (bg == 3) {
        reg_pa = ENGINE_REG_BG3PA;
        reg_x_l = ENGINE_REG_BG3X_L;
        reg_y_l = ENGINE_REG_BG3Y_L;
    } else {
        return;
    }

    if (((dispcnt >> (8 + bg)) & 1u) == 0u) {
        return;
    }
    if ((bgcnt & 0x3u) != (uint16_t)priority) {
        return;
    }

    pa = (int)(int16_t)*engine_reg16(reg_pa);
    pb = (int)(int16_t)*engine_reg16(reg_pa + 2u);
    pc = (int)(int16_t)*engine_reg16(reg_pa + 4u);
    pd = (int)(int16_t)*engine_reg16(reg_pa + 6u);
    /*
     * Oak intro (and similar) uses mode-1 BG2 as a byte-mapped layer with only BGnX/BGnY updates
     * from ChangeBgX/ChangeBgY; it never calls SetBgAffine / BgAffineSet. After IO reset PA–PD are 0,
     * which freezes map stepping here and blanks the layer. Hardware identity uses 0x0100 in 8.8.
     */
    if (pa == 0 && pc == 0) {
        pa = 0x100;
    }
    if (pb == 0 && pd == 0) {
        pd = 0x100;
    }

    lx0 = engine_read_bg_affine_ref(reg_x_l);
    ly0 = engine_read_bg_affine_ref(reg_y_l);
    layer_kind = (uint8_t)(ENGINE_LAYER_BG0 + bg);

    for (screen_y = 0; screen_y < ENGINE_GBA_HEIGHT; ++screen_y) {
        int sy = screen_y;
        int32_t line_start_x;
        int32_t line_start_y;

        if (bg_mosaic && mosaic_v > 1) {
            sy = screen_y - (screen_y % mosaic_v);
        }
        line_start_x = lx0 + sy * pb;
        line_start_y = ly0 + sy * pd;

        for (screen_x = 0; screen_x < ENGINE_GBA_WIDTH; ++screen_x) {
            uint32_t rgba;
            int sx = screen_x;
            int32_t cur_x;
            int32_t cur_y;
            size_t index = (size_t)screen_y * ENGINE_GBA_WIDTH + (size_t)screen_x;
            uint8_t window_mask = g_window_mask_cache[index];

            if ((window_mask & (1u << bg)) == 0) {
                continue;
            }

            if (bg_mosaic && mosaic_h_raw != 0) {
                sx = screen_x - (screen_x % mosaic_h);
            }
            cur_x = line_start_x + pa * sx;
            cur_y = line_start_y + pc * sx;

            if (engine_sample_affine_rotoscale_pixel(bgcnt, screen_base, map_tiles, cur_x >> 8, cur_y >> 8,
                                                     use_256_colors, wrap, &rgba)) {
                engine_insert_layer_pixel(screen_x, screen_y, rgba, layer_kind, 0);
            }
        }
    }
}

static void engine_render_mode1_bg2_layer(int priority, uint16_t dispcnt, uint16_t bgcnt) {
    engine_render_affine_rotoscale_bg(2, priority, dispcnt, bgcnt, 0);
}

static EngineLayerPixel engine_get_pixel_layer(const EnginePixelLayers *pixel_layers, int reverse_index)
{
    EngineLayerPixel empty = {0};

    if (reverse_index < 0 || reverse_index >= pixel_layers->count)
        return empty;

    return pixel_layers->layers[pixel_layers->count - 1 - reverse_index];
}

static void engine_trace_trainer_card_compose_pixel(int x, int y,
                                                     EngineLayerPixel top,
                                                     EngineLayerPixel second,
                                                     uint32_t color,
                                                     uint8_t window_mask,
                                                     uint16_t bldcnt,
                                                     int effect,
                                                     int special_enabled,
                                                     int top_target1,
                                                     int second_target2) {
#ifdef PORTABLE
    static const char *const s_layer_names[] = {
        "BG0",
        "BG1",
        "BG2",
        "BG3",
        "OBJ",
        "BACKDROP",
    };
    static const struct {
        int x;
        int y;
    } points[] = {
        {80, 80},
        {120, 80},
        {180, 80},
        {200, 120},
    };
    unsigned long frame = engine_backend_get_completed_frame_external();
    uint16_t dispcnt = *engine_reg16(ENGINE_REG_DISPCNT);
    uint16_t bg0cnt = *engine_reg16(ENGINE_REG_BG0CNT);
    char buffer[224];

    if (dispcnt != 0x3F40u || bg0cnt != 0x9B02u)
        return;
    if (!engine_should_trace_trainer_card_samples(frame))
        return;
    if (!engine_is_trainer_card_sample_point(x, y))
        return;

    snprintf(buffer, sizeof(buffer),
             "TCOMP frame=%lu x=%d y=%d top=%s/%08lX second=%s/%08lX valid=%u final=%08lX mask=%02X bldcnt=%04X eff=%d sp=%d t1=%d t2=%d",
             frame,
             x,
             y,
             s_layer_names[top.kind <= ENGINE_LAYER_BACKDROP ? top.kind : ENGINE_LAYER_BACKDROP],
             (unsigned long)top.color,
             s_layer_names[second.kind <= ENGINE_LAYER_BACKDROP ? second.kind : ENGINE_LAYER_BACKDROP],
             (unsigned long)second.color,
             second.valid,
             (unsigned long)color,
             window_mask,
             bldcnt,
             effect,
             special_enabled,
             top_target1,
             second_target2);
    ENGINE_RENDERER_TRACE_MSG(buffer);
#else
    (void)x;
    (void)y;
    (void)top;
    (void)second;
    (void)color;
    (void)window_mask;
    (void)bldcnt;
    (void)effect;
    (void)special_enabled;
    (void)top_target1;
    (void)second_target2;
#endif
}

static void engine_compose_framebuffer(void) {
    uint16_t bldcnt = *engine_reg16(ENGINE_REG_BLDCNT);
    uint16_t bldalpha = *engine_reg16(ENGINE_REG_BLDALPHA);
    uint16_t bldy = *engine_reg16(ENGINE_REG_BLDY);
    int eva = bldalpha & 0x1F;
    int evb = (bldalpha >> 8) & 0x1F;
    int evy = bldy & 0x1F;
    int effect = (bldcnt >> 6) & 0x3;
    int x;
    int y;

    ENGINE_RENDERER_TRACE_MSG("compose: enter");

    if (eva > 16) eva = 16;
    if (evb > 16) evb = 16;
    if (evy > 16) evy = 16;

    if (effect == 0 && !g_has_semi_transparent_obj) {
        for (y = 0; y < ENGINE_GBA_HEIGHT; ++y) {
            for (x = 0; x < ENGINE_GBA_WIDTH; ++x) {
                size_t index = (size_t)y * ENGINE_GBA_WIDTH + (size_t)x;
                EngineLayerPixel top = engine_get_pixel_layer(&g_pixel_layers[index], 0);

                if (top.kind == ENGINE_LAYER_BACKDROP && (g_window_mask_cache[index] & 0x20u) == 0) {
                    engine_put_color(x, y, 0xFF000000u);
                } else {
                    engine_put_color(x, y, top.color);
                }
            }
        }
        ENGINE_RENDERER_TRACE_MSG("compose: exit");
        return;
    }

    for (y = 0; y < ENGINE_GBA_HEIGHT; ++y) {
        for (x = 0; x < ENGINE_GBA_WIDTH; ++x) {
            size_t index = (size_t)y * ENGINE_GBA_WIDTH + (size_t)x;
            EngineLayerPixel top = engine_get_pixel_layer(&g_pixel_layers[index], 0);
            EngineLayerPixel second = engine_get_pixel_layer(&g_pixel_layers[index], 1);
            uint32_t color = top.color;
            uint8_t window_mask = g_window_mask_cache[index];
            int special_enabled = (window_mask & 0x20u) != 0;
            uint8_t top_bit = engine_target_bit_for_kind(top.kind);
            uint8_t second_bit = engine_target_bit_for_kind(second.kind);
            int top_target1 = (bldcnt & top_bit) != 0;
            int second_target2 = ((bldcnt >> 8) & second_bit) != 0;

            if (top.kind == ENGINE_LAYER_BACKDROP && (window_mask & 0x20u) == 0) {
                color = 0xFF000000u;
                engine_put_color(x, y, color);
                continue;
            }

            if (special_enabled) {
                if (top.semi_transparent && top.kind == ENGINE_LAYER_OBJ) {
                    if (second.valid && second_target2) {
                        color = engine_blend_alpha(top.color, second.color, eva, evb);
                    }
                } else if (effect == 1 && top_target1 && second.valid && second_target2) {
                    color = engine_blend_alpha(top.color, second.color, eva, evb);
                } else if (effect == 2 && top_target1) {
                    color = engine_blend_brighten(top.color, evy);
                } else if (effect == 3 && top_target1) {
                    color = engine_blend_darken(top.color, evy);
                }
            }

            engine_trace_trainer_card_compose_pixel(x, y, top, second, color, window_mask, bldcnt,
                                                     effect, special_enabled, top_target1, second_target2);

            engine_put_color(x, y, color);
        }
    }

    ENGINE_RENDERER_TRACE_MSG("compose: exit");
}

/* GBA text BG: BGCNT char base (2 bits) * 16KiB + linear 4bpp tile_index * 32. Indices may span two 16KiB
 * slabs from one base (FireRed field map tileset policy documents this in `fieldmap.h` MAP_BG_*). */
static void engine_render_4bpp_tile(int bg, int screen_x, int screen_y, uint16_t tile_index, uint16_t palette_bank, int hflip, int vflip) {
    uint8_t *vram = engine_vram();
    uint16_t bgcnt = engine_bgcnt_value(bg);
    uint32_t char_base = (uint32_t)(((bgcnt >> 2) & 0x3u) * 0x4000u);
    uint32_t tile_offset = char_base + (uint32_t)tile_index * 32u;
    const uint8_t *tile_data;
    int x;
    int y;

    if (tile_offset > ENGINE_VRAM_SIZE - 32u) {
        return;
    }

    tile_data = vram + tile_offset;
    for (y = 0; y < 8; ++y) {
        int src_y = vflip ? (7 - y) : y;
        for (x = 0; x < 8; ++x) {
            int src_x = hflip ? (7 - x) : x;
            uint8_t pair = tile_data[src_y * 4 + (src_x / 2)];
            uint8_t pixel = (src_x & 1) ? (uint8_t)(pair >> 4) : (uint8_t)(pair & 0x0Fu);
            if (pixel != 0) {
                engine_put_color(screen_x + x, screen_y + y, g_palette_cache[palette_bank * 16u + pixel]);
            }
        }
    }
}

static void engine_render_8bpp_tile(int bg, int screen_x, int screen_y, uint16_t tile_index, int hflip, int vflip) {
    uint8_t *vram = engine_vram();
    uint16_t bgcnt = engine_bgcnt_value(bg);
    uint32_t char_base = (uint32_t)(((bgcnt >> 2) & 0x3u) * 0x4000u);
    uint32_t tile_offset = char_base + (uint32_t)tile_index * 64u;
    const uint8_t *tile_data;
    int x;
    int y;

    if (tile_offset > ENGINE_VRAM_SIZE - 64u) {
        return;
    }

    tile_data = vram + tile_offset;
    for (y = 0; y < 8; ++y) {
        int src_y = vflip ? (7 - y) : y;
        for (x = 0; x < 8; ++x) {
            int src_x = hflip ? (7 - x) : x;
            uint8_t pixel = tile_data[src_y * 8 + src_x];
            if (pixel != 0) {
                engine_put_color(screen_x + x, screen_y + y, g_palette_cache[pixel]);
            }
        }
    }
}

static uint32_t engine_text_bg_tilemap_offset(uint32_t screen_base, int screen_size, int screen_tile_x, int screen_tile_y) {
    int block_x = screen_tile_x / 32;
    int block_y = screen_tile_y / 32;
    int local_x = screen_tile_x % 32;
    int local_y = screen_tile_y % 32;
    uint32_t screenblock_index = 0;

    switch (screen_size) {
    case 0:
        screenblock_index = 0;
        break;
    case 1:
        screenblock_index = (uint32_t)block_x;
        break;
    case 2:
        screenblock_index = (uint32_t)block_y;
        break;
    case 3:
        screenblock_index = (uint32_t)(block_y * 2 + block_x);
        break;
    }

    return screen_base + screenblock_index * 0x800u + (uint32_t)((local_y * 32 + local_x) * 2);
}

static const uint16_t *engine_active_scanline_buffer(void) {
    if (gScanlineEffect.state == 0) {
        return NULL;
    }

    return gScanlineEffectRegBuffers[gScanlineEffect.srcBuffer ^ 1];
}

static int engine_bg_offset_for_line(int bg, int line, int vertical, int fallback) {
    const uint16_t *buffer = engine_active_scanline_buffer();
    uintptr_t dest = (uintptr_t)gScanlineEffect.dmaDest;
    uintptr_t hofs_reg = ENGINE_REG_BG0HOFS + (uintptr_t)(bg * 4);
    uintptr_t vofs_reg = ENGINE_REG_BG0VOFS + (uintptr_t)(bg * 4);

    if (buffer == NULL || line < 0 || line >= ENGINE_GBA_HEIGHT) {
        return fallback;
    }

    if (gScanlineEffect.dmaControl == SCANLINE_EFFECT_DMACNT_16BIT) {
        if ((vertical && dest == vofs_reg) || (!vertical && dest == hofs_reg)) {
            return (int)(int16_t)buffer[line];
        }
    } else if (gScanlineEffect.dmaControl == SCANLINE_EFFECT_DMACNT_32BIT && dest == hofs_reg) {
        if (vertical) {
            return (int)(int16_t)buffer[line * 2 + 1];
        }
        return (int)(int16_t)buffer[line * 2];
    }

    return fallback;
}

static void engine_fill_bg_offset_cache(int bg, int fallback_hofs, int fallback_vofs,
                                        int *line_hofs_cache, int *line_vofs_cache) {
    const uint16_t *buffer = engine_active_scanline_buffer();
    uintptr_t dest = (uintptr_t)gScanlineEffect.dmaDest;
    uintptr_t hofs_reg = ENGINE_REG_BG0HOFS + (uintptr_t)(bg * 4);
    uintptr_t vofs_reg = ENGINE_REG_BG0VOFS + (uintptr_t)(bg * 4);
    int line;

    if (buffer == NULL) {
        for (line = 0; line < ENGINE_GBA_HEIGHT; ++line) {
            line_hofs_cache[line] = fallback_hofs;
            line_vofs_cache[line] = fallback_vofs;
        }
        return;
    }

    if (gScanlineEffect.dmaControl == SCANLINE_EFFECT_DMACNT_16BIT) {
        if (dest == hofs_reg) {
            for (line = 0; line < ENGINE_GBA_HEIGHT; ++line) {
                line_hofs_cache[line] = (int)(int16_t)buffer[line];
                line_vofs_cache[line] = fallback_vofs;
            }
            return;
        }
        if (dest == vofs_reg) {
            for (line = 0; line < ENGINE_GBA_HEIGHT; ++line) {
                line_hofs_cache[line] = fallback_hofs;
                line_vofs_cache[line] = (int)(int16_t)buffer[line];
            }
            return;
        }
    } else if (gScanlineEffect.dmaControl == SCANLINE_EFFECT_DMACNT_32BIT && dest == hofs_reg) {
        for (line = 0; line < ENGINE_GBA_HEIGHT; ++line) {
            line_hofs_cache[line] = (int)(int16_t)buffer[line * 2];
            line_vofs_cache[line] = (int)(int16_t)buffer[line * 2 + 1];
        }
        return;
    }

    for (line = 0; line < ENGINE_GBA_HEIGHT; ++line) {
        line_hofs_cache[line] = fallback_hofs;
        line_vofs_cache[line] = fallback_vofs;
    }
}

static int engine_sample_bg_pixel(int bg, uint16_t bgcnt, uint32_t screen_base, int screen_size, int use_256_colors, int src_x, int src_y, uint32_t *rgba_out) {
    uint8_t *vram = engine_vram();
    uint32_t char_base = (uint32_t)(((bgcnt >> 2) & 0x3u) * 0x4000u);
    int screen_tile_x = src_x >> 3;
    int screen_tile_y = src_y >> 3;
    uint32_t tilemap_offset = engine_text_bg_tilemap_offset(screen_base, screen_size, screen_tile_x, screen_tile_y);
    uint16_t tile_entry;
    uint16_t tile_index;
    int hflip;
    int vflip;
    int pixel_x;
    int pixel_y;
    uint32_t tile_offset;

    if (tilemap_offset > ENGINE_VRAM_SIZE - 2u) {
        return 0;
    }

    tile_entry = *(uint16_t *)(void *)(vram + tilemap_offset);
    tile_index = (uint16_t)(tile_entry & 0x03FFu);
    hflip = (tile_entry & 0x0400u) != 0;
    vflip = (tile_entry & 0x0800u) != 0;
    pixel_x = src_x & 7;
    pixel_y = src_y & 7;

    if (hflip) {
        pixel_x = 7 - pixel_x;
    }
    if (vflip) {
        pixel_y = 7 - pixel_y;
    }

    if (use_256_colors) {
        uint8_t pixel;

        tile_offset = char_base + (uint32_t)tile_index * 64u + (uint32_t)(pixel_y * 8 + pixel_x);
        if (tile_offset > ENGINE_VRAM_SIZE - 1u) {
            return 0;
        }

        pixel = vram[tile_offset];
        if (pixel == 0) {
            return 0;
        }

        *rgba_out = g_palette_cache[pixel];
#ifdef PORTABLE
        {
            static unsigned long s_logged_frame = (unsigned long)-1;
            static uint8_t s_logged_mask = 0;
            unsigned long frame = engine_backend_get_completed_frame_external();
            uint16_t dispcnt = *engine_reg16(ENGINE_REG_DISPCNT);
            uint16_t bg0cnt_live = *engine_reg16(ENGINE_REG_BG0CNT);
            int screen_x = src_x & 0xFF;
            int screen_y = src_y & 0xFF;
            char buffer[224];

            if (dispcnt == 0x3F40u && bg0cnt_live == 0x9B02u && (frame % 30u) == 0u) {
                if (frame != s_logged_frame) {
                    s_logged_frame = frame;
                    s_logged_mask = 0;
                }
                if (bg < 4 && !(s_logged_mask & (1u << bg)) && screen_x == 80 && screen_y == 80) {
                    s_logged_mask |= (uint8_t)(1u << bg);
                    snprintf(buffer, sizeof(buffer),
                             "TBGPIX bg=%d 8bpp entry=%04X idx=%u pixel=%u rgba=%08lX tileofs=%04lX",
                             bg, tile_entry, (unsigned)tile_index, (unsigned)pixel,
                             (unsigned long)*rgba_out, (unsigned long)(char_base + (uint32_t)tile_index * 64u));
                    ENGINE_RENDERER_TRACE_MSG(buffer);
                }
            }
        }
#endif
        return 1;
    } else {
        uint16_t palette_bank = (uint16_t)((tile_entry >> 12) & 0x0Fu);
        uint8_t pair;
        uint8_t pixel;

        tile_offset = char_base + (uint32_t)tile_index * 32u + (uint32_t)(pixel_y * 4 + (pixel_x / 2));
        if (tile_offset > ENGINE_VRAM_SIZE - 1u) {
            return 0;
        }

        pair = vram[tile_offset];
        pixel = (pixel_x & 1) ? (uint8_t)(pair >> 4) : (uint8_t)(pair & 0x0Fu);
        if (pixel == 0) {
            return 0;
        }

        *rgba_out = g_palette_cache[palette_bank * 16u + pixel];
#ifdef PORTABLE
        {
            static unsigned long s_logged_frame = (unsigned long)-1;
            static uint8_t s_logged_mask = 0;
            unsigned long frame = engine_backend_get_completed_frame_external();
            uint16_t dispcnt = *engine_reg16(ENGINE_REG_DISPCNT);
            uint16_t bg0cnt_live = *engine_reg16(ENGINE_REG_BG0CNT);
            int screen_x = src_x & 0xFF;
            int screen_y = src_y & 0xFF;
            char buffer[256];

            if (dispcnt == 0x3F40u && bg0cnt_live == 0x9B02u && (frame % 30u) == 0u) {
                if (frame != s_logged_frame) {
                    s_logged_frame = frame;
                    s_logged_mask = 0;
                }
                if (bg < 4 && !(s_logged_mask & (1u << bg)) && screen_x == 80 && screen_y == 80) {
                    s_logged_mask |= (uint8_t)(1u << bg);
                    snprintf(buffer, sizeof(buffer),
                             "TBGPIX bg=%d 4bpp entry=%04X idx=%u pal=%u pair=%02X pixel=%u rgba=%08lX tileofs=%04lX",
                             bg, tile_entry, (unsigned)tile_index, (unsigned)palette_bank,
                             pair, (unsigned)pixel, (unsigned long)*rgba_out,
                             (unsigned long)(char_base + (uint32_t)tile_index * 32u));
                    ENGINE_RENDERER_TRACE_MSG(buffer);
                }
            }
        }
#endif
        return 1;
    }
}

static void engine_render_mode0_backgrounds_for_priority(int priority) {
    uint16_t dispcnt = *engine_reg16(ENGINE_REG_DISPCNT);
    unsigned int video_mode = dispcnt & 7u;
    uint16_t mosaic_reg = *engine_reg16(ENGINE_REG_MOSAIC);
    int mosaic_h_raw_global = (int)(mosaic_reg & 0xFu);
    int mosaic_v_global = (int)((mosaic_reg >> 4) & 0xFu) + 1;
    int mosaic_h_global = mosaic_h_raw_global + 1;
    int bg;

    ENGINE_RENDERER_TRACEF("render_bg: priority=%d enter", priority);

    for (bg = 3; bg >= 0; --bg) {
        uint16_t bgcnt = engine_bgcnt_value(bg);
        uint32_t screen_base = (uint32_t)(((bgcnt >> 8) & 0x1Fu) * 0x800u);
        int use_256_colors = (bgcnt & 0x0080u) != 0;
        int screen_size = (bgcnt >> 14) & 0x3;
        int screen_width_tiles = 32;
        int screen_height_tiles = 32;
        int hofs;
        int vofs;
        int screen_x;
        int screen_y;
        int line_hofs_cache[ENGINE_GBA_HEIGHT];
        int line_vofs_cache[ENGINE_GBA_HEIGHT];
        int bg_mosaic = (bgcnt & BGCNT_MOSAIC) != 0;
        int mosaic_h_active = bg_mosaic && mosaic_h_raw_global != 0;

        if (((dispcnt >> (8 + bg)) & 1u) == 0u) {
            continue;
        }
        if ((bgcnt & 0x3u) != (uint16_t)priority) {
            continue;
        }

        if (video_mode == 2u) {
            if (bg < 2) {
                continue;
            }
            engine_render_affine_rotoscale_bg(bg, priority, dispcnt, bgcnt, 1);
            continue;
        }

        if (video_mode == 1u && bg == 2) {
            engine_render_mode1_bg2_layer(priority, dispcnt, bgcnt);
            continue;
        }

        if (screen_size == 1) {
            screen_width_tiles = 64;
        } else if (screen_size == 2) {
            screen_height_tiles = 64;
        } else if (screen_size == 3) {
            screen_width_tiles = 64;
            screen_height_tiles = 64;
        }

        hofs = engine_bg_hofs(bg);
        vofs = engine_bg_vofs(bg);
        engine_fill_bg_offset_cache(bg, hofs, vofs, line_hofs_cache, line_vofs_cache);

        for (screen_y = 0; screen_y < ENGINE_GBA_HEIGHT; ++screen_y) {
            int line_hofs = line_hofs_cache[screen_y];
            int line_vofs = line_vofs_cache[screen_y];
            int eff_y = screen_y;
            int src_y;

            if (bg_mosaic && mosaic_v_global > 1) {
                eff_y = screen_y - (screen_y % mosaic_v_global);
            }
            src_y = (eff_y + line_vofs) & (screen_height_tiles * 8 - 1);

            for (screen_x = 0; screen_x < ENGINE_GBA_WIDTH; ++screen_x) {
                int eff_x = screen_x;
                int src_x;
                uint32_t rgba;
                size_t index = (size_t)screen_y * ENGINE_GBA_WIDTH + (size_t)screen_x;
                uint8_t window_mask = g_window_mask_cache[index];

                if ((window_mask & (1u << bg)) == 0) {
                    continue;
                }

                if (mosaic_h_active) {
                    eff_x = screen_x - (screen_x % mosaic_h_global);
                }
                src_x = (eff_x + line_hofs) & (screen_width_tiles * 8 - 1);

                if (engine_sample_bg_pixel(bg, bgcnt, screen_base, screen_size, use_256_colors, src_x, src_y, &rgba)) {
                    engine_insert_layer_pixel(screen_x, screen_y, rgba, (uint8_t)(ENGINE_LAYER_BG0 + bg), 0);
                }
            }
        }
    }

    ENGINE_RENDERER_TRACEF("render_bg: priority=%d exit", priority);
}

static void engine_precompute_window_masks(void) {
    uint16_t dispcnt = *engine_reg16(ENGINE_REG_DISPCNT);
    uint16_t win0h = *engine_reg16(ENGINE_REG_WIN0H);
    uint16_t win1h = *engine_reg16(ENGINE_REG_WIN1H);
    uint16_t win0v = *engine_reg16(ENGINE_REG_WIN0V);
    uint16_t win1v = *engine_reg16(ENGINE_REG_WIN1V);
    uint16_t winin = *engine_reg16(ENGINE_REG_WININ);
    uint16_t winout = *engine_reg16(ENGINE_REG_WINOUT);
    int x;
    int y;

    ENGINE_RENDERER_TRACEF("precompute_windows: DISPCNT=%04X", dispcnt);

    engine_precompute_obj_window_mask(dispcnt);

    if ((dispcnt & (ENGINE_DISPCNT_WIN0_ON | ENGINE_DISPCNT_WIN1_ON | ENGINE_DISPCNT_OBJWIN_ON)) == 0) {
        memset(g_window_mask_cache, 0x3F, sizeof(g_window_mask_cache));
        ENGINE_RENDERER_TRACE_MSG("precompute_windows: done");
        return;
    }

    for (y = 0; y < ENGINE_GBA_HEIGHT; ++y) {
        for (x = 0; x < ENGINE_GBA_WIDTH; ++x) {
            size_t index = (size_t)y * ENGINE_GBA_WIDTH + (size_t)x;
            g_window_mask_cache[index] = engine_compute_window_mask_for_pixel(x, y, dispcnt, win0h, win1h, win0v, win1v, winin, winout);
        }
    }

    ENGINE_RENDERER_TRACE_MSG("precompute_windows: done");
}

static void engine_render_sprite(const EngineOAMEntry *oam,
                                 uint16_t attr0,
                                 uint16_t attr1,
                                 uint16_t attr2,
                                 int obj_1d,
                                 int x_pos,
                                 int y_pos,
                                 int width,
                                 int height,
                                 uint8_t semi_transparent) {
    int draw_x;
    int draw_y;
    int draw_width;
    int draw_height;
    int start_x;
    int start_y;
    int end_x;
    int end_y;
    int use_256_colors = (attr0 & 0x2000u) != 0;
    uint16_t tile_index = (uint16_t)(attr2 & 0x03FFu);
    uint16_t palette_bank = (uint16_t)((attr2 >> 12) & 0x0Fu);
    int screen_x;
    int screen_y;

    engine_sprite_draw_bounds(attr0, x_pos, y_pos, width, height, &draw_x, &draw_y, &draw_width, &draw_height);
    start_x = (draw_x < 0) ? 0 : draw_x;
    start_y = (draw_y < 0) ? 0 : draw_y;
    end_x = draw_x + draw_width;
    end_y = draw_y + draw_height;
    if (end_x > ENGINE_GBA_WIDTH) {
        end_x = ENGINE_GBA_WIDTH;
    }
    if (end_y > ENGINE_GBA_HEIGHT) {
        end_y = ENGINE_GBA_HEIGHT;
    }

    if (start_x >= end_x || start_y >= end_y) {
        return;
    }

    for (screen_y = start_y; screen_y < end_y; ++screen_y) {
        for (screen_x = start_x; screen_x < end_x; ++screen_x) {
            int src_x;
            int src_y;
            uint32_t rgba;
            size_t index;
            uint8_t window_mask;

            if (!engine_sprite_source_coords_for_pixel(oam, attr0, attr1, x_pos, y_pos, width, height, screen_x, screen_y, &src_x, &src_y)) {
                continue;
            }
            if (!engine_sample_sprite_pixel(obj_1d, use_256_colors, tile_index, palette_bank, width, height, src_x, src_y, &rgba)) {
                continue;
            }

            index = (size_t)screen_y * ENGINE_GBA_WIDTH + (size_t)screen_x;
            window_mask = g_window_mask_cache[index];
            if (window_mask & 0x10u) {
                engine_insert_layer_pixel(screen_x, screen_y, rgba, ENGINE_LAYER_OBJ, semi_transparent);
            }
        }
    }
}

static void engine_render_sprites_for_priority(int priority) {
    EngineOAMEntry *oam = engine_oam();
    uint16_t dispcnt = *engine_reg16(ENGINE_REG_DISPCNT);
    uint16_t bg0cnt = *engine_reg16(ENGINE_REG_BG0CNT);
    uint16_t bg1cnt = *engine_reg16(ENGINE_REG_BG1CNT);
    uint16_t bg2cnt = *engine_reg16(ENGINE_REG_BG2CNT);
    uint16_t bg3cnt = *engine_reg16(ENGINE_REG_BG3CNT);
    int obj_1d = (dispcnt & ENGINE_DISPCNT_OBJ_1D_MAP) != 0;
    int i;

    ENGINE_RENDERER_TRACEF("render_sprites: priority=%d enter", priority);

    for (i = ENGINE_MAX_SPRITES - 1; i >= 0; --i) {
        EngineOAMEntry sprite = oam[i];
        uint16_t attr0 = sprite.attr0;
        uint16_t attr1 = sprite.attr1;
        uint16_t attr2 = sprite.attr2;
        int sprite_priority;
        int x_pos;
        int y_pos;
        int shape;
        int size;
        int width;
        int height;
        int use_256_colors;
        uint8_t semi_transparent;

        if (engine_sprite_is_disabled(attr0)) {
            continue;
        }
        if ((attr0 & 0x0C00u) == 0x0800u) {
            continue;
        }
        /*
         * Uninitialized / cleared OAM decodes as a normal 8x8 OBJ at (0,0) with tile 0.
         * That samples whatever happens to live in OBJ tile 0 and produces small stray shards
         * that change with screen content. Retail builds hide unused slots with dummy OAM, not
         * all-zero attrs.
         */
        if (attr0 == 0u && attr1 == 0u && attr2 == 0u) {
            continue;
        }

        sprite_priority = (attr2 >> 10) & 0x3;
        if (sprite_priority != priority) {
            continue;
        }

        y_pos = attr0 & 0xFF;
        x_pos = (int)(attr1 & 0x1FFu);
        if (y_pos >= 160) {
            y_pos -= 256;
        }
        if (x_pos >= 256) {
            x_pos -= 512;
        }

        shape = (attr0 >> 14) & 0x3;
        size = (attr1 >> 14) & 0x3;
        use_256_colors = (attr0 & 0x2000u) != 0;
        width = 0;
        height = 0;

        engine_sprite_dimensions(shape, size, &width, &height);
        if (width == 0 || height == 0) {
            continue;
        }

        semi_transparent = ((attr0 & 0x0C00u) == 0x0400u);
        if (semi_transparent) {
            g_has_semi_transparent_obj = 1;
        }

        engine_render_sprite(oam, attr0, attr1, attr2, obj_1d, x_pos, y_pos, width, height, semi_transparent);
    }

    ENGINE_RENDERER_TRACEF("render_sprites: priority=%d exit", priority);
}

static void engine_dump_sprite_pipeline_once(void) {
#ifdef PORTABLE
    static int dumped;
    unsigned long frame = engine_backend_get_completed_frame_external();
    EngineOAMEntry *oam;
    uint8_t *vram;
    uint16_t *palette;
    int i;
    int nonzero_count = 0;
    int printed = 0;
    char buffer[320];

    if (dumped || frame < 7400 || frame > 7600) {
        return;
    }
    dumped = 1;

    oam = engine_oam();
    vram = engine_vram();
    palette = engine_palette();

    for (i = 0; i < ENGINE_MAX_SPRITES; ++i) {
        if (oam[i].attr0 != 0 || oam[i].attr1 != 0 || oam[i].attr2 != 0) {
            nonzero_count += 1;
        }
    }

    snprintf(buffer, sizeof(buffer),
             "SPRITECHK frame=%lu obj_on=%u oam_nonzero=%d obj_vram=%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X obj_pltt=%04X %04X %04X %04X %04X %04X %04X %04X",
             frame,
             ((*engine_reg16(ENGINE_REG_DISPCNT) & ENGINE_DISPCNT_OBJ_ON) != 0),
             nonzero_count,
             vram[0x10000], vram[0x10001], vram[0x10002], vram[0x10003],
             vram[0x10004], vram[0x10005], vram[0x10006], vram[0x10007],
             vram[0x10008], vram[0x10009], vram[0x1000A], vram[0x1000B],
             vram[0x1000C], vram[0x1000D], vram[0x1000E], vram[0x1000F],
             palette[256], palette[257], palette[258], palette[259],
             palette[260], palette[261], palette[262], palette[263]);
    ENGINE_RENDERER_TRACE_MSG(buffer);

    for (i = 0; i < ENGINE_MAX_SPRITES && printed < 4; ++i) {
        EngineOAMEntry sprite = oam[i];
        uint16_t attr0 = sprite.attr0;
        uint16_t attr1 = sprite.attr1;
        uint16_t attr2 = sprite.attr2;

        if (attr0 == 0 && attr1 == 0 && attr2 == 0) {
            continue;
        }

        snprintf(buffer, sizeof(buffer),
                 "SPRITECHK OAM[%d] attr0=%04X attr1=%04X attr2=%04X x=%d y=%d tile=%u pal=%u size=%u shape=%u prio=%u",
                 i,
                 attr0,
                 attr1,
                 attr2,
                 (attr1 & 0x1FFu) >= 256u ? (int)((attr1 & 0x1FFu) - 512) : (int)(attr1 & 0x1FFu),
                 (attr0 & 0xFFu) >= 160 ? (int)((attr0 & 0xFFu) - 256) : (int)(attr0 & 0xFFu),
                 (unsigned)(attr2 & 0x03FFu),
                 (unsigned)((attr2 >> 12) & 0x0Fu),
                 (unsigned)((attr1 >> 14) & 0x3u),
                 (unsigned)((attr0 >> 14) & 0x3u),
                 (unsigned)((attr2 >> 10) & 0x3u));
        ENGINE_RENDERER_TRACE_MSG(buffer);
        printed += 1;
    }
#endif
}

static void engine_dump_trainer_card_render_once(uint16_t dispcnt) {
#ifdef PORTABLE
    static int dumped;
    uint16_t bg0cnt;
    uint16_t *palette;
    uint8_t *vram;
    EngineOAMEntry *oam;
    int i;
    char buffer[320];

    if (dumped)
        return;

    bg0cnt = *engine_reg16(ENGINE_REG_BG0CNT);
    if (dispcnt != 0x3F40u || bg0cnt != 0x9B02u)
        return;

    dumped = 1;
    palette = engine_palette();
    vram = engine_vram();

    for (i = 0; i < 4; ++i) {
        uint16_t bgcnt = engine_bgcnt_value(i);
        uint32_t screen_base = (uint32_t)(((bgcnt >> 8) & 0x1Fu) * 0x800u);
        uint32_t char_base = (uint32_t)(((bgcnt >> 2) & 0x3u) * 0x4000u);
        uint32_t entry_index;
        uint16_t tile_entry = 0;
        uint16_t tile_index = 0;
        uint16_t palette_bank = 0;
        uint32_t tile_offset = 0;

        if (((dispcnt >> (8 + i)) & 1u) == 0u)
            continue;

        for (entry_index = 0; entry_index < 32u * 32u; ++entry_index) {
            uint32_t tilemap_offset = screen_base + entry_index * 2u;

            tile_entry = *(uint16_t *)(void *)(vram + tilemap_offset);
            if (tile_entry != 0)
                break;
        }

        tile_index = (uint16_t)(tile_entry & 0x03FFu);
        palette_bank = (uint16_t)((tile_entry >> 12) & 0x0Fu);
        tile_offset = char_base + (uint32_t)tile_index * 32u;

        snprintf(buffer, sizeof(buffer),
                 "TCRENDER BG%d entry=%04X idx=%u pal=%u charbase=%04lX screenbase=%04lX first=%lu tile=%02X%02X%02X%02X bgpal=%04X,%04X,%04X,%04X objpal=%04X,%04X,%04X,%04X faded=%04X,%04X,%04X,%04X",
                 i,
                 tile_entry,
                 (unsigned)tile_index,
                 (unsigned)palette_bank,
                 (unsigned long)char_base,
                 (unsigned long)screen_base,
                 (unsigned long)entry_index,
                 vram[tile_offset + 0], vram[tile_offset + 1], vram[tile_offset + 2], vram[tile_offset + 3],
                 palette[palette_bank * 16u + 0], palette[palette_bank * 16u + 1], palette[palette_bank * 16u + 2], palette[palette_bank * 16u + 3],
                 palette[256u + palette_bank * 16u + 0], palette[256u + palette_bank * 16u + 1], palette[256u + palette_bank * 16u + 2], palette[256u + palette_bank * 16u + 3],
                 gPlttBufferFaded[palette_bank * 16u + 0], gPlttBufferFaded[palette_bank * 16u + 1], gPlttBufferFaded[palette_bank * 16u + 2], gPlttBufferFaded[palette_bank * 16u + 3]);
        ENGINE_RENDERER_TRACE_MSG(buffer);
    }

    oam = engine_oam();
    for (i = 0; i < ENGINE_MAX_SPRITES; ++i) {
        uint16_t attr0 = oam[i].attr0;
        uint16_t attr1 = oam[i].attr1;
        uint16_t attr2 = oam[i].attr2;
        uint16_t obj_tile;
        uint16_t obj_pal;
        uint32_t obj_tile_offset;

        if (attr0 == 0 && attr1 == 0 && attr2 == 0)
            continue;

        obj_tile = (uint16_t)(attr2 & 0x03FFu);
        obj_pal = (uint16_t)((attr2 >> 12) & 0x0Fu);
        obj_tile_offset = 0x10000u + (uint32_t)obj_tile * 32u;
        snprintf(buffer, sizeof(buffer),
                 "TCRENDER OBJ attr0=%04X attr1=%04X attr2=%04X tile=%u pal=%u objtile=%02X%02X%02X%02X objpal=%04X,%04X,%04X,%04X",
                 attr0, attr1, attr2,
                 (unsigned)obj_tile,
                 (unsigned)obj_pal,
                 vram[obj_tile_offset + 0], vram[obj_tile_offset + 1], vram[obj_tile_offset + 2], vram[obj_tile_offset + 3],
                 palette[256u + obj_pal * 16u + 0], palette[256u + obj_pal * 16u + 1], palette[256u + obj_pal * 16u + 2], palette[256u + obj_pal * 16u + 3]);
        ENGINE_RENDERER_TRACE_MSG(buffer);
        break;
    }
#else
    (void)dispcnt;
#endif
}

void engine_video_reset(void) {
    memset(g_framebuffer, 0, sizeof(g_framebuffer));
    memset(g_palette_cache, 0, sizeof(g_palette_cache));
    memset(g_palette_snapshot, 0, sizeof(g_palette_snapshot));
    memset(g_pixel_layers, 0, sizeof(g_pixel_layers));
    memset(g_obj_window_mask_cache, 0, sizeof(g_obj_window_mask_cache));
    memset(g_window_mask_cache, 0, sizeof(g_window_mask_cache));
    g_has_semi_transparent_obj = 0;
}

void engine_video_render_frame(void) {
    uint16_t dispcnt = *engine_reg16(ENGINE_REG_DISPCNT);
    uint32_t backdrop;
    int mode;
    int priority;
    static unsigned long render_count;

#ifdef PORTABLE
    {
        char buffer[256];
        uint16_t bg0cnt = *engine_reg16(ENGINE_REG_BG0CNT);
        uint16_t *palette = engine_palette();
        snprintf(buffer, sizeof(buffer),
                 "engine_video_render_frame: DISPCNT=%04X BG0CNT=%04X PLTT0=%04X PLTT1=%04X VRAM0=%02X VRAM1=%02X",
                 dispcnt, bg0cnt, palette[0], palette[1], engine_vram()[0], engine_vram()[1]);
        ENGINE_RENDERER_TRACE_MSG(buffer);
    }
#endif

    engine_update_palette_cache();
    engine_dump_trainer_card_render_once(dispcnt);

    if (dispcnt & ENGINE_DISPCNT_FORCED_BLANK) {
        engine_clear_framebuffer(0xFFFFFFFFu);
        return;
    }

    backdrop = g_palette_cache[0];
    engine_clear_framebuffer(backdrop);
    memset(g_pixel_layers, 0, sizeof(g_pixel_layers));
    g_has_semi_transparent_obj = 0;
    engine_precompute_window_masks();
    {
        size_t i;
        EngineLayerPixel fill;
        EnginePixelLayers *pixel_layers;

        ENGINE_RENDERER_TRACE_MSG("backdrop_init: enter");

        fill.color = backdrop;
        fill.kind = ENGINE_LAYER_BACKDROP;
        fill.valid = 1;
        fill.semi_transparent = 0;

        for (i = 0; i < ENGINE_GBA_WIDTH * ENGINE_GBA_HEIGHT; ++i) {
            pixel_layers = &g_pixel_layers[i];
            pixel_layers->layers[0] = fill;
            pixel_layers->count = 1;
        }

        ENGINE_RENDERER_TRACE_MSG("backdrop_init: exit");
    }

    mode = dispcnt & 0x7;
    for (priority = 3; priority >= 0; --priority) {
        if (mode == 0 || mode == 1 || mode == 2) {
            engine_render_mode0_backgrounds_for_priority(priority);
        }

        if (dispcnt & ENGINE_DISPCNT_OBJ_ON) {
            engine_render_sprites_for_priority(priority);
        }
    }

    engine_compose_framebuffer();

    engine_dump_sprite_pipeline_once();

#ifdef PORTABLE
    render_count += 1;
    if (render_count == 16) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
                 "FRAME16 FB %02X %02X %02X %02X %02X %02X %02X %02X backdrop=%08lX",
                 g_framebuffer[0], g_framebuffer[1], g_framebuffer[2], g_framebuffer[3],
                 g_framebuffer[4], g_framebuffer[5], g_framebuffer[6], g_framebuffer[7],
                 (unsigned long)backdrop);
        ENGINE_RENDERER_TRACE_MSG(buffer);
    }
#endif
}

const uint8_t *engine_video_get_framebuffer(void) {
    return g_framebuffer;
}

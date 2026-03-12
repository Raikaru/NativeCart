/**
 * @file gba_memory.cpp
 * @brief GBA Memory System - Implementation
 * 
 * This file implements the GBA memory overlay system with host-addressable
 * backing storage for all GBA memory regions.
 * 
 * Key features:
 * - Direct memory mapping for fast access
 * - Little-endian byte order (matching GBA)
 * - Address mirroring for appropriate regions
 * - ROM is read-only from GBA perspective
 * 
 * @author GBA Memory System Engineer
 * @version 1.0.0
 */

// 1. NOMINMAX guard
#define NOMINMAX

// 2. Standard C++ headers
#include "gba_memory.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// GLOBAL MEMORY CONTEXT
// =============================================================================

static GBAMemoryContext g_memory_context;
static int g_initialized = 0;

// =============================================================================
// INTERNAL HELPERS
// =============================================================================

/**
 * @brief Initialize a memory region
 */
static void init_region(GBAMemoryRegion* region, uint32_t size, uint32_t gba_base, const char* name) {
    region->base = (uint8_t*)calloc(1, size);
    region->size = size;
    region->gba_base_addr = gba_base;
    region->name = name;
}

/**
 * @brief Free a memory region
 */
static void free_region(GBAMemoryRegion* region) {
    if (region->base != NULL) {
        free(region->base);
        region->base = NULL;
    }
    region->size = 0;
}

/**
 * @brief Apply address mirroring based on region
 */
static uint32_t apply_mirroring(uint32_t addr, uint32_t base, uint32_t mask) {
    return base + ((addr - base) & mask);
}

/**
 * @brief Translate GBA address with mirroring to host pointer
 */
static uint8_t* translate_address_internal(uint32_t gba_addr) {
    if (!g_initialized) {
        return NULL;
    }

    // Mask to 24-bit address space
    gba_addr &= GBA_ADDRESS_MASK;

    // EWRAM (256KB) - mirrors every 256KB
    if (gba_addr >= GBA_EWRAM_ADDR && gba_addr < GBA_IWRAM_ADDR) {
        uint32_t offset = (gba_addr - GBA_EWRAM_ADDR) & GBA_EWRAM_MASK;
        if (offset < g_memory_context.ewram.size) {
            return g_memory_context.ewram.base + offset;
        }
    }
    // IWRAM (32KB) - mirrors every 32KB
    else if (gba_addr >= GBA_IWRAM_ADDR && gba_addr < GBA_IOREG_ADDR) {
        uint32_t offset = (gba_addr - GBA_IWRAM_ADDR) & GBA_IWRAM_MASK;
        if (offset < g_memory_context.iwram.size) {
            return g_memory_context.iwram.base + offset;
        }
    }
    // I/O Registers (1KB) - has gaps, not fully mirrored
    else if (gba_addr >= GBA_IOREG_ADDR && gba_addr < GBA_PALETTE_ADDR) {
        uint32_t offset = gba_addr - GBA_IOREG_ADDR;
        if (offset < g_memory_context.ioregs.size) {
            return g_memory_context.ioregs.base + offset;
        }
    }
    // Palette RAM (1KB) - mirrors every 1KB
    else if (gba_addr >= GBA_PALETTE_ADDR && gba_addr < GBA_VRAM_ADDR) {
        uint32_t offset = (gba_addr - GBA_PALETTE_ADDR) & GBA_PALETTE_MASK;
        if (offset < g_memory_context.palette.size) {
            return g_memory_context.palette.base + offset;
        }
    }
    // VRAM (96KB) - complex mirroring behavior
    // 0x06000000-0x06017FFF (96KB) - main VRAM
    // 0x06018000-0x0601FFFF (32KB) - mirrors 0x06000000-0x06007FFF
    else if (gba_addr >= GBA_VRAM_ADDR && gba_addr < GBA_OAM_ADDR) {
        uint32_t offset = gba_addr - GBA_VRAM_ADDR;
        // Handle VRAM mirroring: last 32KB mirrors first 32KB
        if (offset >= 0x18000) {
            offset = offset & 0x7FFF;  // Mirror to first 32KB
        }
        if (offset < g_memory_context.vram.size) {
            return g_memory_context.vram.base + offset;
        }
    }
    // OAM (1KB) - mirrors every 1KB
    else if (gba_addr >= GBA_OAM_ADDR && gba_addr < GBA_ROM_ADDR) {
        uint32_t offset = (gba_addr - GBA_OAM_ADDR) & GBA_OAM_MASK;
        if (offset < g_memory_context.oam.size) {
            return g_memory_context.oam.base + offset;
        }
    }
    // ROM - up to 32MB, no mirroring
    else if (gba_addr >= GBA_ROM_ADDR && gba_addr < GBA_SRAM_ADDR) {
        if (g_memory_context.rom.base != NULL) {
            uint32_t offset = gba_addr - GBA_ROM_ADDR;
            if (offset < g_memory_context.rom.size) {
                return g_memory_context.rom.base + offset;
            }
        }
    }
    // SRAM (128KB) - only in 0x0E000000-0x0E00FFFF range
    else if (gba_addr >= GBA_SRAM_ADDR && gba_addr < (GBA_SRAM_ADDR + GBA_SRAM_SIZE)) {
        uint32_t offset = gba_addr - GBA_SRAM_ADDR;
        if (offset < g_memory_context.sram.size) {
            return g_memory_context.sram.base + offset;
        }
    }

    return NULL;
}

/**
 * @brief Check if address is in ROM region
 */
static int is_rom_address(uint32_t gba_addr) {
    gba_addr &= GBA_ADDRESS_MASK;
    return (gba_addr >= GBA_ROM_ADDR && gba_addr < GBA_SRAM_ADDR);
}

// =============================================================================
// CORE FUNCTIONS
// =============================================================================

void gba_memory_init(void) {
    if (g_initialized) {
        return;  // Already initialized
    }

    // Clear context
    memset(&g_memory_context, 0, sizeof(g_memory_context));

    // Initialize all memory regions
    init_region(&g_memory_context.ewram, GBA_EWRAM_SIZE, GBA_EWRAM_ADDR, "EWRAM");
    init_region(&g_memory_context.iwram, GBA_IWRAM_SIZE, GBA_IWRAM_ADDR, "IWRAM");
    init_region(&g_memory_context.ioregs, GBA_IOREG_SIZE, GBA_IOREG_ADDR, "IOREGS");
    init_region(&g_memory_context.palette, GBA_PALETTE_SIZE, GBA_PALETTE_ADDR, "PALETTE");
    init_region(&g_memory_context.vram, GBA_VRAM_SIZE, GBA_VRAM_ADDR, "VRAM");
    init_region(&g_memory_context.oam, GBA_OAM_SIZE, GBA_OAM_ADDR, "OAM");
    init_region(&g_memory_context.sram, GBA_SRAM_SIZE, GBA_SRAM_ADDR, "SRAM");
    
    // ROM is not pre-allocated - loaded on demand
    g_memory_context.rom.base = NULL;
    g_memory_context.rom.size = 0;
    g_memory_context.rom.gba_base_addr = GBA_ROM_ADDR;
    g_memory_context.rom.name = "ROM";

    // Initialize I/O registers to default values
    // Display control - forced blank
    if (g_memory_context.ioregs.base != NULL) {
        g_memory_context.ioregs.base[GBA_REG_DISPCNT] = 0x80;  // FORCED_BLANK
        g_memory_context.ioregs.base[GBA_REG_DISPCNT + 1] = 0x00;
    }

    g_initialized = 1;
    g_memory_context.initialized = 1;
}

void gba_memory_shutdown(void) {
    if (!g_initialized) {
        return;
    }

    // Free all allocated regions
    free_region(&g_memory_context.ewram);
    free_region(&g_memory_context.iwram);
    free_region(&g_memory_context.ioregs);
    free_region(&g_memory_context.palette);
    free_region(&g_memory_context.vram);
    free_region(&g_memory_context.oam);
    free_region(&g_memory_context.rom);
    free_region(&g_memory_context.sram);

    g_initialized = 0;
    g_memory_context.initialized = 0;
}

GBAMemoryContext* gba_memory_get_context(void) {
    if (!g_initialized) {
        return NULL;
    }
    return &g_memory_context;
}

// =============================================================================
// ADDRESS TRANSLATION
// =============================================================================

uint8_t* gba_ptr(uint32_t gba_address) {
    return translate_address_internal(gba_address);
}

bool gba_is_address_valid(uint32_t gba_address) {
    return translate_address_internal(gba_address) != NULL;
}

const GBAMemoryRegion* gba_get_region(uint32_t gba_address) {
    if (!g_initialized) {
        return NULL;
    }

    gba_address &= GBA_ADDRESS_MASK;

    if (gba_address >= GBA_EWRAM_ADDR && gba_address < GBA_IWRAM_ADDR) {
        return &g_memory_context.ewram;
    } else if (gba_address >= GBA_IWRAM_ADDR && gba_address < GBA_IOREG_ADDR) {
        return &g_memory_context.iwram;
    } else if (gba_address >= GBA_IOREG_ADDR && gba_address < GBA_PALETTE_ADDR) {
        return &g_memory_context.ioregs;
    } else if (gba_address >= GBA_PALETTE_ADDR && gba_address < GBA_VRAM_ADDR) {
        return &g_memory_context.palette;
    } else if (gba_address >= GBA_VRAM_ADDR && gba_address < GBA_OAM_ADDR) {
        return &g_memory_context.vram;
    } else if (gba_address >= GBA_OAM_ADDR && gba_address < GBA_ROM_ADDR) {
        return &g_memory_context.oam;
    } else if (gba_address >= GBA_ROM_ADDR && gba_address < GBA_SRAM_ADDR) {
        return &g_memory_context.rom;
    } else if (gba_address >= GBA_SRAM_ADDR && gba_address < (GBA_SRAM_ADDR + GBA_SRAM_SIZE)) {
        return &g_memory_context.sram;
    }

    return NULL;
}

// =============================================================================
// MEMORY READ OPERATIONS (Little-endian)
// =============================================================================

uint8_t gba_read8(uint32_t addr) {
    uint8_t* ptr = translate_address_internal(addr);
    return ptr ? *ptr : 0;
}

uint16_t gba_read16(uint32_t addr) {
    uint8_t* ptr = translate_address_internal(addr);
    if (!ptr) return 0;
    
    // GBA uses little-endian
    return (uint16_t)ptr[0] | ((uint16_t)ptr[1] << 8);
}

uint32_t gba_read32(uint32_t addr) {
    uint8_t* ptr = translate_address_internal(addr);
    if (!ptr) return 0;
    
    // GBA uses little-endian
    return (uint32_t)ptr[0] | 
           ((uint32_t)ptr[1] << 8) | 
           ((uint32_t)ptr[2] << 16) | 
           ((uint32_t)ptr[3] << 24);
}

// =============================================================================
// MEMORY WRITE OPERATIONS (Little-endian)
// =============================================================================

void gba_write8(uint32_t addr, uint8_t val) {
    // ROM is read-only
    if (is_rom_address(addr)) {
        return;
    }

    uint8_t* ptr = translate_address_internal(addr);
    if (ptr) {
        *ptr = val;
    }
}

void gba_write16(uint32_t addr, uint16_t val) {
    // ROM is read-only
    if (is_rom_address(addr)) {
        return;
    }

    uint8_t* ptr = translate_address_internal(addr);
    if (ptr) {
        // GBA uses little-endian
        ptr[0] = (uint8_t)(val & 0xFF);
        ptr[1] = (uint8_t)((val >> 8) & 0xFF);
    }
}

void gba_write32(uint32_t addr, uint32_t val) {
    // ROM is read-only
    if (is_rom_address(addr)) {
        return;
    }

    uint8_t* ptr = translate_address_internal(addr);
    if (ptr) {
        // GBA uses little-endian
        ptr[0] = (uint8_t)(val & 0xFF);
        ptr[1] = (uint8_t)((val >> 8) & 0xFF);
        ptr[2] = (uint8_t)((val >> 16) & 0xFF);
        ptr[3] = (uint8_t)((val >> 24) & 0xFF);
    }
}

// =============================================================================
// ROM OPERATIONS
// =============================================================================

bool gba_load_rom(const uint8_t* data, size_t size) {
    if (!g_initialized) {
        return false;
    }

    if (data == NULL || size == 0 || size > GBA_MAX_ROM_SIZE) {
        return false;
    }

    // Free existing ROM if any
    if (g_memory_context.rom.base != NULL) {
        free(g_memory_context.rom.base);
    }

    // Allocate and copy new ROM
    g_memory_context.rom.base = (uint8_t*)malloc(size);
    if (g_memory_context.rom.base == NULL) {
        g_memory_context.rom.size = 0;
        return false;
    }

    memcpy(g_memory_context.rom.base, data, size);
    g_memory_context.rom.size = (uint32_t)size;

    return true;
}

uint8_t* gba_get_rom_ptr(void) {
    if (!g_initialized) {
        return NULL;
    }
    return g_memory_context.rom.base;
}

uint32_t gba_get_rom_size(void) {
    if (!g_initialized) {
        return 0;
    }
    return g_memory_context.rom.size;
}

void gba_unload_rom(void) {
    if (!g_initialized) {
        return;
    }

    if (g_memory_context.rom.base != NULL) {
        free(g_memory_context.rom.base);
        g_memory_context.rom.base = NULL;
    }
    g_memory_context.rom.size = 0;
}

// =============================================================================
// REGION-SPECIFIC ACCESSORS
// =============================================================================

uint8_t* gba_get_ewram_ptr(void) {
    return g_initialized ? g_memory_context.ewram.base : NULL;
}

uint8_t* gba_get_iwram_ptr(void) {
    return g_initialized ? g_memory_context.iwram.base : NULL;
}

uint8_t* gba_get_ioregs_ptr(void) {
    return g_initialized ? g_memory_context.ioregs.base : NULL;
}

uint8_t* gba_get_palette_ptr(void) {
    return g_initialized ? g_memory_context.palette.base : NULL;
}

uint8_t* gba_get_vram_ptr(void) {
    return g_initialized ? g_memory_context.vram.base : NULL;
}

uint8_t* gba_get_oam_ptr(void) {
    return g_initialized ? g_memory_context.oam.base : NULL;
}

uint8_t* gba_get_sram_ptr(void) {
    return g_initialized ? g_memory_context.sram.base : NULL;
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

void gba_memory_clear_region(uint32_t gba_base_addr) {
    if (!g_initialized) {
        return;
    }

    GBAMemoryRegion* region = NULL;

    switch (gba_base_addr & GBA_ADDRESS_MASK) {
        case GBA_EWRAM_ADDR:
            region = &g_memory_context.ewram;
            break;
        case GBA_IWRAM_ADDR:
            region = &g_memory_context.iwram;
            break;
        case GBA_IOREG_ADDR:
            region = &g_memory_context.ioregs;
            break;
        case GBA_PALETTE_ADDR:
            region = &g_memory_context.palette;
            break;
        case GBA_VRAM_ADDR:
            region = &g_memory_context.vram;
            break;
        case GBA_OAM_ADDR:
            region = &g_memory_context.oam;
            break;
        case GBA_SRAM_ADDR:
            region = &g_memory_context.sram;
            break;
        default:
            return;
    }

    if (region != NULL && region->base != NULL) {
        memset(region->base, 0, region->size);
    }
}

void gba_memory_reset(void) {
    if (!g_initialized) {
        return;
    }

    // Clear all writable regions (not ROM)
    if (g_memory_context.ewram.base != NULL) {
        memset(g_memory_context.ewram.base, 0, g_memory_context.ewram.size);
    }
    if (g_memory_context.iwram.base != NULL) {
        memset(g_memory_context.iwram.base, 0, g_memory_context.iwram.size);
    }
    if (g_memory_context.ioregs.base != NULL) {
        memset(g_memory_context.ioregs.base, 0, g_memory_context.ioregs.size);
    }
    if (g_memory_context.palette.base != NULL) {
        memset(g_memory_context.palette.base, 0, g_memory_context.palette.size);
    }
    if (g_memory_context.vram.base != NULL) {
        memset(g_memory_context.vram.base, 0, g_memory_context.vram.size);
    }
    if (g_memory_context.oam.base != NULL) {
        memset(g_memory_context.oam.base, 0, g_memory_context.oam.size);
    }
    if (g_memory_context.sram.base != NULL) {
        memset(g_memory_context.sram.base, 0, g_memory_context.sram.size);
    }
}

size_t gba_memcpy_to(uint32_t gba_dest, const uint8_t* src, size_t size) {
    if (!g_initialized || src == NULL || size == 0) {
        return 0;
    }

    // Check bounds and copy byte by byte to handle mirroring correctly
    size_t copied = 0;
    for (size_t i = 0; i < size; i++) {
        uint8_t* ptr = translate_address_internal(gba_dest + i);
        if (ptr != NULL && !is_rom_address(gba_dest + i)) {
            *ptr = src[i];
            copied++;
        } else {
            break;  // Stop at first invalid address
        }
    }

    return copied;
}

size_t gba_memcpy_from(uint8_t* dest, uint32_t gba_src, size_t size) {
    if (!g_initialized || dest == NULL || size == 0) {
        return 0;
    }

    // Check bounds and copy byte by byte to handle mirroring correctly
    size_t copied = 0;
    for (size_t i = 0; i < size; i++) {
        uint8_t* ptr = translate_address_internal(gba_src + i);
        if (ptr != NULL) {
            dest[i] = *ptr;
            copied++;
        } else {
            break;  // Stop at first invalid address
        }
    }

    return copied;
}

#ifdef __cplusplus
}
#endif

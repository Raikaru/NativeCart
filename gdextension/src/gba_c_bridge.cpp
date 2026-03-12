/**
 * @file gba_c_bridge.cpp
 * @brief Bridge between Godot C++ and pokefirered C code
 * 
 * This file provides the interface layer that allows the original GBA C code
 * to run within the Godot environment. It handles:
 * - Memory mapping (EWRAM, IWRAM, VRAM, etc.)
 * - Hardware register emulation
 * - Interrupt handling
 * - Input/output redirection
 */

// 1. NOMINMAX guard
#define NOMINMAX

// 2. Standard C++ headers
#include <cstring>
#include <cstdlib>
#include <cstdio>

#include "../include/gba_memory_map.hpp"
#include "../include/gba_asset_offsets.h"

using namespace GBA;

// Forward declarations for asset initialization
extern "C" void gba_assets_init(const uint8_t* rom_buffer, size_t rom_size);
extern "C" void gba_assets_shutdown(void);

// =============================================================================
// GLOBAL MEMORY CONTEXT
// =============================================================================

namespace {
    // Main memory context - holds all GBA memory regions
    struct MemoryContext {
        MemoryRegion ewram;      // External Work RAM (256KB)
        MemoryRegion iwram;      // Internal Work RAM (32KB)
        MemoryRegion ioReg;      // I/O Registers (1KB)
        MemoryRegion palette;    // Palette RAM (1KB)
        MemoryRegion vram;       // Video RAM (96KB)
        MemoryRegion oam;        // Object Attribute Memory (1KB)
        MemoryRegion rom;        // Game ROM (loaded from file)
        MemoryRegion sram;       // Save RAM (Flash/SRAM 128KB)
        
        // Helper to translate GBA address to host pointer
        uint8_t* translateAddress(uint32_t gbaAddr) const {
            // Mask to 24-bit address space
            gbaAddr &= 0x00FFFFFF;
            
            if (gbaAddr >= GBA_EWRAM_ADDR && gbaAddr < GBA_EWRAM_ADDR + EWRAM_SIZE) {
                return ewram.base + (gbaAddr - GBA_EWRAM_ADDR);
            }
            else if (gbaAddr >= GBA_IWRAM_ADDR && gbaAddr < GBA_IWRAM_ADDR + IWRAM_SIZE) {
                return iwram.base + (gbaAddr - GBA_IWRAM_ADDR);
            }
            else if (gbaAddr >= GBA_IOREG_ADDR && gbaAddr < GBA_IOREG_ADDR + IOREG_SIZE) {
                return ioReg.base + (gbaAddr - GBA_IOREG_ADDR);
            }
            else if (gbaAddr >= GBA_PLTT_ADDR && gbaAddr < GBA_PLTT_ADDR + PLTT_SIZE) {
                return palette.base + (gbaAddr - GBA_PLTT_ADDR);
            }
            else if (gbaAddr >= GBA_VRAM_ADDR && gbaAddr < GBA_VRAM_ADDR + VRAM_SIZE) {
                return vram.base + (gbaAddr - GBA_VRAM_ADDR);
            }
            else if (gbaAddr >= GBA_OAM_ADDR && gbaAddr < GBA_OAM_ADDR + OAM_SIZE) {
                return oam.base + (gbaAddr - GBA_OAM_ADDR);
            }
            else if (gbaAddr >= GBA_ROM_ADDR && gbaAddr < GBA_ROM_ADDR + rom.size) {
                return rom.base + (gbaAddr - GBA_ROM_ADDR);
            }
            else if (gbaAddr >= GBA_SRAM_ADDR && gbaAddr < GBA_SRAM_ADDR + SRAM_SIZE) {
                return sram.base + (gbaAddr - GBA_SRAM_ADDR);
            }
            
            // Return null for unmapped addresses
            return nullptr;
        }
    };
    
    // Global memory context instance
    MemoryContext* g_memoryContext = nullptr;
}

// =============================================================================
// INITIALIZATION
// =============================================================================

/**
 * @brief Initialize the GBA C runtime bridge
 * 
 * Allocates all memory regions needed by the GBA code.
 * Must be called before any GBA code is executed.
 */
extern "C" void gba_c_runtime_init() {
    if (g_memoryContext != nullptr) {
        // Already initialized
        return;
    }
    
    g_memoryContext = new MemoryContext();
    
    // Allocate memory regions
    g_memoryContext->ewram.base = new uint8_t[EWRAM_SIZE]();
    g_memoryContext->ewram.size = EWRAM_SIZE;
    g_memoryContext->ewram.gbaBaseAddr = GBA_EWRAM_ADDR;
    g_memoryContext->ewram.name = "EWRAM";
    
    g_memoryContext->iwram.base = new uint8_t[IWRAM_SIZE]();
    g_memoryContext->iwram.size = IWRAM_SIZE;
    g_memoryContext->iwram.gbaBaseAddr = GBA_IWRAM_ADDR;
    g_memoryContext->iwram.name = "IWRAM";
    
    g_memoryContext->ioReg.base = new uint8_t[IOREG_SIZE]();
    g_memoryContext->ioReg.size = IOREG_SIZE;
    g_memoryContext->ioReg.gbaBaseAddr = GBA_IOREG_ADDR;
    g_memoryContext->ioReg.name = "IOREG";
    
    g_memoryContext->palette.base = new uint8_t[PLTT_SIZE]();
    g_memoryContext->palette.size = PLTT_SIZE;
    g_memoryContext->palette.gbaBaseAddr = GBA_PLTT_ADDR;
    g_memoryContext->palette.name = "PALETTE";
    
    g_memoryContext->vram.base = new uint8_t[VRAM_SIZE]();
    g_memoryContext->vram.size = VRAM_SIZE;
    g_memoryContext->vram.gbaBaseAddr = GBA_VRAM_ADDR;
    g_memoryContext->vram.name = "VRAM";
    
    g_memoryContext->oam.base = new uint8_t[OAM_SIZE]();
    g_memoryContext->oam.size = OAM_SIZE;
    g_memoryContext->oam.gbaBaseAddr = GBA_OAM_ADDR;
    g_memoryContext->oam.name = "OAM";
    
    g_memoryContext->rom.base = nullptr;  // Will be loaded later
    g_memoryContext->rom.size = 0;
    g_memoryContext->rom.gbaBaseAddr = GBA_ROM_ADDR;
    g_memoryContext->rom.name = "ROM";
    
    g_memoryContext->sram.base = new uint8_t[SRAM_SIZE]();
    g_memoryContext->sram.size = SRAM_SIZE;
    g_memoryContext->sram.gbaBaseAddr = GBA_SRAM_ADDR;
    g_memoryContext->sram.name = "SRAM";
    
    // Initialize I/O registers to default values
    // Display control - forced blank
    GBA_REG16(g_memoryContext, IO::DISPCNT) = DISPCNT::FORCED_BLANK;
    
    // All interrupts disabled
    GBA_REG16(g_memoryContext, IO::IE) = 0;
    GBA_REG16(g_memoryContext, IO::IME) = 0;
}

/**
 * @brief Shutdown the GBA C runtime bridge
 * 
 * Frees all allocated memory regions.
 */
extern "C" void gba_c_runtime_shutdown() {
    if (g_memoryContext == nullptr) {
        return;
    }
    
    delete[] g_memoryContext->ewram.base;
    delete[] g_memoryContext->iwram.base;
    delete[] g_memoryContext->ioReg.base;
    delete[] g_memoryContext->palette.base;
    delete[] g_memoryContext->vram.base;
    delete[] g_memoryContext->oam.base;
    delete[] g_memoryContext->rom.base;
    delete[] g_memoryContext->sram.base;
    
    delete g_memoryContext;
    g_memoryContext = nullptr;
}

/**
 * @brief Load a ROM file into memory
 * 
 * @param romData Pointer to ROM data
 * @param size Size of ROM data in bytes
 * @return true if successful, false otherwise
 */
// Forward declarations - actual implementations are in gba_memory.cpp
extern "C" bool gba_load_rom(const uint8_t* romData, size_t size);
extern "C" uint8_t gba_read8(uint32_t addr);
extern "C" uint16_t gba_read16(uint32_t addr);
extern "C" uint32_t gba_read32(uint32_t addr);
extern "C" void gba_write8(uint32_t addr, uint8_t value);
extern "C" void gba_write16(uint32_t addr, uint16_t value);
extern "C" void gba_write32(uint32_t addr, uint32_t value);

// =============================================================================
// VIDEO RENDERING HELPERS
// =============================================================================

/**
 * @brief Get pointer to VRAM
 */
extern "C" uint8_t* gba_get_vram() {
    if (g_memoryContext == nullptr) return nullptr;
    return g_memoryContext->vram.base;
}

/**
 * @brief Get pointer to palette RAM
 */
extern "C" uint8_t* gba_get_palette() {
    if (g_memoryContext == nullptr) return nullptr;
    return g_memoryContext->palette.base;
}

/**
 * @brief Get pointer to OAM
 */
extern "C" uint8_t* gba_get_oam() {
    if (g_memoryContext == nullptr) return nullptr;
    return g_memoryContext->oam.base;
}

/**
 * @brief Get current display mode
 */
extern "C" int gba_get_display_mode() {
    if (g_memoryContext == nullptr) return 0;
    return GBA_REG16(g_memoryContext, IO::DISPCNT) & DISPCNT::MODE_MASK;
}

// =============================================================================
// INPUT HANDLING
// =============================================================================

static uint16_t g_currentInput = INPUT::ALL_KEYS;  // All keys released (inverted logic)

/**
 * @brief Set the current input state
 * 
 * @param keys Bitmask of currently pressed keys (using INPUT constants)
 */
extern "C" void gba_set_input(uint16_t keys) {
    g_currentInput = ~keys & INPUT::ALL_KEYS;  // Invert for GBA (0 = pressed)
}

/**
 * @brief Get current input state for KEYINPUT register
 */
extern "C" uint16_t gba_get_input() {
    return g_currentInput;
}

// =============================================================================
// INTERRUPT HANDLING
// =============================================================================

/**
 * @brief Trigger a VBlank interrupt
 */
extern "C" void gba_trigger_vblank() {
    if (g_memoryContext == nullptr) return;
    
    // Set VBlank flag in DISPSTAT
    GBA_REG16(g_memoryContext, IO::DISPSTAT) |= 0x0001;
    
    // Check if VBlank interrupt is enabled
    if ((GBA_REG16(g_memoryContext, IO::DISPSTAT) & 0x0008) &&  // VBlank IRQ enable
        (GBA_REG16(g_memoryContext, IO::IE) & INTR::VBLANK) &&   // VBlank interrupt enabled
        (GBA_REG16(g_memoryContext, IO::IME) & 0x0001)) {        // Master interrupt enable
        // Set interrupt flag
        GBA_REG16(g_memoryContext, IO::IF) |= INTR::VBLANK;
    }
}

/**
 * @brief Check if an interrupt should be handled
 */
extern "C" bool gba_check_interrupt() {
    if (g_memoryContext == nullptr) return false;
    
    uint16_t ie = GBA_REG16(g_memoryContext, IO::IE);
    uint16_t irqFlag = GBA_REG16(g_memoryContext, IO::IF);
    uint16_t ime = GBA_REG16(g_memoryContext, IO::IME);
    
    return (ime & 0x0001) && (ie & irqFlag);
}

// =============================================================================
// ASSET INITIALIZATION (Runtime ROM extraction)
// =============================================================================

// Extern declarations for transformed asset pointers
// These are defined in the transformed pokefirered source files
// Example from bag.c: extern const u16 *sBagWindowPalF;

// Asset tracking for bounds checking
typedef struct {
    const char* name;
    void** pointer;
    uint32_t rom_offset;
    uint32_t size_bytes;
    uint8_t type_size;
} AssetMapping;

static AssetMapping* g_assetMappings = nullptr;
static size_t g_assetCount = 0;

/**
 * @brief Get the size of an asset by looking up its binary file
 */
static uint32_t get_asset_size_from_registry(const char* varname) {
    // The size is already stored in g_asset_offsets from rom_offset_mapper.py
    for (int i = 0; g_asset_offsets[i].varname != nullptr; i++) {
        if (strcmp(g_asset_offsets[i].varname, varname) == 0) {
            return g_asset_offsets[i].size_bytes;
        }
    }
    return 0;
}

/**
 * @brief Scan ROM for an asset by binary fingerprint
 * 
 * Uses the first 32 bytes of the original asset file as a fingerprint
 * to locate the asset in the ROM.
 * 
 * @param rom_buffer ROM data
 * @param rom_size ROM size
 * @param varname Asset variable name
 * @return ROM offset if found, 0xFFFFFFFF otherwise
 */
static uint32_t scan_rom_for_asset_runtime(
    const uint8_t* rom_buffer,
    size_t rom_size,
    const char* varname
) {
    // This is a simplified runtime scanner
    // In production, you'd use a more sophisticated approach with pre-computed hashes
    
    // For now, we return OFFSET_RUNTIME_DISCOVERY marker
    // The actual implementation would search using asset fingerprints
    (void)rom_buffer;
    (void)rom_size;
    (void)varname;
    
    return 0xFFFFFFFF;  // OFFSET_RUNTIME_DISCOVERY
}

/**
 * @brief Initialize all asset pointers from ROM buffer
 * 
 * This function is called after the ROM is loaded. It populates all the
 * static asset pointers that were transformed from INCBIN declarations.
 * 
 * @param rom_buffer Pointer to loaded ROM data
 * @param rom_size Size of ROM data in bytes
 */
extern "C" void gba_assets_init(const uint8_t* rom_buffer, size_t rom_size) {
    if (rom_buffer == nullptr || rom_size == 0) {
        fprintf(stderr, "gba_assets_init: Invalid ROM buffer\n");
        return;
    }
    
    // Count assets
    size_t count = 0;
    for (int i = 0; g_asset_offsets[i].varname != nullptr; i++) {
        count++;
    }
    
    if (count == 0) {
        printf("gba_assets_init: No assets to initialize\n");
        return;
    }
    
    printf("gba_assets_init: Initializing %zu assets from ROM...\n", count);
    
    // Allocate tracking array
    g_assetMappings = (AssetMapping*)malloc(sizeof(AssetMapping) * count);
    g_assetCount = 0;
    
    int successCount = 0;
    int runtimeDiscoveryCount = 0;
    int failedCount = 0;
    
    // Initialize each asset
    for (int i = 0; g_asset_offsets[i].varname != nullptr; i++) {
        const AssetOffsetEntry* entry = &g_asset_offsets[i];
        const char* varname = entry->varname;
        uint32_t rom_offset = entry->rom_offset;
        uint32_t size = entry->size_bytes;
        uint8_t type_size = entry->type_size;
        
        // Check if offset needs runtime discovery
        if (rom_offset == 0xFFFFFFFF || rom_offset == 0) {
            // Try to discover at runtime
            rom_offset = scan_rom_for_asset_runtime(rom_buffer, rom_size, varname);
            
            if (rom_offset == 0xFFFFFFFF) {
                fprintf(stderr, "  Asset '%s' requires runtime discovery (not implemented)\n", varname);
                runtimeDiscoveryCount++;
                continue;
            }
        }
        
        // Bounds check
        if (rom_offset + size > rom_size) {
            fprintf(stderr, "  Asset '%s' offset (0x%06X) out of ROM bounds\n", 
                    varname, rom_offset);
            failedCount++;
            continue;
        }
        
        // Calculate pointer
        void* asset_ptr = (void*)(rom_buffer + rom_offset);
        
        // Create extern declaration and assign
        // We need to use the actual symbol name from the transformed source
        // This is done via extern declarations
        
        // For now, we track it - actual assignment happens via generated code
        g_assetMappings[g_assetCount].name = varname;
        g_assetMappings[g_assetCount].pointer = nullptr;  // Will be set by generated code
        g_assetMappings[g_assetCount].rom_offset = rom_offset;
        g_assetMappings[g_assetCount].size_bytes = size;
        g_assetMappings[g_assetCount].type_size = type_size;
        g_assetCount++;
        
        successCount++;
    }
    
    printf("gba_assets_init: %d succeeded, %d pending discovery, %d failed\n",
           successCount, runtimeDiscoveryCount, failedCount);
}

/**
 * @brief Shutdown asset system
 */
extern "C" void gba_assets_shutdown(void) {
    if (g_assetMappings != nullptr) {
        free(g_assetMappings);
        g_assetMappings = nullptr;
    }
    g_assetCount = 0;
}

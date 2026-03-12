/**
 * @file gba_memory.h
 * @brief GBA Memory System - C Interface Header
 * 
 * This header provides a C-compatible interface for the GBA memory overlay system.
 * It implements virtual memory mapping for all GBA memory regions with host-addressable
 * backing storage.
 * 
 * Memory Regions:
 * - EWRAM (256KB)   : External Work RAM at 0x02000000
 * - IWRAM (32KB)    : Internal Work RAM at 0x03000000
 * - IOREGS (1KB)    : I/O Registers at 0x04000000
 * - PALETTE (1KB)   : Palette RAM at 0x05000000
 * - VRAM (96KB)     : Video RAM at 0x06000000
 * - OAM (1KB)       : Object Attribute Memory at 0x07000000
 * - ROM             : Game ROM (loaded from file)
 * 
 * @author GBA Memory System Engineer
 * @version 1.0.0
 */

#ifndef GBA_MEMORY_H
#define GBA_MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// GBA MEMORY REGION ADDRESSES
// =============================================================================

#define GBA_EWRAM_ADDR      0x02000000U  // External Work RAM (256KB)
#define GBA_IWRAM_ADDR      0x03000000U  // Internal Work RAM (32KB)
#define GBA_IOREG_ADDR      0x04000000U  // I/O Registers (1KB)
#define GBA_PALETTE_ADDR    0x05000000U  // Palette RAM (1KB)
#define GBA_VRAM_ADDR       0x06000000U  // Video RAM (96KB)
#define GBA_OAM_ADDR        0x07000000U  // Object Attribute Memory (1KB)
#define GBA_ROM_ADDR        0x08000000U  // Game ROM
#define GBA_SRAM_ADDR       0x0E000000U  // Save RAM (Flash/SRAM 128KB)

// =============================================================================
// GBA MEMORY REGION SIZES
// =============================================================================

#define GBA_EWRAM_SIZE      0x40000U     // 256 KB
#define GBA_IWRAM_SIZE      0x08000U     // 32 KB
#define GBA_IOREG_SIZE      0x00400U     // 1 KB
#define GBA_PALETTE_SIZE    0x00400U     // 1 KB
#define GBA_VRAM_SIZE       0x18000U     // 96 KB
#define GBA_OAM_SIZE        0x00400U     // 1 KB
#define GBA_SRAM_SIZE       0x20000U     // 128 KB
#define GBA_MAX_ROM_SIZE    0x2000000U   // 32 MB

// =============================================================================
// ADDRESS MASKS FOR MIRRORING
// =============================================================================

#define GBA_ADDRESS_MASK    0x00FFFFFFU  // 24-bit address space
#define GBA_EWRAM_MASK      0x0003FFFFU  // 256KB mirroring
#define GBA_IWRAM_MASK      0x00007FFFU  // 32KB mirroring
#define GBA_PALETTE_MASK    0x000003FFU  // 1KB mirroring
#define GBA_OAM_MASK        0x000003FFU  // 1KB mirroring
#define GBA_IOREG_MASK      0x000003FFU  // 1KB mirroring (with gaps)
#define GBA_VRAM_MASK       0x0001FFFFU  // 96KB with mirroring behavior

// =============================================================================
// I/O REGISTER OFFSETS
// =============================================================================

typedef enum {
    GBA_REG_DISPCNT     = 0x000,  // Display control
    GBA_REG_DISPSTAT    = 0x004,  // Display status
    GBA_REG_VCOUNT      = 0x006,  // Vertical line counter
    GBA_REG_BG0CNT      = 0x008,  // Background 0 control
    GBA_REG_BG1CNT      = 0x00A,  // Background 1 control
    GBA_REG_BG2CNT      = 0x00C,  // Background 2 control
    GBA_REG_BG3CNT      = 0x00E,  // Background 3 control
    GBA_REG_BG0HOFS     = 0x010,  // Background 0 H offset
    GBA_REG_BG0VOFS     = 0x012,  // Background 0 V offset
    GBA_REG_BG1HOFS     = 0x014,  // Background 1 H offset
    GBA_REG_BG1VOFS     = 0x016,  // Background 1 V offset
    GBA_REG_BG2HOFS     = 0x018,  // Background 2 H offset
    GBA_REG_BG2VOFS     = 0x01A,  // Background 2 V offset
    GBA_REG_BG3HOFS     = 0x01C,  // Background 3 H offset
    GBA_REG_BG3VOFS     = 0x01E,  // Background 3 V offset
    GBA_REG_WIN0H       = 0x040,  // Window 0 H bounds
    GBA_REG_WIN1H       = 0x042,  // Window 1 H bounds
    GBA_REG_WIN0V       = 0x044,  // Window 0 V bounds
    GBA_REG_WIN1V       = 0x046,  // Window 1 V bounds
    GBA_REG_WININ       = 0x048,  // Window inside control
    GBA_REG_WINOUT      = 0x04A,  // Window outside control
    GBA_REG_MOSAIC      = 0x04C,  // Mosaic control
    GBA_REG_BLDCNT      = 0x050,  // Blend control
    GBA_REG_BLDALPHA    = 0x052,  // Blend alpha
    GBA_REG_BLDY        = 0x054,  // Blend Y
    GBA_REG_IE          = 0x200,  // Interrupt enable
    GBA_REG_IF          = 0x202,  // Interrupt request flags
    GBA_REG_IME         = 0x208,  // Interrupt master enable
    GBA_REG_KEYINPUT    = 0x130,  // Key input
    GBA_REG_KEYCNT      = 0x132,  // Key control
    GBA_REG_DMA0SAD     = 0x0B0,  // DMA 0 source address
    GBA_REG_DMA0DAD     = 0x0B4,  // DMA 0 destination address
    GBA_REG_DMA0CNT     = 0x0B8,  // DMA 0 control
    GBA_REG_DMA1SAD     = 0x0BC,  // DMA 1 source address
    GBA_REG_DMA1DAD     = 0x0C0,  // DMA 1 destination address
    GBA_REG_DMA1CNT     = 0x0C4,  // DMA 1 control
    GBA_REG_DMA2SAD     = 0x0C8,  // DMA 2 source address
    GBA_REG_DMA2DAD     = 0x0CC,  // DMA 2 destination address
    GBA_REG_DMA2CNT     = 0x0D0,  // DMA 2 control
    GBA_REG_DMA3SAD     = 0x0D4,  // DMA 3 source address
    GBA_REG_DMA3DAD     = 0x0D8,  // DMA 3 destination address
    GBA_REG_DMA3CNT     = 0x0DC,  // DMA 3 control
    GBA_REG_TM0CNT      = 0x100,  // Timer 0 control
    GBA_REG_TM1CNT      = 0x104,  // Timer 1 control
    GBA_REG_TM2CNT      = 0x108,  // Timer 2 control
    GBA_REG_TM3CNT      = 0x10C,  // Timer 3 control
} GBA_IORegister;

// =============================================================================
// MEMORY REGION HANDLE
// =============================================================================

/**
 * @brief Memory region descriptor
 */
typedef struct {
    uint8_t*    base;           // Host virtual address
    uint32_t    size;           // Region size in bytes
    uint32_t    gba_base_addr;  // Original GBA base address
    const char* name;           // Human-readable name
} GBAMemoryRegion;

/**
 * @brief Complete GBA memory context
 */
typedef struct {
    GBAMemoryRegion ewram;      // External Work RAM (256KB)
    GBAMemoryRegion iwram;      // Internal Work RAM (32KB)
    GBAMemoryRegion ioregs;     // I/O Registers (1KB)
    GBAMemoryRegion palette;    // Palette RAM (1KB)
    GBAMemoryRegion vram;       // Video RAM (96KB)
    GBAMemoryRegion oam;        // Object Attribute Memory (1KB)
    GBAMemoryRegion rom;        // Game ROM
    GBAMemoryRegion sram;       // Save RAM (128KB)
    int             initialized; // Initialization flag
} GBAMemoryContext;

// =============================================================================
// CORE FUNCTIONS
// =============================================================================

/**
 * @brief Initialize the GBA memory system
 * 
 * Allocates all memory regions (EWRAM, IWRAM, IOREGS, PALETTE, VRAM, OAM, SRAM).
 * Must be called before any other gba_memory functions.
 */
void gba_memory_init(void);

/**
 * @brief Shutdown the GBA memory system
 * 
 * Frees all allocated memory regions.
 */
void gba_memory_shutdown(void);

/**
 * @brief Get the global memory context
 * 
 * @return Pointer to the global memory context, or NULL if not initialized
 */
GBAMemoryContext* gba_memory_get_context(void);

// =============================================================================
// ADDRESS TRANSLATION
// =============================================================================

/**
 * @brief Translate GBA address to host pointer
 * 
 * Converts a GBA memory address to a host-accessible pointer.
 * Handles address mirroring for appropriate memory regions.
 * 
 * @param gba_address GBA memory address (24-bit, 0x000000 - 0xFFFFFF)
 * @return Host pointer to the memory location, or NULL if unmapped
 */
uint8_t* gba_ptr(uint32_t gba_address);

/**
 * @brief Check if a GBA address is valid and mapped
 * 
 * @param gba_address GBA memory address
 * @return true if address is mapped, false otherwise
 */
bool gba_is_address_valid(uint32_t gba_address);

/**
 * @brief Get the memory region for a GBA address
 * 
 * @param gba_address GBA memory address
 * @return Pointer to the memory region descriptor, or NULL if unmapped
 */
const GBAMemoryRegion* gba_get_region(uint32_t gba_address);

// =============================================================================
// MEMORY READ OPERATIONS
// =============================================================================

/**
 * @brief Read 8-bit value from GBA memory
 * 
 * @param addr GBA memory address
 * @return 8-bit value at the address (0 if unmapped)
 */
uint8_t gba_read8(uint32_t addr);

/**
 * @brief Read 16-bit value from GBA memory (little-endian)
 * 
 * @param addr GBA memory address
 * @return 16-bit value at the address (0 if unmapped)
 */
uint16_t gba_read16(uint32_t addr);

/**
 * @brief Read 32-bit value from GBA memory (little-endian)
 * 
 * @param addr GBA memory address
 * @return 32-bit value at the address (0 if unmapped)
 */
uint32_t gba_read32(uint32_t addr);

// =============================================================================
// MEMORY WRITE OPERATIONS
// =============================================================================

/**
 * @brief Write 8-bit value to GBA memory
 * 
 * @param addr GBA memory address
 * @param val Value to write
 */
void gba_write8(uint32_t addr, uint8_t val);

/**
 * @brief Write 16-bit value to GBA memory (little-endian)
 * 
 * @param addr GBA memory address
 * @param val Value to write
 */
void gba_write16(uint32_t addr, uint16_t val);

/**
 * @brief Write 32-bit value to GBA memory (little-endian)
 * 
 * @param addr GBA memory address
 * @param val Value to write
 */
void gba_write32(uint32_t addr, uint32_t val);

// =============================================================================
// ROM OPERATIONS
// =============================================================================

/**
 * @brief Load ROM data into memory
 * 
 * Copies ROM data into the ROM memory region. Previous ROM data is freed.
 * ROM is read-only from GBA perspective (writes will be ignored).
 * 
 * @param data Pointer to ROM data
 * @param size Size of ROM data in bytes
 * @return true if successful, false otherwise
 */
bool gba_load_rom(const uint8_t* data, size_t size);

/**
 * @brief Get pointer to ROM data
 * 
 * @return Pointer to ROM base address, or NULL if no ROM loaded
 */
uint8_t* gba_get_rom_ptr(void);

/**
 * @brief Get the size of the loaded ROM
 * 
 * @return ROM size in bytes (0 if no ROM loaded)
 */
uint32_t gba_get_rom_size(void);

/**
 * @brief Unload the current ROM
 */
void gba_unload_rom(void);

// =============================================================================
// REGION-SPECIFIC ACCESSORS
// =============================================================================

/**
 * @brief Get pointer to EWRAM
 * @return Pointer to EWRAM base (256KB)
 */
uint8_t* gba_get_ewram_ptr(void);

/**
 * @brief Get pointer to IWRAM
 * @return Pointer to IWRAM base (32KB)
 */
uint8_t* gba_get_iwram_ptr(void);

/**
 * @brief Get pointer to I/O Registers
 * @return Pointer to IOREGS base (1KB)
 */
uint8_t* gba_get_ioregs_ptr(void);

/**
 * @brief Get pointer to Palette RAM
 * @return Pointer to PALETTE base (1KB)
 */
uint8_t* gba_get_palette_ptr(void);

/**
 * @brief Get pointer to VRAM
 * @return Pointer to VRAM base (96KB)
 */
uint8_t* gba_get_vram_ptr(void);

/**
 * @brief Get pointer to OAM
 * @return Pointer to OAM base (1KB)
 */
uint8_t* gba_get_oam_ptr(void);

/**
 * @brief Get pointer to SRAM
 * @return Pointer to SRAM base (128KB)
 */
uint8_t* gba_get_sram_ptr(void);

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

/**
 * @brief Clear (zero) a memory region
 * 
 * @param gba_base_addr Base address of the region to clear
 */
void gba_memory_clear_region(uint32_t gba_base_addr);

/**
 * @brief Reset all writable memory regions to zero
 * (EWRAM, IWRAM, PALETTE, VRAM, OAM, SRAM)
 */
void gba_memory_reset(void);

/**
 * @brief Copy data to GBA memory
 * 
 * @param gba_dest Destination GBA address
 * @param src Source data pointer
 * @param size Number of bytes to copy
 * @return Number of bytes actually copied
 */
size_t gba_memcpy_to(uint32_t gba_dest, const uint8_t* src, size_t size);

/**
 * @brief Copy data from GBA memory
 * 
 * @param dest Destination buffer
 * @param gba_src Source GBA address
 * @param size Number of bytes to copy
 * @return Number of bytes actually copied
 */
size_t gba_memcpy_from(uint8_t* dest, uint32_t gba_src, size_t size);

#ifdef __cplusplus
}
#endif

#endif // GBA_MEMORY_H

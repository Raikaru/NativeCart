#ifndef GBA_MEMORY_MAP_H
#define GBA_MEMORY_MAP_H

#include <cstdint>
#include <cstddef>

// =============================================================================
// GBA MEMORY ARCHITECTURE → MODERN 64-BIT MAPPING
// =============================================================================
// This header defines the memory layout mapping from the GBA's 16-bit ARM
// architecture to modern 64-bit execution environments.
//
// KEY ARCHITECTURAL DIFFERENCES RESOLVED:
// 1. 16-bit → 64-bit: Pointer sizes change from 4 bytes to 8 bytes
// 2. Memory alignment: ARM allows unaligned access in some modes; x64 requires alignment
// 3. Endianness: GBA is little-endian; modern CPUs match
// 4. Address space: GBA uses 24-bit addressing; we use full 64-bit virtual addresses
// =============================================================================

namespace GBA {

// =============================================================================
// ORIGINAL GBA MEMORY REGIONS (for reference)
// =============================================================================
constexpr uint32_t GBA_EWRAM_ADDR    = 0x02000000;  // External WRAM (256KB)
constexpr uint32_t GBA_IWRAM_ADDR    = 0x03000000;  // Internal WRAM (32KB)
constexpr uint32_t GBA_IOREG_ADDR    = 0x04000000;  // I/O Registers (1KB)
constexpr uint32_t GBA_PLTT_ADDR     = 0x05000000;  // Palette RAM (1KB)
constexpr uint32_t GBA_VRAM_ADDR     = 0x06000000;  // Video RAM (96KB)
constexpr uint32_t GBA_OAM_ADDR      = 0x07000000;  // Object Attribute Memory (1KB)
constexpr uint32_t GBA_ROM_ADDR      = 0x08000000;  // Game ROM (up to 32MB)
constexpr uint32_t GBA_SRAM_ADDR     = 0x0E000000;  // Save RAM (Flash/SRAM 128KB)

// =============================================================================
// MEMORY REGION SIZES
// =============================================================================
constexpr size_t EWRAM_SIZE          = 0x40000;     // 256 KB
constexpr size_t IWRAM_SIZE          = 0x08000;     // 32 KB
constexpr size_t IOREG_SIZE          = 0x00400;     // 1 KB
constexpr size_t PLTT_SIZE           = 0x00400;     // 1 KB (512 bytes BG + 512 bytes OBJ)
constexpr size_t VRAM_SIZE           = 0x18000;     // 96 KB
constexpr size_t OAM_SIZE            = 0x00400;     // 1 KB (128 entries × 8 bytes)
constexpr size_t MAX_ROM_SIZE        = 0x2000000;   // 32 MB
constexpr size_t SRAM_SIZE           = 0x20000;     // 128 KB

// =============================================================================
// MODERN MEMORY LAYOUT (Runtime allocated)
// =============================================================================
// All GBA memory regions are allocated as contiguous blocks in host memory.
// We use a single MemoryContext structure to hold all regions, ensuring
// cache-friendly access patterns and simplified pointer redirection.

struct MemoryRegion {
    uint8_t*    base;           // Host virtual address
    size_t      size;           // Region size in bytes
    uint32_t    gbaBaseAddr;    // Original GBA base address (for debugging)
    const char* name;           // Human-readable name
};

// =============================================================================
// HARDWARE REGISTER OFFSETS (within IOREG region)
// =============================================================================
namespace IO {
    // Display Control
    constexpr uint32_t DISPCNT      = 0x000;  // Display control
    constexpr uint32_t DISPSTAT     = 0x004;  // Display status
    constexpr uint32_t VCOUNT       = 0x006;  // Vertical line counter
    
    // Background Control
    constexpr uint32_t BG0CNT       = 0x008;
    constexpr uint32_t BG1CNT       = 0x00A;
    constexpr uint32_t BG2CNT       = 0x00C;
    constexpr uint32_t BG3CNT       = 0x00E;
    
    // Background Offsets
    constexpr uint32_t BG0HOFS      = 0x010;
    constexpr uint32_t BG0VOFS      = 0x012;
    constexpr uint32_t BG1HOFS      = 0x014;
    constexpr uint32_t BG1VOFS      = 0x016;
    constexpr uint32_t BG2HOFS      = 0x018;
    constexpr uint32_t BG2VOFS      = 0x01A;
    constexpr uint32_t BG3HOFS      = 0x01C;
    constexpr uint32_t BG3VOFS      = 0x01E;
    
    // Affine Background Parameters (BG2)
    constexpr uint32_t BG2PA        = 0x020;
    constexpr uint32_t BG2PB        = 0x022;
    constexpr uint32_t BG2PC        = 0x024;
    constexpr uint32_t BG2PD        = 0x026;
    constexpr uint32_t BG2X         = 0x028;
    constexpr uint32_t BG2Y         = 0x02C;
    
    // Affine Background Parameters (BG3)
    constexpr uint32_t BG3PA        = 0x030;
    constexpr uint32_t BG3PB        = 0x032;
    constexpr uint32_t BG3PC        = 0x034;
    constexpr uint32_t BG3PD        = 0x036;
    constexpr uint32_t BG3X         = 0x038;
    constexpr uint32_t BG3Y         = 0x03C;
    
    // Window Registers
    constexpr uint32_t WIN0H        = 0x040;
    constexpr uint32_t WIN1H        = 0x042;
    constexpr uint32_t WIN0V        = 0x044;
    constexpr uint32_t WIN1V        = 0x046;
    constexpr uint32_t WININ        = 0x048;
    constexpr uint32_t WINOUT       = 0x04A;
    
    // Mosaic
    constexpr uint32_t MOSAIC       = 0x04C;
    
    // Blending
    constexpr uint32_t BLDCNT       = 0x050;
    constexpr uint32_t BLDALPHA     = 0x052;
    constexpr uint32_t BLDY         = 0x054;
    
    // Interrupt Control
    constexpr uint32_t IE           = 0x200;
    constexpr uint32_t IF           = 0x202;
    constexpr uint32_t IME          = 0x208;
    
    // Input
    constexpr uint32_t KEYINPUT     = 0x130;
    constexpr uint32_t KEYCNT       = 0x132;
    
    // DMA Control
    constexpr uint32_t DMA0SAD      = 0x0B0;
    constexpr uint32_t DMA0DAD      = 0x0B4;
    constexpr uint32_t DMA0CNT      = 0x0B8;
    constexpr uint32_t DMA1SAD      = 0x0BC;
    constexpr uint32_t DMA1DAD      = 0x0C0;
    constexpr uint32_t DMA1CNT      = 0x0C4;
    constexpr uint32_t DMA2SAD      = 0x0C8;
    constexpr uint32_t DMA2DAD      = 0x0CC;
    constexpr uint32_t DMA2CNT      = 0x0D0;
    constexpr uint32_t DMA3SAD      = 0x0D4;
    constexpr uint32_t DMA3DAD      = 0x0D8;
    constexpr uint32_t DMA3CNT      = 0x0DC;
    
    // Timer Control
    constexpr uint32_t TM0CNT       = 0x100;
    constexpr uint32_t TM1CNT       = 0x104;
    constexpr uint32_t TM2CNT       = 0x108;
    constexpr uint32_t TM3CNT       = 0x10C;
    
    // Serial Communication
    constexpr uint32_t SIODATA      = 0x120;
    constexpr uint32_t SIOMULTI0    = 0x120;
    constexpr uint32_t SIOMULTI1    = 0x122;
    constexpr uint32_t SIOMULTI2    = 0x124;
    constexpr uint32_t SIOMULTI3    = 0x126;
    constexpr uint32_t SIOCNT       = 0x128;
    constexpr uint32_t SIOMLT_SEND  = 0x12A;
    constexpr uint32_t SIODATA8     = 0x12A;
}

// =============================================================================
// DISPLAY CONTROL FLAGS
// =============================================================================
namespace DISPCNT {
    constexpr uint16_t MODE_MASK    = 0x0007;
    constexpr uint16_t MODE_0       = 0x0000;  // 4 tiled BGs
    constexpr uint16_t MODE_1       = 0x0001;  // 3 BGs (2 affine)
    constexpr uint16_t MODE_2       = 0x0002;  // 2 affine BGs
    constexpr uint16_t MODE_3       = 0x0003;  // Bitmap 240×160 (16-bit)
    constexpr uint16_t MODE_4       = 0x0004;  // Bitmap 240×160 (8-bit palette)
    constexpr uint16_t MODE_5       = 0x0005;  // Bitmap 160×128 (16-bit)
    constexpr uint16_t FRAME_SELECT = 0x0010;  // Frame select for modes 4/5
    constexpr uint16_t HBLANK_OAM   = 0x0020;  // OAM access during HBlank
    constexpr uint16_t OBJ_1D_MAP   = 0x0040;  // 1D sprite mapping
    constexpr uint16_t OBJ_2D_MAP   = 0x0000;  // 2D sprite mapping
    constexpr uint16_t FORCED_BLANK = 0x0080;  // Force blank (white screen)
    constexpr uint16_t BG0_ON       = 0x0100;  // Enable BG0
    constexpr uint16_t BG1_ON       = 0x0200;  // Enable BG1
    constexpr uint16_t BG2_ON       = 0x0400;  // Enable BG2
    constexpr uint16_t BG3_ON       = 0x0800;  // Enable BG3
    constexpr uint16_t OBJ_ON       = 0x1000;  // Enable sprites
    constexpr uint16_t WIN0_ON      = 0x2000;  // Enable Window 0
    constexpr uint16_t WIN1_ON      = 0x4000;  // Enable Window 1
    constexpr uint16_t OBJWIN_ON    = 0x8000;  // Enable Object Window
}

// =============================================================================
// INTERRUPT FLAGS
// =============================================================================
namespace INTR {
    constexpr uint16_t VBLANK       = 0x0001;
    constexpr uint16_t HBLANK       = 0x0002;
    constexpr uint16_t VCOUNT       = 0x0004;
    constexpr uint16_t TIMER0       = 0x0008;
    constexpr uint16_t TIMER1       = 0x0010;
    constexpr uint16_t TIMER2       = 0x0020;
    constexpr uint16_t TIMER3       = 0x0040;
    constexpr uint16_t SERIAL       = 0x0080;
    constexpr uint16_t DMA0         = 0x0100;
    constexpr uint16_t DMA1         = 0x0200;
    constexpr uint16_t DMA2         = 0x0400;
    constexpr uint16_t DMA3         = 0x0800;
    constexpr uint16_t KEYPAD       = 0x1000;
    constexpr uint16_t GAMEPAK      = 0x2000;
}

// =============================================================================
// INPUT FLAGS (inverted logic - 0 = pressed)
// =============================================================================
namespace INPUT {
    constexpr uint16_t A            = 0x0001;
    constexpr uint16_t B            = 0x0002;
    constexpr uint16_t SELECT       = 0x0004;
    constexpr uint16_t START        = 0x0008;
    constexpr uint16_t RIGHT        = 0x0010;
    constexpr uint16_t LEFT         = 0x0020;
    constexpr uint16_t UP           = 0x0040;
    constexpr uint16_t DOWN         = 0x0080;
    constexpr uint16_t R            = 0x0100;
    constexpr uint16_t L            = 0x0200;
    constexpr uint16_t ALL_KEYS     = 0x03FF;
}

// =============================================================================
// SCREEN DIMENSIONS
// =============================================================================
constexpr uint32_t SCREEN_WIDTH    = 240;
constexpr uint32_t SCREEN_HEIGHT   = 160;
constexpr uint32_t SCREEN_TILE_W   = 30;   // 240 / 8
constexpr uint32_t SCREEN_TILE_H   = 20;   // 160 / 8

// =============================================================================
// POINTER REDIRECTION MACROS
// =============================================================================
// These macros handle the conversion between GBA memory addresses and
// modern host pointers. All accesses go through the MemoryContext.

#define GBA_PTR(context, gba_addr) \
    (context->translateAddress((gba_addr)))

#define GBA_PTR_T(context, gba_addr, type) \
    (reinterpret_cast<type*>(context->translateAddress((gba_addr))))

#define GBA_REG16(context, offset) \
    (*reinterpret_cast<uint16_t*>(context->ioReg.base + (offset)))

#define GBA_REG32(context, offset) \
    (*reinterpret_cast<uint32_t*>(context->ioReg.base + (offset)))

} // namespace GBA

#endif // GBA_MEMORY_MAP_H

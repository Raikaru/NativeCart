/**
 * @file gba_hardware.h
 * @brief GBA hardware register access and timing
 * 
 * Provides hardware register access and frame timing for the GBA emulation.
 * This is a minimum viable implementation for booting to the Nintendo screen.
 */

#ifndef GBA_HARDWARE_H
#define GBA_HARDWARE_H

// 1. NOMINMAX guard
#define NOMINMAX

// 2. Standard C++ headers
#include <cstdint>
#include <cstddef>

// =============================================================================
// FRAME TIMING CONSTANTS
// =============================================================================
constexpr uint32_t GBA_SCANLINES_PER_FRAME = 228;
constexpr uint32_t GBA_VISIBLE_SCANLINES = 160;
constexpr uint32_t GBA_VBLANK_SCANLINES = 68;
constexpr uint32_t GBA_SCREEN_WIDTH = 240;
constexpr uint32_t GBA_SCREEN_HEIGHT = 160;

// =============================================================================
// HARDWARE REGISTER ADDRESSES
// =============================================================================
constexpr uint32_t REG_DISPCNT_ADDR = 0x04000000;   // Display control
constexpr uint32_t REG_DISPSTAT_ADDR = 0x04000004;  // Display status
constexpr uint32_t REG_VCOUNT_ADDR = 0x04000006;    // Vertical line counter
constexpr uint32_t REG_BG0CNT_ADDR = 0x04000008;    // BG0 control
constexpr uint32_t REG_BG1CNT_ADDR = 0x0400000A;    // BG1 control
constexpr uint32_t REG_BG2CNT_ADDR = 0x0400000C;    // BG2 control
constexpr uint32_t REG_BG3CNT_ADDR = 0x0400000E;    // BG3 control

// DMA Registers
constexpr uint32_t REG_DMA0SAD_ADDR = 0x040000B0;   // DMA 0 source address
constexpr uint32_t REG_DMA0DAD_ADDR = 0x040000B4;   // DMA 0 destination address
constexpr uint32_t REG_DMA0CNT_ADDR = 0x040000B8;   // DMA 0 control
constexpr uint32_t REG_DMA1SAD_ADDR = 0x040000BC;   // DMA 1 source address
constexpr uint32_t REG_DMA1DAD_ADDR = 0x040000C0;   // DMA 1 destination address
constexpr uint32_t REG_DMA1CNT_ADDR = 0x040000C4;   // DMA 1 control
constexpr uint32_t REG_DMA2SAD_ADDR = 0x040000C8;   // DMA 2 source address
constexpr uint32_t REG_DMA2DAD_ADDR = 0x040000CC;   // DMA 2 destination address
constexpr uint32_t REG_DMA2CNT_ADDR = 0x040000D0;   // DMA 2 control
constexpr uint32_t REG_DMA3SAD_ADDR = 0x040000D4;   // DMA 3 source address
constexpr uint32_t REG_DMA3DAD_ADDR = 0x040000D8;   // DMA 3 destination address
constexpr uint32_t REG_DMA3CNT_ADDR = 0x040000DC;   // DMA 3 control

// Interrupt Registers
constexpr uint32_t REG_IE_ADDR = 0x04000200;        // Interrupt enable
constexpr uint32_t REG_IF_ADDR = 0x04000202;        // Interrupt request flags
constexpr uint32_t REG_IME_ADDR = 0x04000208;       // Interrupt master enable

// =============================================================================
// DISPLAY CONTROL FLAGS
// =============================================================================
namespace DISPCNT {
    constexpr uint16_t MODE_MASK    = 0x0007;
    constexpr uint16_t MODE_0       = 0x0000;  // 4 tiled BGs
    constexpr uint16_t MODE_1       = 0x0001;  // 3 BGs (2 affine)
    constexpr uint16_t MODE_2       = 0x0002;  // 2 affine BGs
    constexpr uint16_t MODE_3       = 0x0003;  // Bitmap 240x160 (16-bit)
    constexpr uint16_t MODE_4       = 0x0004;  // Bitmap 240x160 (8-bit palette)
    constexpr uint16_t MODE_5       = 0x0005;  // Bitmap 160x128 (16-bit)
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
// DISPLAY STATUS FLAGS
// =============================================================================
namespace DISPSTAT {
    constexpr uint16_t VBLANK_FLAG  = 0x0001;  // In VBlank
    constexpr uint16_t HBLANK_FLAG  = 0x0002;  // In HBlank
    constexpr uint16_t VCOUNT_FLAG  = 0x0004;  // VCounter match
    constexpr uint16_t VBLANK_IRQ   = 0x0008;  // VBlank interrupt enable
    constexpr uint16_t HBLANK_IRQ   = 0x0010;  // HBlank interrupt enable
    constexpr uint16_t VCOUNT_IRQ   = 0x0020;  // VCounter interrupt enable
    constexpr uint16_t VCOUNT_MASK  = 0xFF00;  // VCounter setting (bits 8-15)
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
// INITIALIZATION AND SHUTDOWN
// =============================================================================

/**
 * @brief Initialize the GBA hardware emulation
 * 
 * Sets up hardware registers to default values and allocates framebuffer.
 * Must be called before any other hardware functions.
 */
void gba_hardware_init();

/**
 * @brief Shutdown the GBA hardware emulation
 * 
 * Frees allocated resources.
 */
void gba_hardware_shutdown();

// =============================================================================
// FRAME TIMING AND EXECUTION
// =============================================================================

/**
 * @brief Execute one full frame (VBlank period)
 * 
 * Simulates the GBA's display timing:
 * - Lines 0-159: Visible screen (160 lines)
 * - Lines 160-227: VBlank period (68 lines)
 * 
 * This function:
 * 1. Increments through scanlines 0-227
 * 2. Updates REG_VCOUNT accordingly
 * 3. Triggers VBlank interrupt at line 160 if enabled
 * 4. Copies rendered framebuffer to output buffer
 */
void gba_run_frame();

/**
 * @brief Get the current scanline
 * @return Current VCOUNT value (0-227)
 */
uint16_t gba_get_vcount();

/**
 * @brief Check if currently in VBlank period
 * @return true if in VBlank (lines 160-227), false otherwise
 */
bool gba_is_vblank();

/**
 * @brief Trigger VBlank interrupt if enabled
 * 
 * Sets the VBlank flag in DISPSTAT and triggers interrupt
 * if VBlank IRQ is enabled and IME/IE allow interrupts.
 */
void gba_trigger_vblank();

// =============================================================================
// FRAMEBUFFER ACCESS
// =============================================================================

/**
 * @brief Get pointer to the output framebuffer
 * 
 * Returns a pointer to the 240x160 16bpp RGB framebuffer.
 * The framebuffer format is RGB565 (5 bits red, 6 bits green, 5 bits blue).
 * 
 * @return Pointer to 16bpp framebuffer (size: 240 * 160 * 2 = 76800 bytes)
 */
uint16_t* gba_get_framebuffer();

/**
 * @brief Get pointer to VRAM for rendering
 * @return Pointer to GBA VRAM
 */
uint8_t* gba_get_vram_ptr();

/**
 * @brief Get pointer to palette RAM
 * @return Pointer to GBA palette RAM
 */
uint8_t* gba_get_palette_ptr();

// =============================================================================
// HARDWARE REGISTER ACCESS
// =============================================================================

/**
 * @brief Read 16-bit hardware register
 * @param addr Register address (0x04000000-0x040003FE)
 * @return Current register value
 */
uint16_t gba_hw_read16(uint32_t addr);

/**
 * @brief Write 16-bit hardware register
 * @param addr Register address (0x04000000-0x040003FE)
 * @param value Value to write
 */
void gba_hw_write16(uint32_t addr, uint16_t value);

/**
 * @brief Read 32-bit hardware register
 * @param addr Register address (0x04000000-0x040003FE)
 * @return Current register value
 */
uint32_t gba_hw_read32(uint32_t addr);

/**
 * @brief Write 32-bit hardware register
 * @param addr Register address (0x04000000-0x040003FE)
 * @param value Value to write
 */
void gba_hw_write32(uint32_t addr, uint32_t value);

#endif // GBA_HARDWARE_H

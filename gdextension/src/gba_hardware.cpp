/**
 * @file gba_hardware.cpp
 * @brief GBA hardware register access and timing implementation
 * 
 * Implements hardware register access and frame timing for the GBA emulation.
 * Minimum viable implementation for booting to the Nintendo screen.
 */

// 1. NOMINMAX guard
#define NOMINMAX

// 2. Standard C++ headers
#include "gba_hardware.h"
#include <cstring>
#include <cstdlib>

// =============================================================================
// INTERNAL HARDWARE STATE
// =============================================================================

namespace {
    // Hardware registers (I/O register space)
    struct HardwareRegisters {
        // Display registers
        uint16_t dispCnt;       // REG_DISPCNT (0x04000000)
        uint16_t _pad0;         // Padding
        uint16_t dispStat;      // REG_DISPSTAT (0x04000004)
        uint16_t vCount;        // REG_VCOUNT (0x04000006)
        
        // Background control
        uint16_t bg0Cnt;        // REG_BG0CNT (0x04000008)
        uint16_t bg1Cnt;        // REG_BG1CNT (0x0400000A)
        uint16_t bg2Cnt;        // REG_BG2CNT (0x0400000C)
        uint16_t bg3Cnt;        // REG_BG3CNT (0x0400000E)
        
        // More registers can be added as needed
    };
    
    // DMA channel state
    struct DMAChannel {
        uint32_t sourceAddr;
        uint32_t destAddr;
        uint32_t count;
        uint16_t control;
    };
    
    // Global hardware state
    struct HardwareState {
        HardwareRegisters regs;
        DMAChannel dma[4];
        
        // Interrupt registers
        uint16_t ie;            // Interrupt enable
        uint16_t irqFlags;      // Interrupt request flags
        uint16_t ime;           // Interrupt master enable
        
        // Frame timing
        uint32_t currentScanline;
        bool inVBlank;
        
        // Framebuffer (240x160, 16bpp RGB565)
        uint16_t framebuffer[GBA_SCREEN_WIDTH * GBA_SCREEN_HEIGHT];
        
        // VRAM and palette pointers (external)
        uint8_t* vram;
        uint8_t* palette;
        
        bool initialized;
    };
    
    HardwareState* g_hardware = nullptr;
}

// =============================================================================
// INITIALIZATION AND SHUTDOWN
// =============================================================================

void gba_hardware_init() {
    if (g_hardware != nullptr) {
        // Already initialized
        return;
    }
    
    g_hardware = new HardwareState();
    
    // Initialize registers to default values
    g_hardware->regs.dispCnt = DISPCNT::FORCED_BLANK;  // Start in forced blank
    g_hardware->regs.dispStat = 0;
    g_hardware->regs.vCount = 0;
    g_hardware->regs.bg0Cnt = 0;
    g_hardware->regs.bg1Cnt = 0;
    g_hardware->regs.bg2Cnt = 0;
    g_hardware->regs.bg3Cnt = 0;
    
    // Initialize DMA channels
    for (int i = 0; i < 4; i++) {
        g_hardware->dma[i].sourceAddr = 0;
        g_hardware->dma[i].destAddr = 0;
        g_hardware->dma[i].count = 0;
        g_hardware->dma[i].control = 0;
    }
    
    // Initialize interrupt registers
    g_hardware->ie = 0;
    g_hardware->irqFlags = 0;
    g_hardware->ime = 0;
    
    // Initialize frame timing
    g_hardware->currentScanline = 0;
    g_hardware->inVBlank = false;
    
    // Clear framebuffer (black)
    memset(g_hardware->framebuffer, 0, sizeof(g_hardware->framebuffer));
    
    // External pointers (will be set by caller)
    g_hardware->vram = nullptr;
    g_hardware->palette = nullptr;
    
    g_hardware->initialized = true;
}

void gba_hardware_shutdown() {
    if (g_hardware == nullptr) {
        return;
    }
    
    delete g_hardware;
    g_hardware = nullptr;
}

// =============================================================================
// FRAME TIMING AND EXECUTION
// =============================================================================

void gba_run_frame() {
    if (g_hardware == nullptr || !g_hardware->initialized) {
        return;
    }
    
    // Process all 228 scanlines per frame
    for (uint32_t line = 0; line < GBA_SCANLINES_PER_FRAME; line++) {
        g_hardware->currentScanline = line;
        g_hardware->regs.vCount = static_cast<uint16_t>(line);
        
        // Check for VBlank start (line 160)
        if (line == GBA_VISIBLE_SCANLINES) {
            g_hardware->inVBlank = true;
            g_hardware->regs.dispStat |= DISPSTAT::VBLANK_FLAG;
            
            // Trigger VBlank interrupt if enabled
            gba_trigger_vblank();
        }
        
        // Check for VBlank end (line 228, wrapped to 0 next frame)
        if (line == GBA_SCANLINES_PER_FRAME - 1) {
            // Will be cleared at start of next frame
        }
        
        // Simulate HBlank for each scanline (simplified)
        // In real hardware, HBlank occurs at the end of each scanline
        // For now, we just do minimal processing
    }
    
    // End of frame - clear VBlank flag for next frame
    g_hardware->inVBlank = false;
    g_hardware->regs.dispStat &= ~DISPSTAT::VBLANK_FLAG;
    g_hardware->currentScanline = 0;
    g_hardware->regs.vCount = 0;
    
    // Render frame to framebuffer if display is enabled
    if ((g_hardware->regs.dispCnt & DISPCNT::FORCED_BLANK) == 0) {
        // TODO: Actual rendering based on display mode
        // For now, just fill with a test pattern or black
        // This is a stub - full PPU implementation would go here
        
        // Simple test: fill with black (0x0000)
        // In a full implementation, this would render BGs, sprites, etc.
        memset(g_hardware->framebuffer, 0, sizeof(g_hardware->framebuffer));
    } else {
        // Forced blank - white screen
        memset(g_hardware->framebuffer, 0xFF, sizeof(g_hardware->framebuffer));
    }
}

uint16_t gba_get_vcount() {
    if (g_hardware == nullptr) {
        return 0;
    }
    return g_hardware->regs.vCount;
}

bool gba_is_vblank() {
    if (g_hardware == nullptr) {
        return false;
    }
    return g_hardware->inVBlank;
}

void gba_trigger_vblank() {
    if (g_hardware == nullptr) {
        return;
    }
    
    // Set VBlank flag in DISPSTAT (already done in run_frame)
    // g_hardware->regs.dispStat |= DISPSTAT::VBLANK_FLAG;
    
    // Check if VBlank interrupt is enabled
    bool vblankIrqEnabled = (g_hardware->regs.dispStat & DISPSTAT::VBLANK_IRQ) != 0;
    bool vblankIntEnabled = (g_hardware->ie & INTR::VBLANK) != 0;
    bool masterEnable = (g_hardware->ime & 0x0001) != 0;
    
    if (vblankIrqEnabled && vblankIntEnabled && masterEnable) {
        // Set VBlank interrupt flag
        g_hardware->irqFlags |= INTR::VBLANK;
    }
}

// =============================================================================
// FRAMEBUFFER ACCESS
// =============================================================================

uint16_t* gba_get_framebuffer() {
    if (g_hardware == nullptr) {
        return nullptr;
    }
    return g_hardware->framebuffer;
}

uint8_t* gba_get_vram_ptr() {
    if (g_hardware == nullptr) {
        return nullptr;
    }
    return g_hardware->vram;
}

uint8_t* gba_get_palette_ptr() {
    if (g_hardware == nullptr) {
        return nullptr;
    }
    return g_hardware->palette;
}

// =============================================================================
// HARDWARE REGISTER ACCESS
// =============================================================================

uint16_t gba_hw_read16(uint32_t addr) {
    if (g_hardware == nullptr) {
        return 0;
    }
    
    // Mask to I/O register region
    uint32_t offset = (addr & 0x00FFFFFF) - 0x04000000;
    
    switch (offset) {
        case 0x000:  // REG_DISPCNT
            return g_hardware->regs.dispCnt;
        case 0x004:  // REG_DISPSTAT
            return g_hardware->regs.dispStat;
        case 0x006:  // REG_VCOUNT
            return g_hardware->regs.vCount;
        case 0x008:  // REG_BG0CNT
            return g_hardware->regs.bg0Cnt;
        case 0x00A:  // REG_BG1CNT
            return g_hardware->regs.bg1Cnt;
        case 0x00C:  // REG_BG2CNT
            return g_hardware->regs.bg2Cnt;
        case 0x00E:  // REG_BG3CNT
            return g_hardware->regs.bg3Cnt;
        case 0x200:  // REG_IE
            return g_hardware->ie;
        case 0x202:  // REG_IF
            return g_hardware->irqFlags;
        case 0x208:  // REG_IME
            return g_hardware->ime;
        default:
            // Unknown register - return 0
            return 0;
    }
}

void gba_hw_write16(uint32_t addr, uint16_t value) {
    if (g_hardware == nullptr) {
        return;
    }
    
    // Mask to I/O register region
    uint32_t offset = (addr & 0x00FFFFFF) - 0x04000000;
    
    switch (offset) {
        case 0x000:  // REG_DISPCNT
            g_hardware->regs.dispCnt = value;
            break;
        case 0x004:  // REG_DISPSTAT
            // Preserve read-only bits (VBlank, HBlank, VCounter flags)
            g_hardware->regs.dispStat = (value & 0xFFB8) | (g_hardware->regs.dispStat & 0x0007);
            break;
        case 0x006:  // REG_VCOUNT (read-only)
            // Cannot write to VCOUNT
            break;
        case 0x008:  // REG_BG0CNT
            g_hardware->regs.bg0Cnt = value;
            break;
        case 0x00A:  // REG_BG1CNT
            g_hardware->regs.bg1Cnt = value;
            break;
        case 0x00C:  // REG_BG2CNT
            g_hardware->regs.bg2Cnt = value;
            break;
        case 0x00E:  // REG_BG3CNT
            g_hardware->regs.bg3Cnt = value;
            break;
        case 0x200:  // REG_IE
            g_hardware->ie = value;
            break;
        case 0x202:  // REG_IF (write 1 to clear)
            g_hardware->irqFlags &= ~value;
            break;
        case 0x208:  // REG_IME
            g_hardware->ime = value & 0x0001;
            break;
        
        // DMA Registers (stubs - don't crash)
        case 0x0B8:  // REG_DMA0CNT_L (count)
            g_hardware->dma[0].count = (g_hardware->dma[0].count & 0xFFFF0000) | value;
            break;
        case 0x0BA:  // REG_DMA0CNT_H (control)
            g_hardware->dma[0].control = value;
            // TODO: Trigger DMA if enabled
            break;
        case 0x0C4:  // REG_DMA1CNT_L
            g_hardware->dma[1].count = (g_hardware->dma[1].count & 0xFFFF0000) | value;
            break;
        case 0x0C6:  // REG_DMA1CNT_H
            g_hardware->dma[1].control = value;
            break;
        case 0x0D0:  // REG_DMA2CNT_L
            g_hardware->dma[2].count = (g_hardware->dma[2].count & 0xFFFF0000) | value;
            break;
        case 0x0D2:  // REG_DMA2CNT_H
            g_hardware->dma[2].control = value;
            break;
        case 0x0DC:  // REG_DMA3CNT_L
            g_hardware->dma[3].count = (g_hardware->dma[3].count & 0xFFFF0000) | value;
            break;
        case 0x0DE:  // REG_DMA3CNT_H
            g_hardware->dma[3].control = value;
            break;
            
        default:
            // Unknown register - ignore write
            break;
    }
}

uint32_t gba_hw_read32(uint32_t addr) {
    // Read two 16-bit values and combine
    uint16_t low = gba_hw_read16(addr);
    uint16_t high = gba_hw_read16(addr + 2);
    return low | (high << 16);
}

void gba_hw_write32(uint32_t addr, uint32_t value) {
    // Write as two 16-bit values
    gba_hw_write16(addr, value & 0xFFFF);
    gba_hw_write16(addr + 2, (value >> 16) & 0xFFFF);
    
    // Handle DMA address writes specially
    if (g_hardware == nullptr) {
        return;
    }
    
    uint32_t offset = (addr & 0x00FFFFFF) - 0x04000000;
    
    switch (offset) {
        case 0x0B0:  // REG_DMA0SAD
            g_hardware->dma[0].sourceAddr = value;
            break;
        case 0x0B4:  // REG_DMA0DAD
            g_hardware->dma[0].destAddr = value;
            break;
        case 0x0BC:  // REG_DMA1SAD
            g_hardware->dma[1].sourceAddr = value;
            break;
        case 0x0C0:  // REG_DMA1DAD
            g_hardware->dma[1].destAddr = value;
            break;
        case 0x0C8:  // REG_DMA2SAD
            g_hardware->dma[2].sourceAddr = value;
            break;
        case 0x0CC:  // REG_DMA2DAD
            g_hardware->dma[2].destAddr = value;
            break;
        case 0x0D4:  // REG_DMA3SAD
            g_hardware->dma[3].sourceAddr = value;
            break;
        case 0x0D8:  // REG_DMA3DAD
            g_hardware->dma[3].destAddr = value;
            break;
        default:
            break;
    }
}

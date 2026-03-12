# =============================================================================
# PokeFireRed GDExtension - Architecture Resolution Document
# =============================================================================
# This document explains how the 16-bit ARM GBA architecture is bridged to
# modern 64-bit Godot execution environments.
# =============================================================================

## 1. Architectural Challenge Overview

### GBA (16-bit ARM7TDMI)
- Architecture: ARMv4T (16-bit/32-bit hybrid instruction set)
- Clock: 16.78 MHz
- Memory: 24-bit address bus (16MB directly addressable)
- Registers: 16× 32-bit registers (including PC)
- Endianness: Little-endian
- Screen: 240×160 @ 60Hz

### Modern x86_64/ARM64
- Architecture: 64-bit instruction set
- Clock: 2.0+ GHz (100× faster)
- Memory: 64-bit address bus (theoretical 16EB)
- Registers: 16× 64-bit general purpose + many SIMD
- Endianness: Little-endian (x86) / Bi-endian (ARM)
- Screen: Variable resolution

## 2. Core Resolution Strategies

### 2.1 Memory Address Translation

**Problem**: GBA uses 24-bit addresses (0x00000000-0x0FFFFFFF), modern systems use 64-bit.

**Solution**: Virtual memory mapping with translation layer

```
GBA Address Space          Host Virtual Memory
─────────────────         ─────────────────────
0x02000000 EWRAM    →     Heap-allocated buffer (256KB)
0x03000000 IWRAM    →     Heap-allocated buffer (32KB)
0x04000000 I/O Regs →     Heap-allocated buffer (1KB)
0x05000000 Palette  →     Heap-allocated buffer (1KB)
0x06000000 VRAM     →     Heap-allocated buffer (96KB)
0x07000000 OAM      →     Heap-allocated buffer (1KB)
0x08000000 ROM      →     File-loaded buffer (up to 32MB)
0x0E000000 SRAM     →     Heap-allocated buffer (128KB)
```

**Implementation** (`gba_memory_map.hpp`):
```cpp
struct MemoryContext {
    MemoryRegion ewram, iwram, ioreg, palette, vram, oam, rom, sram;
    
    uint8_t* translateAddress(uint32_t gba_addr) {
        gba_addr &= 0x00FFFFFF;  // Mask to 24-bit
        
        if (gba_addr >= 0x02000000 && gba_addr < 0x02400000)
            return ewram.base + (gba_addr - 0x02000000);
        // ... etc
    }
};
```

### 2.2 Pointer Size Mismatch

**Problem**: GBA uses 32-bit pointers (4 bytes), modern systems use 64-bit (8 bytes).

**Impact**: The original C code contains many pointer casts and arithmetic.

**Solution**: 
1. Keep original C code using 32-bit pointer storage (u32 addresses)
2. Provide translation functions for actual access
3. Use wrapper macros for pointer conversion

**Critical Structures** (from pokefirered analysis):

```c
// Original GBA structure - uses u32 for pointers
truct Sprite {
    struct OamData oam;
    const union AnimCmd *const *anims;  // 4 bytes on GBA
    const struct SpriteFrameImage *images;  // 4 bytes on GBA
    // ...
};

// Our solution: Store as offset from base, translate on access
#define GBA_PTR_TO_HOST(gba_ptr) \
    (g_memoryContext.translateAddress((uint32_t)(gba_ptr)))
```

### 2.3 Data Alignment

**Problem**: ARM allows unaligned access in some modes; x86_64 requires strict alignment for some operations.

**Solution**:
- Use `memcpy` for potentially unaligned accesses
- Mark buffers as aligned where possible
- Compiler flags to handle ARM-like behavior

```cpp
// Safe unaligned read
uint32_t read_unaligned(uint8_t* ptr) {
    uint32_t value;
    memcpy(&value, ptr, sizeof(value));
    return value;
}
```

### 2.4 I/O Register Emulation

**Problem**: GBA has memory-mapped I/O registers (0x04000000-0x04000400) that control hardware.

**Solution**: Emulate registers in host memory with side-effect handlers

```cpp
// I/O Register offsets (from gba_memory_map.hpp)
namespace IO {
    constexpr uint32_t DISPCNT  = 0x000;  // Display control
    constexpr uint32_t DISPSTAT = 0x004;  // Display status
    constexpr uint32_t VCOUNT   = 0x006;  // Vertical line counter
    constexpr uint32_t BG0CNT   = 0x008;  // BG0 control
    // ... etc
}

// Register access with side effects
void write_reg16(uint32_t offset, uint16_t value) {
    uint16_t* reg = (uint16_t*)(ioreg.base + offset);
    *reg = value;
    
    // Handle side effects
    switch(offset) {
        case IO::DISPCNT:
            // Update rendering state
            renderer->set_mode(value & 0x7);
            break;
        case IO::DMA0CNT:
            // Trigger DMA transfer
            process_dma();
            break;
        // ... etc
    }
}
```

### 2.5 Interrupt Handling

**Problem**: GBA uses hardware interrupts (VBlank, HBlank, timers), modern OS doesn't allow direct hardware access.

**Solution**: Software interrupt emulation with timing

```cpp
// Interrupt flags (gba_memory_map.hpp)
namespace INTR {
    constexpr uint16_t VBLANK = 0x0001;
    constexpr uint16_t HBLANK = 0x0002;
    constexpr uint16_t VCOUNT = 0x0004;
    // ... etc
}

// Emulation approach:
// 1. Run game logic for one frame
// 2. Trigger VBlank interrupt
// 3. Call registered handlers
// 4. Continue

void emulate_vblank() {
    uint16_t ie = GBA_REG16(context, IO::IE);
    uint16_t ime = GBA_REG16(context, IO::IME);
    
    if ((ime & 1) && (ie & INTR::VBLANK)) {
        // Set interrupt flag
        GBA_REG16(context, IO::IF) |= INTR::VBLANK;
        
        // Call VBlank handler (if implemented in C code)
        if (gMain.vblankCallback) {
            gMain.vblankCallback();
        }
    }
}
```

### 2.6 Timing and Synchronization

**Problem**: GBA runs at exactly 16.78 MHz with precise timing. Modern systems vary.

**Solution**: 
- Frame-based synchronization (target 60 FPS)
- Skip frames if running too slow
- Use delta time for variable frame rates

```cpp
// Godot _process callback handles timing
void GBARenderer::_process(double delta) {
    // Signal VBlank to C code
    signal_vblank();
    
    // Render frame if needed
    if (vblank_pending) {
        render_frame();
        update_godot_texture();
        vblank_pending = false;
    }
}
```

## 3. Memory Layout Comparison

### GBA Original Layout
```
Address     Region          Size        Access
─────────────────────────────────────────────────
00000000    BIOS ROM        16KB        Read-only
02000000    EWRAM          256KB        R/W (slower)
03000000    IWRAM           32KB        R/W (fast)
04000000    I/O Registers    1KB        Special
05000000    Palette RAM      1KB        R/W (16-bit)
06000000    VRAM            96KB        R/W (16-bit)
07000000    OAM              1KB        R/W
08000000    Game ROM        32MB        Read-only
0E000000    SRAM           128KB        R/W (battery)
```

### Host Implementation
```cpp
// Single allocation block for cache efficiency
struct MemoryContext {
    uint8_t* base_address;  // Aligned to page boundary
    
    // Offsets within block
    static constexpr size_t EWRAM_OFFSET = 0;
    static constexpr size_t IWRAM_OFFSET = EWRAM_OFFSET + EWRAM_SIZE;
    static constexpr size_t IOREG_OFFSET = IWRAM_OFFSET + IWRAM_SIZE;
    // ... etc
};

// Total: ~33MB per instance (reasonable for modern systems)
```

## 4. Graphics Pipeline Resolution

### GBA Rendering Pipeline
1. CPU builds OAM and tile data during active display
2. VBlank triggers:
   - DMA transfers OAM to hardware
   - Palette updates applied
   - REG_DISPCNT changes take effect
3. Hardware renders scanlines independently

### Godot Bridge Pipeline
1. C code runs game logic, modifies VRAM/OAM
2. VBlank signal captured by GBARenderer
3. Software renderer converts to RGBA8:
   - Render backgrounds (tiles + maps)
   - Render sprites (OAM processing)
   - Apply palette
4. Result uploaded to Godot ImageTexture
5. Godot displays in scene

```
GBA Hardware              GDExtension Bridge
────────────────          ────────────────────
VRAM (tiles/maps)   →     Software renderer
OAM (sprite attrs)  →     Sprite processor
Palette (colors)    →     RGBA8 converter
Scanline hardware   →     Full frame buffer
                    ↓
              Godot ImageTexture
                    ↓
              TextureRect/Sprite
```

## 5. Pointer Redirection for Patches

### Challenge
Patches modify ROM data, but C code has hardcoded addresses.

### Solution
Virtual Memory Overlay system:

```cpp
// Original C code does:
void* sprite_data = (void*)0x0834A200;  // Hardcoded ROM address

// Our redirector:
uint8_t* PointerRedirector::mapAddress(uint32_t gba_addr) {
    if (gba_addr >= GBA_ROM_ADDR) {
        // Return pointer into patched buffer
        return patched_rom.base + (gba_addr - GBA_ROM_ADDR);
    }
    // ... other regions
}

// Usage:
void* sprite_data = redirector->mapAddress(0x0834A200);
// Returns pointer to patched data
```

### Layered Patching
```
Base ROM (original)
    ↓
Patch Layer 1 (QOL fixes)
    ↓
Patch Layer 2 (New features)
    ↓
Final ROM buffer (C code reads this)
```

## 6. Save File Compatibility

### GBA Flash Memory Format
- 128KB total
- 32 sectors of 4KB each
- Sector footer with checksum
- XOR encryption with key

### Host Implementation
- Direct binary compatible format
- Store in user directory
- Compression optional for modern storage

## 7. Performance Optimizations

### 1. Memory Pool
Single allocation reduces fragmentation, improves cache locality.

### 2. Lazy Rendering
Only render when display is visible.

### 3. Frame Skipping
Skip rendering if game logic takes too long.

### 4. Palette Caching
Convert GBA BGR555 to RGBA8 once, reuse.

### 5. Godot RenderingServer
Direct RID manipulation for zero-copy texture updates.

## 8. Thread Safety

### Challenge
GBA is single-threaded; Godot is multi-threaded.

### Solution
- Main thread only for GBA code
- Mutex protection for shared state
- Double-buffering for VRAM access

```cpp
class GBARenderer {
    mutable std::mutex vram_mutex;
    
    void render_frame() {
        std::lock_guard<std::mutex> lock(vram_mutex);
        // Access VRAM safely
    }
};
```

## 9. Endianness Considerations

Both GBA ARM and x86_64 are little-endian, so no byte swapping needed for most data.

However, some GBA data structures have mixed-endian fields:
```cpp
// Safe read for potentially cross-platform saves
uint32_t read_little_endian32(uint8_t* ptr) {
    return ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
}
```

## 10. Summary Table

| Aspect | GBA | Modern | Resolution |
|--------|-----|--------|------------|
| Architecture | ARM7TDMI 16-bit | x86_64/ARM64 64-bit | Software emulation layer |
| Clock | 16.78 MHz | 2+ GHz | Frame-based sync |
| Memory | 24-bit address | 64-bit address | Virtual translation |
| Pointers | 4 bytes | 8 bytes | Offset storage + translation |
| I/O | Hardware MMIO | Memory buffers | Register emulation |
| Graphics | Hardware scanline | Software render | Full-frame buffer |
| Audio | Hardware DMA | AudioStream | Buffer-based playback |
| Input | Hardware registers | Godot Input | Event translation |

## 11. Key Files

- `gba_memory_map.hpp` - Address translation definitions
- `gba_c_bridge.cpp` - C/C++ interface layer
- `gba_renderer.hpp/cpp` - Graphics bridge
- `rom_overlay.hpp/cpp` - Patching system
- `SConstruct` - Build configuration

This architecture successfully bridges two decades of hardware evolution while preserving the original game code unchanged.

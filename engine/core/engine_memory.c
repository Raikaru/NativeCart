#include "engine_internal.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

typedef struct EngineMappedRegion {
    uintptr_t address;
    size_t requested_size;
    size_t mapped_size;
    void *mapped_ptr;
} EngineMappedRegion;

enum {
    ENGINE_REGION_EWRAM,
    ENGINE_REGION_IWRAM,
    ENGINE_REGION_IOREG,
    ENGINE_REGION_PALETTE,
    ENGINE_REGION_VRAM,
    ENGINE_REGION_OAM,
    ENGINE_REGION_ROM,
    ENGINE_REGION_SRAM,
    ENGINE_REGION_COUNT
};

static EngineMappedRegion g_regions[ENGINE_REGION_COUNT];
static int g_memory_ready = 0;
static size_t g_loaded_rom_size = 0;
static const char *const g_region_names[ENGINE_REGION_COUNT] = {
    "EWRAM",
    "IWRAM",
    "IOREG",
    "PALETTE",
    "VRAM",
    "OAM",
    "ROM",
    "SRAM",
};

static size_t engine_page_size(void) {
#ifdef _WIN32
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return (size_t)info.dwPageSize;
#else
    long value = sysconf(_SC_PAGESIZE);
    if (value <= 0) {
        return 4096u;
    }
    return (size_t)value;
#endif
}

static size_t engine_align_page(size_t size) {
    size_t page_size = engine_page_size();
    size_t remainder = size % page_size;
    if (remainder == 0) {
        return size;
    }
    return size + (page_size - remainder);
}

static int engine_map_region(EngineMappedRegion *region, uintptr_t address, size_t size) {
    size_t mapped_size = engine_align_page(size);
    void *mapped_ptr;

#ifdef _WIN32
    mapped_ptr = VirtualAlloc((LPVOID)address, mapped_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (mapped_ptr == NULL || (uintptr_t)mapped_ptr != address) {
        DWORD err = GetLastError();
        if (mapped_ptr != NULL) {
            VirtualFree(mapped_ptr, 0, MEM_RELEASE);
        }
        fprintf(stderr,
                "[engine_memory] map_failed addr=0x%08lX req_size=0x%lX map_size=0x%lX ptr=0x%p gle=%lu\n",
                (unsigned long)address,
                (unsigned long)size,
                (unsigned long)mapped_size,
                mapped_ptr,
                (unsigned long)err);
        fflush(stderr);
        return 0;
    }
#else
    mapped_ptr = mmap((void *)address, mapped_size, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    if (mapped_ptr == MAP_FAILED || (uintptr_t)mapped_ptr != address) {
        if (mapped_ptr != MAP_FAILED) {
            munmap(mapped_ptr, mapped_size);
        }
        return 0;
    }
#endif

    region->address = address;
    region->requested_size = size;
    region->mapped_size = mapped_size;
    region->mapped_ptr = mapped_ptr;
    memset(mapped_ptr, 0, mapped_size);
    return 1;
}

/*
 * One-line proof that cartridge bytes (incl. hacks) landed in ENGINE_ROM.
 * Compare to hardcoded portable symbols (e.g. cores/firered/portable/rom_header_portable.c).
 * Enable: FIRERED_TRACE_ROM_CONSUMPTION=1
 */
static void engine_memory_trace_rom_cart_header(const uint8_t *rom_mapped, size_t rom_size)
{
    const char *e;
    unsigned i;

    e = getenv("FIRERED_TRACE_ROM_CONSUMPTION");
    if (e == NULL || e[0] == '\0' || strcmp(e, "0") == 0)
        return;
    if (rom_size < 0xB0u || rom_mapped == NULL)
        return;

    fprintf(stderr, "[firered rom-trace] GBA header in mapped ROM: game_code@0xAC=");
    for (i = 0; i < 4; i++)
        fprintf(stderr, "%02x", (unsigned)rom_mapped[0xACu + i]);
    fprintf(stderr, " title@0xA0=");
    for (i = 0; i < 12; i++)
        fprintf(stderr, "%02x", (unsigned)rom_mapped[0xA0u + i]);
    fprintf(stderr, " (portable UI often uses compiled *_Portable assets / script tables instead)\n");
    fflush(stderr);
}

static void engine_unmap_region(EngineMappedRegion *region) {
    if (region->mapped_ptr == NULL) {
        return;
    }

#ifdef _WIN32
    VirtualFree(region->mapped_ptr, 0, MEM_RELEASE);
#else
    munmap(region->mapped_ptr, region->mapped_size);
#endif

    region->mapped_ptr = NULL;
    region->address = 0;
    region->requested_size = 0;
    region->mapped_size = 0;
}

int engine_memory_init(const uint8_t *rom, size_t rom_size) {
    size_t copy_size;
    static const struct {
        int index;
        uintptr_t address;
        size_t size;
    } region_specs[] = {
        { ENGINE_REGION_EWRAM, ENGINE_EWRAM_ADDR, ENGINE_EWRAM_SIZE },
        { ENGINE_REGION_IWRAM, ENGINE_IWRAM_ADDR, ENGINE_IWRAM_SIZE },
        { ENGINE_REGION_IOREG, ENGINE_IOREG_ADDR, ENGINE_IOREG_SIZE },
        { ENGINE_REGION_PALETTE, ENGINE_PALETTE_ADDR, ENGINE_PALETTE_SIZE },
        { ENGINE_REGION_VRAM, ENGINE_VRAM_ADDR, ENGINE_VRAM_SIZE },
        { ENGINE_REGION_OAM, ENGINE_OAM_ADDR, ENGINE_OAM_SIZE },
        { ENGINE_REGION_ROM, ENGINE_ROM_ADDR, ENGINE_ROM_SIZE },
        { ENGINE_REGION_SRAM, ENGINE_SRAM_ADDR, ENGINE_SRAM_SIZE },
    };
    size_t i;

    if (rom == NULL || rom_size == 0 || rom_size > ENGINE_ROM_SIZE) {
        fprintf(stderr,
                "[engine_memory] init_guard_fail rom=%p rom_size=0x%lX rom_limit=0x%lX\n",
                (const void *)rom,
                (unsigned long)rom_size,
                (unsigned long)ENGINE_ROM_SIZE);
        fflush(stderr);
        return 0;
    }

    if (g_memory_ready) {
        return 1;
    }

    for (i = 0; i < sizeof(region_specs) / sizeof(region_specs[0]); ++i) {
        int index = region_specs[i].index;
        if (!engine_map_region(&g_regions[index], region_specs[i].address, region_specs[i].size)) {
            fprintf(stderr,
                    "[engine_memory] init_region_fail region=%s addr=0x%08lX size=0x%lX\n",
                    g_region_names[index],
                    (unsigned long)region_specs[i].address,
                    (unsigned long)region_specs[i].size);
            fflush(stderr);
            engine_memory_shutdown();
            return 0;
        }
    }

    copy_size = rom_size;
    memcpy((void *)(uintptr_t)ENGINE_ROM_ADDR, rom, copy_size);
    g_loaded_rom_size = copy_size;
    engine_memory_trace_rom_cart_header((const uint8_t *)(uintptr_t)ENGINE_ROM_ADDR, copy_size);

    *(volatile uint16_t *)(uintptr_t)ENGINE_REG_DISPCNT = ENGINE_DISPCNT_FORCED_BLANK;
    *(volatile uint16_t *)(uintptr_t)ENGINE_REG_KEYINPUT = 0x03FFu;

    g_memory_ready = 1;
    return 1;
}

void engine_memory_shutdown(void) {
    int i;

    for (i = ENGINE_REGION_COUNT - 1; i >= 0; --i) {
        engine_unmap_region(&g_regions[i]);
    }

    g_memory_ready = 0;
    g_loaded_rom_size = 0;
}

size_t engine_memory_get_loaded_rom_size(void) {
    return g_loaded_rom_size;
}

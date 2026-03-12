# PokeFireRed GDExtension Build System

This directory contains the SCons-based build system for the PokeFireRed GDExtension - a Godot 4.x extension that runs the original GBA PokeFireRed code.

## Overview

The build system compiles two distinct codebases into a single shared library:

1. **Original C Code** (`../src/*.c`): The original pokefirered GBA game code
2. **GDExtension Bridge** (`src/*.cpp`): C++17 wrapper that interfaces the C code with Godot

## Requirements

- **SCons**: `pip install scons`
- **Godot 4.x CPP bindings**: Download or clone [godot-cpp](https://github.com/godotengine/godot-cpp)
- **C/C++ Compiler**: GCC, Clang, or MSVC (Windows)
- **Python 3.7+**

## Project Structure

```
gdextension/
├── SConstruct              # Main build configuration
├── extension_api.json      # Godot API bindings (minimal)
├── README.md               # This file
├── include/
│   └── gba_memory_map.hpp  # Memory mapping definitions
├── src/
│   ├── register_types.cpp  # GDExtension entry point
│   └── gba_c_bridge.cpp    # C/C++ bridge implementation
└── bin/                    # Build output (generated)
```

## Quick Start

### 1. Setup godot-cpp

```bash
# Clone godot-cpp at the project root
cd D:\Godot\pokefirered-master
git clone -b 4.1 https://github.com/godotengine/godot-cpp.git

# Build godot-cpp (or let SCons do it automatically)
cd godot-cpp
scons target=debug platform=windows
```

### 2. Set Environment Variable (Optional)

```bash
# Windows (Command Prompt)
set GODOT_CPP_PATH=D:\Godot\pokefirered-master\godot-cpp

# Windows (PowerShell)
$env:GODOT_CPP_PATH = "D:\Godot\pokefirered-master\godot-cpp"

# Linux/macOS
export GODOT_CPP_PATH=/path/to/godot-cpp
```

### 3. Build the Extension

```bash
cd D:\Godot\pokefirered-master\gdextension

# Debug build (default)
scons

# Release build
scons target=release

# Specific platform
scons platform=windows target=release

# With custom suffix
scons extra_suffix=custom
```

### 4. Build Targets

```bash
# Build library only
scons library

# Generate .gdextension config
scons config

# Install to demo project
scons install

# Build everything
scons all
```

## Build Options

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| `platform` | windows/linux/macos/android/ios/web | auto-detected | Target platform |
| `target` | debug/release | debug | Build type |
| `arch` | auto/x86_64/x86_32/arm64/arm32 | auto | Architecture |
| `use_llvm` | yes/no | no (yes on macOS) | Use Clang instead of GCC |
| `use_static_cpp` | yes/no | no | Static link C++ runtime (Windows) |
| `godot_cpp_path` | path | ../godot-cpp | Path to godot-cpp |
| `extra_suffix` | string | "" | Custom library name suffix |

### Examples

```bash
# Windows x86_64 release with MSVC
scons platform=windows target=release arch=x86_64

# Linux debug build
scons platform=linux target=debug

# macOS ARM64 release with Clang
scons platform=macos target=release arch=arm64 use_llvm=yes

# WebAssembly build
scons platform=web target=release arch=wasm32
```

## Output

The build produces:

```
gdextension/bin/
├── libpokefirered.debug.windows.x86_64.dll   # Windows debug
├── libpokefirered.release.windows.x86_64.dll # Windows release
├── libpokefirered.debug.linux.x86_64.so      # Linux debug
├── libpokefirered.release.linux.x86_64.so    # Linux release
└── ... (platform-specific)

gdextension/demo/bin/
└── [symlinks/copies of the above]

gdextension/demo/
└── pokefirered.gdextension  # Auto-generated config
```

## Troubleshooting

### "godot-cpp library not found"

Build godot-cpp manually first:
```bash
cd $GODOT_CPP_PATH
scons target=debug platform=windows arch=x86_64
```

### "Cannot find gdextension_interface.h"

Ensure godot-cpp is properly initialized:
```bash
cd $GODOT_CPP_PATH
git submodule update --init --recursive
```

### Linker errors on Windows

If using MinGW, ensure you have the correct toolchain:
```bash
# Install MSYS2 MinGW-w64
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-SCons
```

### "undefined reference to gdextension..."

Make sure to link against the correct godot-cpp library:
- Debug builds need `libgodot-cpp.debug.*.a`
- Release builds need `libgodot-cpp.release.*.a`

## Integration with Godot

The generated `.gdextension` file automatically configures:
- Library paths for all platforms
- Debug vs Release builds
- Architecture variants

Simply copy the `gdextension/demo/` folder contents to your Godot project's root.

## Development Workflow

1. **Make changes** to C/C++ source files
2. **Build**: `scons target=debug`
3. **Test** in Godot - the library auto-reloads in debug builds
4. **Release**: `scons target=release`

## Compiler Flags

### C Code (Original GBA Code)
- Standard: C11 (`-std=c11`)
- Defines: `AGB_PRINT`, `PORTABLE`, `NOCOAST`, `NO_SAVEFLASH`
- Warnings suppressed: GBA-specific code patterns

### C++ Code (Bridge)
- Standard: C++17 (`-std=c++17`)
- Defines: `GDEXTENSION`, `GODOT_CPP_ENABLED`
- Includes: godot-cpp headers + original GBA headers

## License

This build system is part of the PokeFireRed project. The original game code is proprietary to Nintendo/Game Freak. The GDExtension bridge is open source.

## Support

For build issues:
1. Check SCons version: `scons --version`
2. Verify godot-cpp path: Check `GODOT_CPP_PATH` environment variable
3. Clean and rebuild: `scons -c && scons`
4. Verbose build: `scons VERBOSE=1`

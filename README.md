# decomp-engine

`decomp-engine` is a portable host/runtime project that separates a reusable decomp
engine shell from a FireRed-specific game core.

The current public layout is split into:

- `engine/` - reusable runtime, renderer, shell interfaces, and Godot shell code
- `engine/shells/sdl/` - standalone SDL shell using the same engine/core APIs
- `cores/firered/` - FireRed-specific adapter, portable glue, and generated data
- `gdextension/` - compatibility-facing build entrypoint for the Godot shell

The FireRed runtime currently boots, steps frames, and renders through the Godot
host path used by this repository.

## Repository Layout

- `engine/core/` - canonical runtime, renderer, memory, input, audio, and platform code
- `engine/interfaces/` - generic interfaces such as `GameCore`
- `engine/shells/godot/` - Godot shell implementation
- `cores/firered/` - FireRed core adapter and FireRed-only portable data/glue
- `docs/architecture.txt` - architecture summary
- `INSTALL.md` - setup and compile instructions

## Legal / Asset Note

This repository does not ship a commercial Game Boy Advance ROM.
You must provide your own legally obtained ROM and any local copyrighted inputs
required for personal building/testing.

The `.gitignore` is configured to exclude ROMs, build outputs, saves, logs, and
other local-only artifacts that should not be published.

## Credits

- [pret](https://github.com/pret) and the `pokefirered` decomp community for the
  original reverse-engineering and decompilation work this project builds on
- Nintendo, Game Freak, and Creatures for the original Pokemon FireRed game
- [godot-cpp](https://github.com/godotengine/godot-cpp) for the Godot GDExtension bindings
- Contributors to this portable runtime / shell split and FireRed host integration

## Documentation

- Install and build instructions: [INSTALL.md](INSTALL.md)
- Architecture notes: [docs/architecture.txt](docs/architecture.txt)
- Build notes: [docs/build.txt](docs/build.txt)

## External Dependencies

- `godot-cpp` is intentionally not vendored in this repository
- clone it separately as described in [INSTALL.md](INSTALL.md)

## Current Status

- canonical reusable runtime path lives in `engine/core/`
- SDL and Godot both now exist as shells over the generic engine/core path
- canonical FireRed-specific portable/generated glue lives in `cores/firered/`
- compatibility wrappers remain in `pokefirered_core/` and `gdextension/` so the
  current build still works while the public architecture is cleaner

## Release Checklist

- confirm no ROMs, saves, logs, or local build artifacts are staged
- confirm `.gitignore` still covers local-only copyrighted inputs and generated outputs
- build with `cd gdextension && scons -Q platform=windows target=debug -j4`
- smoke-check with `build/runtime_progress_runner.exe baserom.gba`
- review `README.md`, `INSTALL.md`, `docs/architecture.txt`, and `docs/build.txt`
- verify `godot-cpp/` is installed locally as an external dependency and not tracked in git

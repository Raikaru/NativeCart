# decomp-engine

`decomp-engine` is a portable host/runtime project that separates a reusable decomp
engine shell from a FireRed-specific game core.

The current public layout is split into:

- `engine/` — reusable runtime, renderer, shell interfaces, and the **SDL/native** shell
- `engine/shells/sdl/` — **canonical** standalone host using the same engine/core APIs
- `cores/firered/` — FireRed-specific adapter, portable glue, and generated data
- `tools/portable_generators/` — shared portable data generators (invoked from the SDL SCons build)

The FireRed portable runtime boots, steps frames, and renders through **engine/core**;
the **SDL shell** is the only supported day-to-day host.

## Repository Layout

- `engine/core/` — canonical runtime, renderer, memory, input, audio, and platform code
- `engine/interfaces/` — generic interfaces such as `GameCore`
- `cores/firered/` — FireRed core adapter and FireRed-only portable data/glue
- `docs/architecture.txt` — architecture summary
- `INSTALL.md` — setup and compile instructions

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
- Contributors to this portable runtime / shell split and FireRed host integration

## Documentation

- Install and build instructions: [INSTALL.md](INSTALL.md)
- Upstream / pret **how-to** (merge, jsonproc, build glue): [docs/upstream_pokefirered.md](docs/upstream_pokefirered.md)
- Vanilla **reference lane** (policy, **pinned pret baseline §0**, **lane state / stopping point**, **metrics policy**, observed compare/ROM notes): [docs/upstream_reference_lane.md](docs/upstream_reference_lane.md) — **not** a claim that **`make compare_*` is green** on `main`; bounded pret-shaped source reconciliation is **documented as complete** there
- Architecture notes: [docs/architecture.txt](docs/architecture.txt)
- Build notes: [docs/build.txt](docs/build.txt)
- Portable runtime troubleshooting notes: [docs/portable_runtime_notes.md](docs/portable_runtime_notes.md)
- SDL portable CI + local scripts: [.github/workflows/portable_host.yml](.github/workflows/portable_host.yml), `tools/verify_portable_default.sh`, `tools/verify_portable_default.ps1` — same offline Python gates as CI (Direction B / Project C block-word + Phase 3 constants / layout bins / Direction D) then SDL build; **`python tools/run_offline_build_gates.py`** when **`make`** is unavailable (see [INSTALL.md](INSTALL.md))

## Search Workflow

- Use `rg` / `rg --files` for normal text and file discovery.
- Use `ast-grep` (`sg`) for syntax-aware C/C++ searches when a plain text match is too noisy.
- Treat plain `grep` as fallback-only for environments missing the better tools.

## External Dependencies

- **SDL3** — required for the native shell (see [INSTALL.md](INSTALL.md))

## Current Status

- Canonical reusable runtime path lives in `engine/core/`
- **SDL/native** is the debugging, validation, and primary host shell
- Canonical FireRed-specific portable/generated glue lives in `cores/firered/`
- `pokefirered_core/` remains as a compatibility layer alongside `engine/` and `cores/firered/`

## Release Checklist

- confirm no ROMs, saves, logs, or local build artifacts are staged
- confirm `.gitignore` still covers local-only copyrighted inputs and generated outputs
- run `tools/verify_portable_default.ps1` / `.sh` or CI-equivalent (offline validators + SDL debug; see [INSTALL.md](INSTALL.md))
- smoke-check with `build/runtime_progress_runner.exe baserom.gba` and/or `build/decomp_engine_sdl.exe baserom.gba`
- review `README.md`, `INSTALL.md`, `docs/architecture.txt`, and `docs/build.txt`

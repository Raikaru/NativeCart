# Portable data generators

Python scripts that emit C sources under `pokefirered_core/generated/` (and related
paths) from decomp JSON/asm inputs. Invoked at the start of
**`engine/shells/sdl/SConstruct`** via **`tools/portable_scons_helpers.run_portable_generators`**.

They are **not** host-specific; they live under `tools/` so the SDL shell build does
not depend on a separate extension tree.

# Shared portable-core source lists and generator hooks for SCons.
# Used by engine/shells/sdl/SConstruct — single source of truth for portable C inputs.

from __future__ import annotations

import glob
import os
import subprocess
import sys
from typing import Dict, List, Mapping, Optional, Sequence, Tuple

# Basenames under tools/portable_generators/, invocation order.
_PORTABLE_GENERATOR_SCRIPTS: Sequence[str] = (
    "generate_portable_region_map_overrides.py",
    "generate_portable_battle_script_data.py",
    "generate_portable_battle_ai_data.py",
    "generate_portable_battle_anim_data.py",
    "generate_portable_event_script_data.py",
    "generate_portable_cry_data.py",
    "generate_portable_item_data.py",
    "generate_portable_map_data.py",
)

# Basenames under cores/firered/portable/ for -fno-asm translation units.
_PORTABLE_ASM_BASENAMES: Sequence[str] = (
    "isagbprn_portable.c",
    "libgcnmultiboot_portable.c",
    "m4a_portable.c",
    "main_portable.c",
    "multiboot_programs_portable.c",
    "multiboot_portable.c",
    "script_portable.c",
    "librfu_intr_portable.c",
)

# (bucket, filename) — bucket selects a directory key from portable_repo_paths().
_CoreEntry = Tuple[str, str]
_CORE_SOURCE_ENTRIES: Sequence[_CoreEntry] = (
    ("engine_core", "core_loader.c"),
    ("engine_core", "engine_runtime.c"),
    ("engine_core", "engine_video.c"),
    ("engine_core", "engine_audio.c"),
    ("engine_core", "engine_input.c"),
    ("engine_core", "engine_runtime_backend.c"),
    ("engine_core", "mod_patch_bps.c"),
    ("engine_core", "mod_patch_ups.c"),
    ("engine_core", "engine_renderer_backend.c"),
    ("engine_core", "engine_memory.c"),
    ("engine_core", "engine_audio_backend.c"),
    ("engine_core", "engine_input_backend.c"),
    ("firered_module", "firered_core.c"),
    ("core_src", "firered_runtime.c"),
    ("core_src", "firered_memory.c"),
    ("core_src", "firered_video.c"),
    ("core_src", "firered_audio.c"),
    ("core_src", "firered_input.c"),
    ("engine_core", "engine_platform_portable.c"),
    ("firered_portable", "mystery_event_script_portable.c"),
    ("firered_portable", "portable_script_pointer_resolver.c"),
    ("firered_portable", "rom_header_portable.c"),
    ("firered_portable", "firered_portable_rom_queries.c"),
    ("firered_portable", "firered_portable_rom_compat.c"),
    ("firered_portable", "firered_portable_rom_asset_profile.c"),
    ("firered_portable", "firered_portable_rom_asset_bind.c"),
    ("firered_portable", "firered_portable_rom_text_family.c"),
    ("firered_portable", "firered_portable_rom_u32_table.c"),
    ("firered_portable", "firered_portable_rom_std_scripts_table.c"),
    ("firered_portable", "firered_portable_rom_special_vars_table.c"),
    ("firered_portable", "firered_portable_rom_field_effect_script_pointers.c"),
    ("firered_portable", "firered_portable_rom_field_effect_pointer_overlay.c"),
    ("firered_portable", "firered_portable_rom_event_script_pointer_overlay.c"),
    ("firered_portable", "firered_portable_rom_map_object_event_script_pointer_overlay.c"),
    ("firered_portable", "firered_portable_rom_battle_script_pointer_overlay.c"),
    ("firered_portable", "firered_portable_rom_battle_anims_general_table.c"),
    ("firered_portable", "firered_portable_rom_battle_anim_pointer_overlay.c"),
    ("firered_portable", "firered_portable_rom_battle_ai_scripts_table.c"),
    ("firered_portable", "firered_portable_rom_battle_ai_fragment_prefix.c"),
    ("firered_portable", "firered_portable_rom_species_national_dex.c"),
    ("firered_portable", "firered_portable_rom_species_hoenn_dex_tables.c"),
    ("firered_portable", "firered_portable_rom_species_cry_id_table.c"),
    ("firered_portable", "firered_portable_rom_hm_moves_table.c"),
    ("firered_portable", "firered_portable_rom_deoxys_base_stats_table.c"),
    ("firered_portable", "firered_portable_rom_experience_tables.c"),
    ("firered_portable", "firered_portable_rom_type_effectiveness_table.c"),
    ("firered_portable", "firered_portable_rom_species_info_table.c"),
    ("firered_portable", "firered_portable_rom_mon_pic_layout.c"),
    ("firered_portable", "firered_portable_rom_tmhm_learnsets_table.c"),
    ("firered_portable", "firered_portable_rom_evolution_table.c"),
    ("firered_portable", "firered_portable_rom_battle_moves_table.c"),
    ("firered_portable", "firered_portable_rom_move_names_table.c"),
    ("firered_portable", "firered_portable_rom_battle_tower_mon_templates.c"),
    ("firered_portable", "firered_portable_rom_battle_terrain_tables.c"),
    ("firered_portable", "firered_portable_rom_tutor_tables.c"),
    ("firered_portable", "firered_portable_rom_heal_locations_table.c"),
    ("firered_portable", "firered_portable_rom_region_map_section_grids.c"),
    ("firered_portable", "firered_portable_rom_region_map_section_layout.c"),
    ("firered_portable", "firered_portable_rom_region_map_fly_destinations.c"),
    ("firered_portable", "firered_portable_rom_region_map_mapsec_names.c"),
    ("firered_portable", "firered_portable_rom_map_layout_metatiles.c"),
    ("firered_portable", "firered_portable_rom_map_header_scalars.c"),
    ("firered_portable", "firered_portable_rom_wild_encounter_family.c"),
    ("firered_portable", "firered_portable_rom_egg_moves_table.c"),
    ("firered_portable", "firered_portable_rom_level_up_learnsets_family.c"),
    ("firered_portable", "firered_portable_rom_lz_asset.c"),
    ("firered_portable", "firered_portable_title_screen_rom.c"),
    ("firered_portable", "firered_portable_oak_speech_text_rom.c"),
    ("firered_portable", "firered_portable_oak_speech_bg_rom.c"),
    ("firered_portable", "firered_portable_main_menu_pal_rom.c"),
    ("firered_portable", "region_map_portable.c"),
    ("firered_portable", "wild_encounter_portable.c"),
    ("firered_generated_data", "battle_scripts_portable_data.c"),
    ("firered_generated_data", "battle_ai_scripts_portable_data.c"),
    ("firered_generated_data", "battle_anim_scripts_portable_data.c"),
    ("firered_generated_data", "cry_tables_portable_data.c"),
    ("firered_generated_data", "event_scripts_portable_data.c"),
    ("firered_generated_data", "field_effect_scripts_portable_data.c"),
    ("firered_generated_data", "map_data_portable.c"),
    ("firered_generated_data", "map_object_event_scripts_portable_data.c"),
)

_BUCKET_KEYS: Dict[str, str] = {
    "engine_core": "engine_core_dir",
    "core_src": "core_src_dir",
    "firered_module": "firered_core_module_dir",
    "firered_portable": "firered_portable_dir",
    "firered_generated_data": "firered_generated_data_dir",
}


def portable_repo_paths(root_dir: str) -> Dict[str, str]:
    """Canonical path layout for portable C builds (repo root = decomp-engine root)."""
    root_dir = os.path.abspath(root_dir)
    core_dir = os.path.join(root_dir, "pokefirered_core")
    firered_core_module_dir = os.path.join(root_dir, "cores", "firered")
    engine_root_dir = os.path.join(root_dir, "engine")
    return {
        "root_dir": root_dir,
        "core_dir": core_dir,
        "core_include_dir": os.path.join(core_dir, "include"),
        "core_src_dir": os.path.join(core_dir, "src"),
        "generated_dir": os.path.join(core_dir, "generated"),
        "generated_include_dir": os.path.join(core_dir, "generated", "include"),
        "generated_src_dir": os.path.join(core_dir, "generated", "src"),
        "engine_root_dir": engine_root_dir,
        "engine_core_dir": os.path.join(engine_root_dir, "core"),
        "engine_interfaces_dir": os.path.join(engine_root_dir, "interfaces"),
        "firered_core_module_dir": firered_core_module_dir,
        "firered_portable_dir": os.path.join(firered_core_module_dir, "portable"),
        "firered_generated_data_dir": os.path.join(firered_core_module_dir, "generated", "data"),
        "engine_src_dir": os.path.join(root_dir, "src"),
        "engine_transformed_dir": os.path.join(root_dir, "src_transformed"),
        "engine_include_dir": os.path.join(root_dir, "include"),
        "engine_gba_include_dir": os.path.join(root_dir, "include", "gba"),
        "portable_gen_dir": os.path.join(root_dir, "tools", "portable_generators"),
    }


def run_portable_generators(root_dir: str, generated_dir: str, python_executable: Optional[str] = None) -> None:
    """Run shared portable data generators into pokefirered_core/generated."""
    py = python_executable or sys.executable
    root_abs = os.path.abspath(root_dir)
    gen_abs = os.path.abspath(generated_dir)
    portable_gen_dir = os.path.join(root_abs, "tools", "portable_generators")
    for name in _PORTABLE_GENERATOR_SCRIPTS:
        script = os.path.join(portable_gen_dir, name)
        subprocess.check_call([py, script, root_abs, gen_abs])


def merged_engine_c_sources(experimental_audio: bool, paths: Mapping[str, str]) -> List[str]:
    """Merged vanilla + transformed engine .c set (portable exclusions)."""
    engine_src_dir = paths["engine_src_dir"]
    engine_transformed_dir = paths["engine_transformed_dir"]
    originals = {os.path.basename(path): path for path in glob.glob(os.path.join(engine_src_dir, "*.c"))}
    transformed = {os.path.basename(path): path for path in glob.glob(os.path.join(engine_transformed_dir, "*.c"))}
    originals.update(transformed)
    originals.pop("librfu_intr.c", None)
    originals.pop("isagbprn.c", None)
    if not experimental_audio:
        originals.pop("m4a.c", None)
        originals.pop("m4a_tables.c", None)
    originals.pop("main.c", None)
    originals.pop("multiboot.c", None)
    originals.pop("region_map.c", None)
    originals.pop("script.c", None)
    originals.pop("wild_encounter.c", None)
    return [originals[name] for name in sorted(originals.keys())]


def portable_asm_c_sources(paths: Mapping[str, str]) -> List[str]:
    d = paths["firered_portable_dir"]
    return [os.path.join(d, b) for b in _PORTABLE_ASM_BASENAMES]


def core_c_sources(paths: Mapping[str, str]) -> List[str]:
    out: List[str] = []
    for bucket, name in _CORE_SOURCE_ENTRIES:
        key = _BUCKET_KEYS[bucket]
        out.append(os.path.join(paths[key], name))
    return out


def portable_release_sensitive_prefixes(root_dir: str) -> List[str]:
    """Path prefixes compiled with conservative flags in SDL release builds."""
    root_dir = os.path.abspath(root_dir)
    return [
        os.path.normcase(os.path.join(root_dir, "src")),
        os.path.normcase(os.path.join(root_dir, "src_transformed")),
        os.path.normcase(os.path.join(root_dir, "cores", "firered")),
        os.path.normcase(os.path.join(root_dir, "pokefirered_core")),
    ]

#!/usr/bin/env python3

import json
import ast
from difflib import SequenceMatcher
import re
import sys
from pathlib import Path


SEVII_ALIAS_FIXES = {
    "MAP_PROTOTYPE_SEVIIISLE_6": "MAP_PROTOTYPE_SEVII_ISLE_6",
    "MAP_PROTOTYPE_SEVIIISLE_7": "MAP_PROTOTYPE_SEVII_ISLE_7",
    "MAP_PROTOTYPE_SEVIIISLE_8": "MAP_PROTOTYPE_SEVII_ISLE_8",
    "MAP_PROTOTYPE_SEVIIISLE_9": "MAP_PROTOTYPE_SEVII_ISLE_9",
    "MAP_SEVEN_ISLAND_TANOBY_RUINS_DILFORDCHAMBER": "MAP_SEVEN_ISLAND_TANOBY_RUINS_DILFORD_CHAMBER",
    "MAP_SEVEN_ISLAND_TANOBY_RUINS_LIPTOOCHAMBER": "MAP_SEVEN_ISLAND_TANOBY_RUINS_LIPTOO_CHAMBER",
    "MAP_SEVEN_ISLAND_TANOBY_RUINS_RIXYCHAMBER": "MAP_SEVEN_ISLAND_TANOBY_RUINS_RIXY_CHAMBER",
    "MAP_SEVEN_ISLAND_TANOBY_RUINS_SCUFIBCHAMBER": "MAP_SEVEN_ISLAND_TANOBY_RUINS_SCUFIB_CHAMBER",
    "MAP_SEVEN_ISLAND_TANOBY_RUINS_VIAPOISCHAMBER": "MAP_SEVEN_ISLAND_TANOBY_RUINS_VIAPOIS_CHAMBER",
    "MAP_SEVEN_ISLAND_TANOBY_RUINS_WEEPTHCHAMBER": "MAP_SEVEN_ISLAND_TANOBY_RUINS_WEEPTH_CHAMBER",
}

ORIGINAL_REGION_MAP_STRING_IDS = {
    "MT__MOON",
    "DIGLETT_S_CAVE",
    "VICTORY_ROAD",
    "POK__MON_MANSION",
    "SAFARI_ZONE",
    "ROCK_TUNNEL",
    "SEAFOAM_ISLANDS",
    "POK__MON_TOWER",
    "CERULEAN_CAVE",
    "POWER_PLANT",
    "MT__EMBER",
    "BERRY_FOREST",
    "ICEFALL_CAVE",
    "LOST_CAVE",
    "TANOBY_CHAMBERS",
    "ALTERING_CAVE",
    "PATTERN_BUSH",
    "DOTTED_HOLE",
}


def clean_string(value: str) -> str:
    out = []
    for ch in value:
        if ch.isascii() and ch.isalnum():
            out.append(ch.upper())
        elif ch.isascii():
            out.append("_")
        else:
            out.append("__")
    return "".join(out)


def write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8", newline="\n")


def load_map_sections(root_dir: Path):
    path = root_dir / "src" / "data" / "region_map" / "region_map_sections.json"
    data = json.loads(path.read_text(encoding="utf-8"))
    return data["map_sections"]


def load_wild_encounter_groups(root_dir: Path):
    path = root_dir / "src" / "data" / "wild_encounters.json"
    data = json.loads(path.read_text(encoding="utf-8"))
    return data["wild_encounter_groups"]


def load_charmap(root_dir: Path):
    path = root_dir / "charmap.txt"
    mapping = {}
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.split("#", 1)[0].strip()
        if not line or "=" not in line:
            continue
        key_text, value_text = [part.strip() for part in line.split("=", 1)]
        if not key_text.startswith("'"):
            continue
        try:
            char = ast.literal_eval(key_text)
        except Exception:
            continue
        values = []
        for part in value_text.split():
            if re.fullmatch(r"[0-9A-Fa-f]{2}", part) is None:
                break
            values.append(int(part, 16))
        if len(char) == 1 and len(values) == 1:
            mapping[char] = values[0]
    return mapping


def encode_text(text: str, charmap) -> list[int]:
    encoded = []
    for char in text:
        if char not in charmap:
            raise ValueError(f"Missing charmap entry for {char!r} in {text!r}")
        encoded.append(charmap[char])
    encoded.append(0xFF)
    return encoded


def compact_symbol(name: str) -> str:
    return name.replace("_", "")


def parse_defined_map_symbols(root_dir: Path):
    path = root_dir / "include" / "constants" / "map_groups.h"
    text = path.read_text(encoding="utf-8")
    defined = set(re.findall(r"^#define\s+(MAP_[A-Z0-9_]+)\b", text, re.MULTILINE))
    return text, defined


def collect_referenced_map_symbols(wild_encounter_groups):
    referenced = set()
    for group in wild_encounter_groups:
        if not group.get("for_maps"):
            continue
        for encounter in group.get("encounters", []):
            map_name = encounter.get("map")
            if map_name:
                referenced.add(map_name)
    return referenced


def choose_best_alias(alias_name: str, matches):
    if not matches:
        return None
    ranked = sorted(
        ((SequenceMatcher(None, alias_name, match).ratio(), match) for match in matches),
        reverse=True,
    )
    if len(ranked) > 1 and ranked[0][0] == ranked[1][0]:
        return None
    return ranked[0][1]


def generate_region_map_sections_header(map_sections) -> str:
    lines = [
        "#ifndef GUARD_CONSTANTS_REGION_MAP_SECTIONS_H",
        "#define GUARD_CONSTANTS_REGION_MAP_SECTIONS_H",
        "",
        "enum {",
    ]
    for section in map_sections:
        lines.append(f"    {section['id']},")
    lines.extend([
        "    MAPSEC_NONE,",
        "    MAPSEC_COUNT",
        "};",
        "",
        "#define KANTO_MAPSEC_START MAPSEC_PALLET_TOWN",
        "#define SEVII_MAPSEC_START MAPSEC_ONE_ISLAND",
        "",
        "#ifndef METLOC_SPECIAL_EGG",
        "#define METLOC_SPECIAL_EGG 0xFD",
        "#endif",
        "#ifndef METLOC_IN_GAME_TRADE",
        "#define METLOC_IN_GAME_TRADE 0xFE",
        "#endif",
        "#ifndef METLOC_FATEFUL_ENCOUNTER",
        "#define METLOC_FATEFUL_ENCOUNTER 0xFF",
        "#endif",
        "",
        "#endif",
        "",
    ])
    return "\n".join(lines)


def build_name_metadata(map_sections):
    owners = {}
    metadata = {}
    for section in map_sections:
        name = section.get("name")
        if not name:
            continue
        ident = clean_string(name)
        if name not in owners:
            owners[name] = section["id"]
            suffix = ""
        elif section.get("name_clone"):
            suffix = "_Clone"
        else:
            suffix = ""
        metadata[section["id"]] = (name, ident, suffix)
    return metadata


def generate_region_map_strings_header(map_sections, charmap) -> str:
    metadata = build_name_metadata(map_sections)
    emitted = set()
    lines = [
        "#ifndef GUARD_DATA_REGION_MAP_REGION_MAP_ENTRY_STRINGS_H",
        "#define GUARD_DATA_REGION_MAP_REGION_MAP_ENTRY_STRINGS_H",
        "",
    ]
    for section in map_sections:
        info = metadata.get(section["id"])
        if info is None:
            continue
        name, ident, suffix = info
        symbol = f"sMapsecName_{ident}{suffix}"
        if suffix == "" and ident in ORIGINAL_REGION_MAP_STRING_IDS:
            continue
        if symbol in emitted:
            continue
        emitted.add(symbol)
        encoded = ", ".join(f"0x{value:02X}" for value in encode_text(name, charmap))
        lines.append(f"static const u8 sMapsecName_{ident}{suffix}[] = {{ {encoded} }};")
    lines.extend([
        "",
        "#endif",
        "",
    ])
    return "\n".join(lines)


def generate_region_map_entries_header(map_sections) -> str:
    metadata = build_name_metadata(map_sections)
    lines = [
        "#ifndef GUARD_DATA_REGION_MAP_REGION_MAP_ENTRIES_H",
        "#define GUARD_DATA_REGION_MAP_REGION_MAP_ENTRIES_H",
        "",
        "static const u8 *const sMapNames[] = {",
    ]
    for section in map_sections:
        info = metadata.get(section["id"])
        if info is None:
            continue
        _, ident, suffix = info
        lines.append(f"    [{section['id']} - KANTO_MAPSEC_START] = sMapsecName_{ident}{suffix},")
    lines.extend([
        "};",
        "",
        "static const u16 sMapSectionTopLeftCorners[MAPSEC_COUNT][2] = {",
    ])
    for section in map_sections:
        if "x" not in section or "y" not in section:
            continue
        lines.append(f"    [{section['id']} - KANTO_MAPSEC_START] = {{ {section['x']}, {section['y']} }},")
    lines.extend([
        "};",
        "",
        "static const u16 sMapSectionDimensions[MAPSEC_COUNT][2] = {",
    ])
    for section in map_sections:
        if "width" not in section or "height" not in section:
            continue
        lines.append(f"    [{section['id']} - KANTO_MAPSEC_START] = {{ {section['width']}, {section['height']} }},")
    lines.extend([
        "};",
        "",
        "#endif",
        "",
    ])
    return "\n".join(lines)


def generate_map_group_aliases_header(root_dir: Path, wild_encounter_groups) -> str:
    text, defined = parse_defined_map_symbols(root_dir)
    lines = [
        "#ifndef FIRERED_PORTABLE_MAP_GROUP_ALIASES_H",
        "#define FIRERED_PORTABLE_MAP_GROUP_ALIASES_H",
        "",
    ]

    emitted = set()

    for source_name, alias_name in SEVII_ALIAS_FIXES.items():
        if source_name in text:
            lines.append(f"#define {alias_name} {source_name}")
            emitted.add(alias_name)

    compact_defined = {}
    for name in defined:
        compact_defined.setdefault(compact_symbol(name), []).append(name)

    for alias_name in sorted(collect_referenced_map_symbols(wild_encounter_groups)):
        if alias_name in defined or alias_name in emitted:
            continue
        matches = compact_defined.get(compact_symbol(alias_name), [])
        best_match = choose_best_alias(alias_name, matches)
        if best_match is not None:
            lines.append(f"#define {alias_name} {best_match}")
            emitted.add(alias_name)

    lines.extend([
        "",
        "#endif",
        "",
    ])
    return "\n".join(lines)


def emit_wild_encounter_rate_macros(lines, field):
    field_name = field["type"].upper()
    if "groups" not in field:
        running_total = 0
        for index, encounter_rate in enumerate(field["encounter_rates"]):
            running_total += encounter_rate
            lines.append(f"#define ENCOUNTER_CHANCE_{field_name}_SLOT_{index} {running_total}")
        lines.append(f"#define ENCOUNTER_CHANCE_{field_name}_TOTAL (ENCOUNTER_CHANCE_{field_name}_SLOT_{len(field['encounter_rates']) - 1})")
        return

    for subgroup_name, slot_indices in field["groups"].items():
        running_total = 0
        subgroup = subgroup_name.upper()
        for slot_index in slot_indices:
            running_total += field["encounter_rates"][slot_index]
            lines.append(f"#define ENCOUNTER_CHANCE_{field_name}_{subgroup}_SLOT_{slot_index} {running_total}")
        lines.append(f"#define ENCOUNTER_CHANCE_{field_name}_{subgroup}_TOTAL (ENCOUNTER_CHANCE_{field_name}_{subgroup}_SLOT_{slot_indices[-1]})")


def maybe_emit_game_guard(lines, base_label, opening):
    if "LeafGreen" in base_label:
        lines.append("#ifdef LEAFGREEN" if opening else "#endif")
        return True
    if "FireRed" in base_label:
        lines.append("#ifdef FIRERED" if opening else "#endif")
        return True
    return False


def emit_wild_mons_table(lines, base_label, field_key, field_suffix, encounter):
    if field_key not in encounter:
        return
    field = encounter[field_key]
    lines.append(f"static const struct WildPokemon {base_label}_{field_suffix}[] = {{")
    for wild_mon in field["mons"]:
        lines.append(f"    {{ {wild_mon['min_level']}, {wild_mon['max_level']}, {wild_mon['species']} }},")
    lines.append("};")
    lines.append("")
    lines.append(f"static const struct WildPokemonInfo {base_label}_{field_suffix}Info = {{ {field['encounter_rate']}, {base_label}_{field_suffix} }};")
    lines.append("")


def generate_wild_encounters_header(wild_encounter_groups) -> str:
    lines = [
        "#ifndef GUARD_DATA_WILD_ENCOUNTERS_H",
        "#define GUARD_DATA_WILD_ENCOUNTERS_H",
        "",
    ]

    for group in wild_encounter_groups:
        if group.get("for_maps"):
            for field in group.get("fields", []):
                emit_wild_encounter_rate_macros(lines, field)
            lines.append("")

        for encounter in group.get("encounters", []):
            guarded = maybe_emit_game_guard(lines, encounter["base_label"], True)
            emit_wild_mons_table(lines, encounter["base_label"], "land_mons", "LandMons", encounter)
            emit_wild_mons_table(lines, encounter["base_label"], "water_mons", "WaterMons", encounter)
            emit_wild_mons_table(lines, encounter["base_label"], "rock_smash_mons", "RockSmashMons", encounter)
            emit_wild_mons_table(lines, encounter["base_label"], "fishing_mons", "FishingMons", encounter)
            if guarded:
                maybe_emit_game_guard(lines, encounter["base_label"], False)
                lines.append("")

        lines.append(f"const struct WildPokemonHeader {group['label']}[] = {{")
        for encounter in group.get("encounters", []):
            guarded = maybe_emit_game_guard(lines, encounter["base_label"], True)
            lines.append("    {")
            if group.get("for_maps"):
                lines.append(f"        .mapGroup = MAP_GROUP({encounter['map']}),")
                lines.append(f"        .mapNum = MAP_NUM({encounter['map']}),")
            else:
                lines.append("        .mapGroup = 0,")
                lines.append("        .mapNum = 0,")
            lines.append(f"        .landMonsInfo = {'&' + encounter['base_label'] + '_LandMonsInfo' if 'land_mons' in encounter else 'NULL'},")
            lines.append(f"        .waterMonsInfo = {'&' + encounter['base_label'] + '_WaterMonsInfo' if 'water_mons' in encounter else 'NULL'},")
            lines.append(f"        .rockSmashMonsInfo = {'&' + encounter['base_label'] + '_RockSmashMonsInfo' if 'rock_smash_mons' in encounter else 'NULL'},")
            lines.append(f"        .fishingMonsInfo = {'&' + encounter['base_label'] + '_FishingMonsInfo' if 'fishing_mons' in encounter else 'NULL'},")
            lines.append("    },")
            if guarded:
                maybe_emit_game_guard(lines, encounter["base_label"], False)
        lines.extend([
            "    {",
            "        .mapGroup = MAP_GROUP(MAP_UNDEFINED),",
            "        .mapNum = MAP_NUM(MAP_UNDEFINED),",
            "        .landMonsInfo = NULL,",
            "        .waterMonsInfo = NULL,",
            "        .rockSmashMonsInfo = NULL,",
            "        .fishingMonsInfo = NULL,",
            "    },",
            "};",
            "",
        ])

    lines.extend([
        "#endif",
        "",
    ])
    return "\n".join(lines)


def main() -> int:
    if len(sys.argv) != 3:
        raise SystemExit("usage: generate_portable_region_map_overrides.py <root_dir> <generated_dir>")

    root_dir = Path(sys.argv[1]).resolve()
    generated_dir = Path(sys.argv[2]).resolve()
    map_sections = load_map_sections(root_dir)
    wild_encounter_groups = load_wild_encounter_groups(root_dir)
    charmap = load_charmap(root_dir)

    write_text(
        generated_dir / "include" / "constants" / "region_map_sections.h",
        generate_region_map_sections_header(map_sections),
    )
    write_text(
        generated_dir / "src" / "data" / "region_map" / "region_map_entry_strings.h",
        generate_region_map_strings_header(map_sections, charmap),
    )
    write_text(
        generated_dir / "src" / "data" / "region_map" / "region_map_entries.h",
        generate_region_map_entries_header(map_sections),
    )
    write_text(
        generated_dir / "include" / "constants" / "map_groups_aliases.h",
        generate_map_group_aliases_header(root_dir, wild_encounter_groups),
    )
    write_text(
        generated_dir / "src" / "data" / "wild_encounters.h",
        generate_wild_encounters_header(wild_encounter_groups),
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

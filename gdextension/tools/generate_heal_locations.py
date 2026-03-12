#!/usr/bin/env python3
"""Generate heal_locations.h from heal_locations.json"""

import json
from pathlib import Path

def generate_heal_locations():
    json_path = Path("D:/Godot/pokefirered-master/src/data/heal_locations.json")
    output_path = Path("D:/Godot/pokefirered-master/src/data/heal_locations.h")
    
    with open(json_path, 'r') as f:
        data = json.load(f)
    
    heal_locations = data['heal_locations']
    
    lines = [
        "#ifndef GUARD_DATA_HEAL_LOCATIONS_H",
        "#define GUARD_DATA_HEAL_LOCATIONS_H",
        "",
        "// Auto-generated from heal_locations.json",
        "",
        "#include \"heal_location.h\"",
        "#include \"constants/maps.h\"",
        "#include \"constants/map_event_ids.h\"",
        "",
        "#define HEAL_LOCATION_NONE 0",
        "",
    ]
    
    # Generate HEAL_LOCATION_* enum
    for i, loc in enumerate(heal_locations, 1):
        lines.append(f"#define {loc['id']} {i}")
    
    lines.append("")
    lines.append(f"#define HEAL_LOCATION_COUNT {len(heal_locations)}")
    lines.append("")
    
    # Generate sHealLocations array
    lines.append("static const struct HealLocation sHealLocations[] = {")
    for loc in heal_locations:
        map_name = loc['map']
        lines.append(f"    {{{map_name}, {loc['x']}, {loc['y']}}},")
    lines.append("};")
    lines.append("")
    
    # Generate sWhiteoutRespawnHealCenterMapIdxs array
    lines.append("static const u16 sWhiteoutRespawnHealCenterMapIdxs[HEAL_LOCATION_COUNT][2] = {")
    for loc in heal_locations:
        respawn_map = loc['respawn_map']
        lines.append(f"    {{MAP_GROUP({respawn_map}), MAP_NUM({respawn_map})}},")
    lines.append("};")
    lines.append("")
    
    # Generate sWhiteoutRespawnHealerNpcIds array
    lines.append("static const u8 sWhiteoutRespawnHealerNpcIds[HEAL_LOCATION_COUNT] = {")
    for loc in heal_locations:
        lines.append(f"    {loc['respawn_npc']},")
    lines.append("};")
    lines.append("")
    
    lines.append("#endif // GUARD_DATA_HEAL_LOCATIONS_H")
    lines.append("")
    
    with open(output_path, 'w') as f:
        f.write('\n'.join(lines))
    
    print(f"Generated {output_path}")
    print(f"  Heal locations: {len(heal_locations)}")

if __name__ == "__main__":
    generate_heal_locations()

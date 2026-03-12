#!/usr/bin/env python3

import json
from pathlib import Path
import struct
import sys


# Source-of-truth inputs:
# - data/layouts/layouts.json plus per-layout border/map binaries
# - data/maps/map_groups.json plus per-map map.json files
# Emitted output:
# - portable C for layouts, events, connections, map headers, gMapLayouts, gMapGroups


def write_if_changed(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists():
        old_text = path.read_text(encoding='utf-8')
        if old_text == text:
            return
    path.write_text(text, encoding='utf-8', newline='\n')


def require_fields(record, fields, context):
    missing = [field for field in fields if field not in record]
    if missing:
        raise ValueError(f'{context}: missing field(s): {", ".join(missing)}')


def read_json(path):
    return json.loads(path.read_text(encoding='utf-8'))


def load_layouts(root_dir):
    data = read_json(root_dir / 'data' / 'layouts' / 'layouts.json')['layouts']
    layout_ids = {}
    for index, layout in enumerate(data, start=1):
        if 'id' in layout:
            require_fields(
                layout,
                ('id', 'name', 'width', 'height', 'border_width', 'border_height', 'primary_tileset', 'secondary_tileset', 'border_filepath', 'blockdata_filepath'),
                f'layout slot {index}',
            )
            layout_ids[layout['id']] = index
    return data, layout_ids


def load_maps(root_dir):
    maps = {}
    for path in sorted((root_dir / 'data' / 'maps').glob('*/map.json')):
        data = read_json(path)
        require_fields(
            data,
            ('id', 'name', 'layout', 'music', 'region_map_section', 'requires_flash', 'weather', 'map_type', 'allow_cycling', 'allow_escaping', 'allow_running', 'show_map_name', 'floor_number', 'battle_scene'),
            f'map file {path.as_posix()}',
        )
        maps[data['name']] = data
    return maps


def read_u16_bin(path):
    data = path.read_bytes()
    if len(data) % 2 != 0:
        raise ValueError(f'odd-sized u16 binary: {path.as_posix()}')
    return struct.unpack('<{}H'.format(len(data) // 2), data)


def c_bool(value):
    return 'TRUE' if value else 'FALSE'


def c_script_ptr(value):
    if value in (None, '0x0', 'NULL'):
        return 'NULL'
    return value


def c_tileset_ptr(value):
    if value == 'NULL':
        return 'NULL'
    return f'&{value}'


def build_map_indices(map_groups, maps):
    indices = {}
    for group_index, group_name in enumerate(map_groups['group_order']):
        for map_index, map_name in enumerate(map_groups[group_name]):
            if map_name not in maps:
                raise ValueError(f'map_groups references unknown map name: {map_name}')
            map_id = maps[map_name]['id']
            indices[map_id] = (group_index, map_index)
    return indices


def target_group(target_map, map_indices):
    if target_map in map_indices:
        return str(map_indices[target_map][0])
    return f'MAP_GROUP({target_map})'


def target_num(target_map, map_indices):
    if target_map in map_indices:
        return str(map_indices[target_map][1])
    return f'MAP_NUM({target_map})'


def emit_u16_array(out, name, values):
    out.append(f'static const u16 {name}[] = {{')
    for value in values:
        out.append(f'    0x{value:04X},')
    out.append('};')
    out.append('')


def append_object_events(out, map_name, events, map_indices):
    label = f's{map_name}_ObjectEvents'
    out.append(f'static const struct ObjectEventTemplate {label}[] = {{')
    for index, event in enumerate(events, start=1):
        require_fields(event, ('type', 'graphics_id', 'x', 'y'), f'{map_name} object_event[{index}]')
        if event['type'] == 'clone':
            require_fields(event, ('target_local_id', 'target_map'), f'{map_name} clone_event[{index}]')
            out.extend([
                '    {',
                f"        .localId = {event.get('local_id', 0)},",
                f"        .graphicsId = {event['graphics_id']},",
                '        .kind = OBJ_KIND_CLONE,',
                f"        .x = {event['x']},",
                f"        .y = {event['y']},",
                '        .objUnion.clone = {',
                f"            .targetLocalId = {event['target_local_id']},",
                f"            .targetMapNum = {target_num(event['target_map'], map_indices)},",
                f"            .targetMapGroup = {target_group(event['target_map'], map_indices)},",
                '        },',
                '        .script = NULL,',
                '        .flagId = 0,',
                '    },',
            ])
        else:
            require_fields(event, ('elevation', 'movement_type', 'movement_range_x', 'movement_range_y', 'trainer_type', 'trainer_sight_or_berry_tree_id', 'script', 'flag'), f'{map_name} object_event[{index}]')
            out.extend([
                '    {',
                f"        .localId = {event.get('local_id', 0)},",
                f"        .graphicsId = {event['graphics_id']},",
                '        .kind = OBJ_KIND_NORMAL,',
                f"        .x = {event['x']},",
                f"        .y = {event['y']},",
                '        .objUnion.normal = {',
                f"            .elevation = {event['elevation']},",
                f"            .movementType = {event['movement_type']},",
                f"            .movementRangeX = {event['movement_range_x']},",
                f"            .movementRangeY = {event['movement_range_y']},",
                f"            .trainerType = {event['trainer_type']},",
                f"            .trainerRange_berryTreeId = {event['trainer_sight_or_berry_tree_id']},",
                '        },',
                f"        .script = {c_script_ptr(event['script'])},",
                f"        .flagId = {event['flag']},",
                '    },',
            ])
    out.append('};')
    out.append('')
    return label


def append_warp_events(out, map_name, events, map_indices):
    label = f's{map_name}_WarpEvents'
    out.append(f'static const struct WarpEvent {label}[] = {{')
    for index, event in enumerate(events, start=1):
        require_fields(event, ('x', 'y', 'elevation', 'dest_warp_id', 'dest_map'), f'{map_name} warp_event[{index}]')
        out.extend([
            '    {',
            f"        .x = {event['x']},",
            f"        .y = {event['y']},",
            f"        .elevation = {event['elevation']},",
            f"        .warpId = {event['dest_warp_id']},",
            f"        .mapNum = {target_num(event['dest_map'], map_indices)},",
            f"        .mapGroup = {target_group(event['dest_map'], map_indices)},",
            '    },',
        ])
    out.append('};')
    out.append('')
    return label


def append_coord_events(out, map_name, events):
    label = f's{map_name}_CoordEvents'
    out.append(f'static const struct CoordEvent {label}[] = {{')
    for index, event in enumerate(events, start=1):
        require_fields(event, ('type', 'x', 'y', 'elevation'), f'{map_name} coord_event[{index}]')
        if event['type'] == 'weather':
            require_fields(event, ('weather',), f'{map_name} coord_weather_event[{index}]')
            var = event['weather']
            index = 0
            script = 'NULL'
        else:
            require_fields(event, ('var', 'var_value', 'script'), f'{map_name} coord_event[{index}]')
            var = event['var']
            index = event['var_value']
            script = c_script_ptr(event['script'])
        out.extend([
            '    {',
            f"        .x = {event['x']},",
            f"        .y = {event['y']},",
            f"        .elevation = {event['elevation']},",
            f"        .trigger = {var},",
            f"        .index = {index},",
            f"        .script = {script},",
            '    },',
        ])
    out.append('};')
    out.append('')
    return label


def append_bg_events(out, map_name, events):
    label = f's{map_name}_BgEvents'
    out.append(f'static const struct BgEvent {label}[] = {{')
    for index, event in enumerate(events, start=1):
        require_fields(event, ('type', 'x', 'y', 'elevation'), f'{map_name} bg_event[{index}]')
        out.extend([
            '    {',
            f"        .x = {event['x']},",
            f"        .y = {event['y']},",
            f"        .elevation = {event['elevation']},",
        ])
        if event['type'] == 'hidden_item':
            require_fields(event, ('item', 'flag', 'quantity', 'underfoot'), f'{map_name} hidden_item[{index}]')
            hidden = (
                f"(({event['item']}) << HIDDEN_ITEM_ITEM_SHIFT) | "
                f"((({event['flag']}) - FLAG_HIDDEN_ITEMS_START) << HIDDEN_ITEM_FLAG_SHIFT) | "
                f"(({event['quantity']}) << HIDDEN_ITEM_QUANTITY_SHIFT) | "
                f"(({1 if event['underfoot'] else 0}) << HIDDEN_ITEM_UNDERFOOT_SHIFT)"
            )
            out.extend([
                '        .kind = BG_EVENT_HIDDEN_ITEM,',
                f'        .bgUnion.hiddenItem = {hidden},',
            ])
        elif event['type'] == 'secret_base':
            require_fields(event, ('secret_base_id',), f'{map_name} secret_base[{index}]')
            out.extend([
                '        .kind = BG_EVENT_SECRET_BASE,',
                f"        .bgUnion.hiddenItem = {event['secret_base_id']},",
            ])
        else:
            require_fields(event, ('player_facing_dir', 'script'), f'{map_name} sign_event[{index}]')
            out.extend([
                f"        .kind = {event['player_facing_dir']},",
                f"        .bgUnion.script = {c_script_ptr(event['script'])},",
            ])
        out.append('    },')
    out.append('};')
    out.append('')
    return label


def append_map_connections(out, map_name, connections, map_indices):
    list_label = f's{map_name}_ConnectionsList'
    data_label = f's{map_name}_Connections'
    direction_map = {
        'up': 'CONNECTION_NORTH',
        'down': 'CONNECTION_SOUTH',
        'left': 'CONNECTION_WEST',
        'right': 'CONNECTION_EAST',
        'dive': 'CONNECTION_DIVE',
        'emerge': 'CONNECTION_EMERGE',
    }
    out.append(f'static const struct MapConnection {list_label}[] = {{')
    for index, connection in enumerate(connections, start=1):
        require_fields(connection, ('direction', 'offset', 'map'), f'{map_name} connection[{index}]')
        if connection['direction'] not in direction_map:
            raise ValueError(f'{map_name} connection[{index}]: unsupported direction: {connection["direction"]}')
        out.extend([
            '    {',
            f"        .direction = {direction_map[connection['direction']]},",
            f"        .offset = {connection['offset']},",
            f"        .mapGroup = {target_group(connection['map'], map_indices)},",
            f"        .mapNum = {target_num(connection['map'], map_indices)},",
            '    },',
        ])
    out.append('};')
    out.append(f'static const struct MapConnections {data_label} = {{')
    out.append(f'    .count = {len(connections)},')
    out.append(f'    .connections = {list_label},')
    out.append('};')
    out.append('')
    return data_label


def generate_source(root_dir):
    layouts, layout_ids = load_layouts(root_dir)
    maps = load_maps(root_dir)
    map_groups = read_json(root_dir / 'data' / 'maps' / 'map_groups.json')
    map_indices = build_map_indices(map_groups, maps)

    out = [
        '#include "global.h"',
        '#include "fieldmap.h"',
        '#include "constants/event_bg.h"',
        '#include "constants/event_object_movement.h"',
        '#include "constants/event_objects.h"',
        '#include "constants/flags.h"',
        '#include "constants/items.h"',
        '#include "constants/map_types.h"',
        '#include "constants/maps.h"',
        '#include "constants/region_map_sections.h"',
        '#include "constants/songs.h"',
        '#include "constants/trainer_types.h"',
        '#include "constants/vars.h"',
        '#include "constants/weather.h"',
        '',
    ]

    tilesets = set()
    scripts = set()

    for layout in layouts:
        if 'id' not in layout:
            continue
        tilesets.add(layout['primary_tileset'])
        tilesets.add(layout['secondary_tileset'])

    for map_data in maps.values():
        scripts.add(f"{map_data['name']}_MapScripts")
        for event in map_data.get('object_events', []):
            if event['type'] == 'object' and event['script'] not in ('0x0', 'NULL'):
                scripts.add(event['script'])
        for event in map_data.get('coord_events', []):
            if event['type'] == 'trigger' and event['script'] not in ('0x0', 'NULL'):
                scripts.add(event['script'])
        for event in map_data.get('bg_events', []):
            if event['type'] == 'sign' and event['script'] not in ('0x0', 'NULL'):
                scripts.add(event['script'])

    for tileset in sorted(tilesets):
        if tileset == 'NULL':
            continue
        out.append(f'extern const struct Tileset {tileset};')
    for script in sorted(scripts):
        out.append(f'extern const u8 {script}[];')
    out.append('')

    for layout in layouts:
        if 'id' not in layout:
            continue
        border_name = f"s{layout['name']}_Border"
        map_name = f"s{layout['name']}_Map"
        border_values = read_u16_bin(root_dir / Path(layout['border_filepath']))
        map_values = read_u16_bin(root_dir / Path(layout['blockdata_filepath']))
        emit_u16_array(out, border_name, border_values)
        emit_u16_array(out, map_name, map_values)
        out.append(f'const struct MapLayout {layout["name"]} = {{')
        out.append(f'    .width = {layout["width"]},')
        out.append(f'    .height = {layout["height"]},')
        out.append(f'    .border = {border_name},')
        out.append(f'    .map = {map_name},')
        out.append(f'    .primaryTileset = {c_tileset_ptr(layout["primary_tileset"])},')
        out.append(f'    .secondaryTileset = {c_tileset_ptr(layout["secondary_tileset"])},')
        out.append(f'    .borderWidth = {layout["border_width"]},')
        out.append(f'    .borderHeight = {layout["border_height"]},')
        out.append('};')
        out.append('')

    for map_name in sorted(maps):
        map_data = maps[map_name]
        object_events = map_data.get('object_events', [])
        warp_events = map_data.get('warp_events', [])
        coord_events = map_data.get('coord_events', [])
        bg_events = map_data.get('bg_events', [])
        object_label = append_object_events(out, map_name, object_events, map_indices) if object_events else 'NULL'
        warp_label = append_warp_events(out, map_name, warp_events, map_indices) if warp_events else 'NULL'
        coord_label = append_coord_events(out, map_name, coord_events) if coord_events else 'NULL'
        bg_label = append_bg_events(out, map_name, bg_events) if bg_events else 'NULL'
        events_label = f's{map_name}_MapEvents'
        out.append(f'static const struct MapEvents {events_label} = {{')
        out.append(f'    .objectEventCount = {len(object_events)},')
        out.append(f'    .warpCount = {len(warp_events)},')
        out.append(f'    .coordEventCount = {len(coord_events)},')
        out.append(f'    .bgEventCount = {len(bg_events)},')
        out.append(f'    .objectEvents = {object_label},')
        out.append(f'    .warps = {warp_label},')
        out.append(f'    .coordEvents = {coord_label},')
        out.append(f'    .bgEvents = {bg_label},')
        out.append('};')
        out.append('')

        if map_data.get('connections'):
            connections_label = append_map_connections(out, map_name, map_data['connections'], map_indices)
        else:
            connections_label = 'NULL'

        layout_name = next((layout['name'] for layout in layouts if layout.get('id') == map_data['layout']), None)
        if layout_name is None:
            raise ValueError(f'{map_name}: unknown layout id {map_data["layout"]}')
        out.append(f'const struct MapHeader {map_name} = {{')
        out.append(f'    .mapLayout = &{layout_name},')
        out.append(f'    .events = &{events_label},')
        out.append(f'    .mapScripts = {map_name}_MapScripts,')
        out.append(f'    .connections = {"&" + connections_label if connections_label != "NULL" else "NULL"},')
        out.append(f'    .music = {map_data["music"]},')
        out.append(f'    .mapLayoutId = {layout_ids[map_data["layout"]]},')
        out.append(f'    .regionMapSectionId = {map_data["region_map_section"]},')
        out.append(f'    .cave = {c_bool(map_data["requires_flash"])},')
        out.append(f'    .weather = {map_data["weather"]},')
        out.append(f'    .mapType = {map_data["map_type"]},')
        out.append(f'    .bikingAllowed = {c_bool(map_data["allow_cycling"])},')
        out.append(f'    .allowEscaping = {1 if map_data["allow_escaping"] else 0},')
        out.append(f'    .allowRunning = {1 if map_data["allow_running"] else 0},')
        out.append(f'    .showMapName = {1 if map_data["show_map_name"] else 0},')
        out.append(f'    .floorNum = {map_data["floor_number"]},')
        out.append(f'    .battleType = {map_data["battle_scene"]},')
        out.append('};')
        out.append('')

    out.append('const struct MapLayout *gMapLayouts[] = {')
    for layout in layouts:
        if 'id' not in layout:
            out.append('    NULL,')
        else:
            out.append(f'    &{layout["name"]},')
    out.append('};')
    out.append('')

    for group_name in map_groups['group_order']:
        maps_in_group = map_groups[group_name]
        local_name = f's{group_name}'
        out.append(f'static const struct MapHeader *const {local_name}[] = {{')
        for map_name in maps_in_group:
            out.append(f'    &{map_name},')
        out.append('};')
        out.append('')

    out.append('const struct MapHeader *const *gMapGroups[] = {')
    for group_name in map_groups['group_order']:
        out.append(f'    s{group_name},')
    out.append('};')
    out.append('')

    return '\n'.join(out)


def main(argv):
    if len(argv) != 3:
        raise SystemExit('usage: generate_portable_map_data.py <root_dir> <generated_dir>')

    root_dir = Path(argv[1]).resolve()
    generated_dir = Path(argv[2]).resolve()
    out_path = generated_dir / 'src' / 'data' / 'map_data_portable.c'
    write_if_changed(out_path, generate_source(root_dir))


if __name__ == '__main__':
    main(sys.argv)

#!/usr/bin/env python3
"""
generate_map_aliases.py - Generate map constant aliases

Creates aliases to map between CamelCase directory names and
underscore-separated constant names used in the code.
"""

import re
from pathlib import Path

def generate_aliases():
    # Read the generated map_groups.h
    header_path = Path("D:/Godot/pokefirered-master/include/constants/map_groups.h")
    
    with open(header_path, 'r') as f:
        content = f.read()
    
    # Find all MAP_ definitions
    pattern = r'#define (MAP_\w+) (0x[0-9A-F]+)'
    matches = re.findall(pattern, content)
    
    aliases = []
    
    for const_name, value in matches:
        # Convert MAP_CELADONCITY_GYM to MAP_CELADON_CITY_GYM format
        # Insert underscores before capital letters (except the first one after MAP_)
        name_part = const_name[4:]  # Remove MAP_
        
        # Insert _ between lowercase-uppercase transitions and after common words
        # e.g. CELADONCITY -> CELADON_CITY
        # e.g. POKEMONTOWER -> POKEMON_TOWER
        
        # Common patterns to fix
        replacements = [
            (r'CELADONCITY', 'CELADON_CITY'),
            (r'CERULEANCITY', 'CERULEAN_CITY'),
            (r'CINNABARISLAND', 'CINNABAR_ISLAND'),
            (r'DIGLETTSCAVE', 'DIGLETTS_CAVE'),
            (r'FUCHSIACITY', 'FUCHSIA_CITY'),
            (r'LAVENDERTOWN', 'LAVENDER_TOWN'),
            (r'PALLETTOWN', 'PALLET_TOWN'),
            (r'PEWTERCITY', 'PEWTER_CITY'),
            (r'ROCKETHIDEOUT', 'ROCKET_HIDEOUT'),
            (r'SAFFRONCITY', 'SAFFRON_CITY'),
            (r'SEAFOAMISLANDS', 'SEAFOAM_ISLANDS'),
            (r'TRAINERTOWER', 'TRAINER_TOWER'),
            (r'UNDERGROUNDPATH', 'UNDERGROUND_PATH'),
            (r'VIRIDIANCITY', 'VIRIDIAN_CITY'),
            (r'VIRIDIANFOREST', 'VIRIDIAN_FOREST'),
            (r'CERULEANCAVE', 'CERULEAN_CAVE'),
            (r'VICTORYROAD', 'VICTORY_ROAD'),
            (r'POKEMONTOWER', 'POKEMON_TOWER'),
            (r'POKEMONMANSION', 'POKEMON_MANSION'),
            (r'POWERPLANT', 'POWER_PLANT'),
            (r'MTEMBER', 'MT_EMBER'),
            (r'MOON', 'MT_MOON'),
            (r'CINNABAR', 'CINNABAR'),
            (r'FOURISLAND', 'FOUR_ISLAND'),
            (r'FIVEISLAND', 'FIVE_ISLAND'),
            (r'SIXISLAND', 'SIX_ISLAND'),
            (r'SEVENISLAND', 'SEVEN_ISLAND'),
            (r'ONEISLAND', 'ONE_ISLAND'),
            (r'TWOISLAND', 'TWO_ISLAND'),
            (r'THREEISLAND', 'THREE_ISLAND'),
            (r'INDIGOPLATEAU', 'INDIGO_PLATEAU'),
            (r'SSAQUA', 'SS_AQUA'),
            (r'SSANNE', 'SS_ANNE'),
            (r'SILPHCO', 'SILPH_CO'),
            (r'GAMECORNER', 'GAME_CORNER'),
            (r'POKEMONCENTER', 'POKEMON_CENTER'),
            (r'WARDENSHOUSE', 'WARDENS_HOUSE'),
            (r'FIGHTINGDOJO', 'FIGHTING_DOJO'),
            (r'CONDOSHOP', 'CONDO_SHOP'),
            (r'CONDORESTAURANT', 'CONDO_RESTAURANT'),
            (r'CONDOROOF', 'CONDO_ROOF'),
            (r'DEPARTMENTSTORE', 'DEPARTMENT_STORE'),
            (r'CONDOKITCHEN', 'CONDO_KITCHEN'),
            (r'ROCKETWAREHOUSE', 'ROCKET_WAREHOUSE'),
            (r'LOSTCAVERN', 'LOST_CAVERN'),
            (r'BIRTHISLAND', 'BIRTH_ISLAND'),
            (r'NAVELROCK', 'NAVEL_ROCK'),
            (r'BATTLECOLOSSEUM', 'BATTLE_COLOSSEUM'),
            (r'CABLECLUB', 'CABLE_CLUB'),
            (r'TRADECENTER', 'TRADE_CENTER'),
            (r'RECORDCENTER', 'RECORD_CENTER'),
            (r'LINKLOBBY', 'LINK_LOBBY'),
            (r'JOYFULGAMECORNER', 'JOYFUL_GAME_CORNER'),
            (r'CAPEBRINK', 'CAPE_BRINK'),
            (r'ICEFALLCAVE', 'ICEFALL_CAVE'),
            (r'PATTERNBUSH', 'PATTERN_BUSH'),
            (r'RUINVALLEY', 'RUIN_VALLEY'),
            (r'DOTTEDHOLE', 'DOTTED_HOLE'),
            (r'LOSTCAVERN', 'LOST_CAVERN'),
            (r'TANOBYKEY', 'TANOBY_KEY'),
            (r'TANOBYRUINS', 'TANOBY_RUINS'),
            (r'MONEANCHAMBER', 'MONEAN_CHAMBER'),
            (r'LIPIFOCHAMBER', 'LIPIFO_CHAMBER'),
            (r'WEVIPETCHAMBER', 'WEVIPET_CHAMBER'),
            (r'ALPHISCHAMBER', 'ALPHIS_CHAMBER'),
            (r'KALOSCHAMBER', 'KALOS_CHAMBER'),
            (r'ISLSHIMMERCHAMBER', 'ISL_SHIMMER_CHAMBER'),
            (r'VIAPURCHASEAMBER', 'VIAPUR_CHASE_CHAMBER'),
            (r'VIARESUMECHAMBER', 'VIARESUME_CHAMBER'),
            (r'MANGELICHAMBER', 'MANGELI_CHAMBER'),
            (r'OLVANTECHAMBER', 'OLVANTE_CHAMBER'),
            (r'HITODEMECHAMBER', 'HITODEME_CHAMBER'),
            (r'LIXPETCHAMBER', 'LIXPET_CHAMBER'),
            (r'PROFECTLAB', 'PROF_ECT_LAB'),
            (r'FAMECHECKER', 'FAME_CHECKER'),
            (r'HEALINGPLACE', 'HEALING_PLACE'),
            (r'ITEMFINDER', 'ITEM_FINDER'),
            (r'HALLWAY', 'HALL_WAY'),
            (r'HALLDOOR', 'HALL_DOOR'),
            (r'HALLDOOR2', 'HALL_DOOR2'),
            (r'DEPARTMENTSTOREELEVATOR', 'DEPARTMENT_STORE_ELEVATOR'),
            (r'POKEMONLAB', 'POKEMON_LAB'),
            (r'FANCLUB', 'FAN_CLUB'),
            (r'BIKESHOP', 'BIKE_SHOP'),
            (r'WARDENSHOUSE', 'WARDENS_HOUSE'),
            (r'GAMECORNERPRIZEROOM', 'GAME_CORNER_PRIZE_ROOM'),
        ]
        
        new_name = name_part
        for old, new in replacements:
            new_name = new_name.replace(old, new)
        
        if new_name != name_part:
            new_const_name = f"MAP_{new_name}"
            aliases.append(f"#define {new_const_name} {const_name}")
    
    return aliases

if __name__ == "__main__":
    aliases = generate_aliases()
    print(f"// Auto-generated map aliases ({len(aliases)} total)")
    print("\n".join(aliases))

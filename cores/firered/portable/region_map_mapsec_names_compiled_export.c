#include "global.h"

#ifndef PORTABLE
void region_map_mapsec_names_compiled_export_unused_(void) { }
#else
/*
 * Emits gRegionMapMapsecNames_Compiled for firered_portable_rom_region_map_mapsec_names.c.
 * src/region_map.c usually includes pret jsonproc data/region_map/region_map_entries.h first
 * (quoted-include directory of region_map.c), so that TU only has static sMapNames + aliases.
 * SDL/PORTABLE CPPPATH lists pokefirered_core/generated/src before repo src/, so the includes
 * below resolve to the portable generator output here.
 */
/* Pret jsonproc strings (names match portable-generated region_map_entries.h initializers). */
#include "../../../src/data/region_map/region_map_entry_strings.h"
#include "data/region_map/region_map_entries.h"
#endif

#ifndef GUARD_CONSTANTS_REGION_MAP_LAYOUT_DIMS_H
#define GUARD_CONSTANTS_REGION_MAP_LAYOUT_DIMS_H

/*
 * Dimensions of `sRegionMapSections_*[layer][y][x]` tensors in region_map.c.
 * ROM-backed packs and layout headers must stay in lockstep with these values.
 */
#define REGION_MAP_SECTION_GRID_WIDTH 22
#define REGION_MAP_SECTION_GRID_HEIGHT 15
#define REGION_MAP_SECTION_GRID_LAYERS 2

#define REGION_MAP_SECTION_GRID_BYTE_SIZE \
    ((unsigned)(REGION_MAP_SECTION_GRID_LAYERS) * (unsigned)(REGION_MAP_SECTION_GRID_HEIGHT) \
        * (unsigned)(REGION_MAP_SECTION_GRID_WIDTH))

#endif /* GUARD_CONSTANTS_REGION_MAP_LAYOUT_DIMS_H */

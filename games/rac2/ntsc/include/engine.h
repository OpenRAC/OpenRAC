#ifndef ENGINE_H
#define ENGINE_H

#include "common.h"
struct Moby; /* RAC1's moby record: games/rac1/pal/include/structs.h */

/*
 * Insomniac Engine Core Geometry Systems:
 * - tfrag: Terrain fragment geometry (static terrain rendering)
 * - tie:   Instanced environment structures
 * - shrub: Instanced foliage, grass, and decorative meshes
 * - moby:  Dynamic interactive entities (actors, projectiles, crates)
 */

typedef enum GeometryKind {
    GEOM_TFRAG = 0,
    GEOM_TIE   = 1,
    GEOM_SHRUB = 2,
    GEOM_MOBY  = 3
} GeometryKind;

/*
 * Collision System API:
 * Pill/capsule collision detection for Mobies.
 * Recovered from assertion strings in engine core (0x001E8890).
 */
s32 MB_CheckCollPill(struct Moby *moby);

/*
 * Camera collision primitive test:
 * Recovered from 0x001E7A50 (author initials RAR).
 */
s32 Camera_CollPrimTest(void *prim, void *grid);

#endif /* ENGINE_H */

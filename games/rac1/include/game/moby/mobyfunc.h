#ifndef GAME_MOBY_MOBYFUNC_H
#define GAME_MOBY_MOBYFUNC_H

/* Functions defined in src/game/moby/mobyfunc.c. */

#include "common.h"

extern void AttachManipulator(void *, int, void *);
extern void *CreateMoby(int);
extern void DeleteMoby(void *);
extern void DetachManipulator(void *, void *);
extern void DrawMobyList(int, int);
extern void DrawMobys(void);
extern void DrawMobysCleanUp(void);
extern void DrawMobysSetup(void);
extern void InitMobyClassDists(void);
extern void InitMobyInstance(void *, int);
extern float MobyGetFrameValue(void *);
extern void MobyTransformScaledVertex(void *, int, void *);
extern void PatchMobyGifs(void);
extern void RestoreMobyClassDists(void);
extern void StashMobyClassDists(void);
extern int find_or_allocate_id_slot(int);
extern void pack_render_command_fields(void *, int, int, int, int);
extern void prune_moby_references(void);
extern void transform_scaled_vertex_batch(void *, int, void *, void *);
extern void update_moby_animation_state(void *);
extern void update_resident_packed_direction(void);

#endif /* GAME_MOBY_MOBYFUNC_H */

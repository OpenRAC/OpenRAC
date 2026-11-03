#ifndef OVERLAYS_SHARED_GOLD_BOLTS_AND_TELEPORTERS_H
#define OVERLAYS_SHARED_GOLD_BOLTS_AND_TELEPORTERS_H

/* Functions defined in src/overlays/shared/gold_bolts_and_teleporters.c. */

#include "common.h"

extern void Moby751Draw(void);
extern void attach_cutscene_fixed_scale_manipulator(void);
extern void draw_moby_offset_textured_quad(Moby *);
extern void set_camera_5_track_acceleration_yz(float, float);
extern struct Moby *spawn_debris_moby(Vec4f *, Vec4f *, s32, s32, s32, f32, f32, f32, f32);
extern char *spawn_sandmouse_projectile(char *, char *, float, float, float, float);
extern void update_teleporter_child_transforms(char *);

#endif /* OVERLAYS_SHARED_GOLD_BOLTS_AND_TELEPORTERS_H */

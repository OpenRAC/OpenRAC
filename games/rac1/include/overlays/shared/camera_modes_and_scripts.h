#ifndef OVERLAYS_SHARED_CAMERA_MODES_AND_SCRIPTS_H
#define OVERLAYS_SHARED_CAMERA_MODES_AND_SCRIPTS_H

/* Functions defined in src/overlays/shared/camera_modes_and_scripts.c. */

#include "common.h"

extern void CameraScript(void *, void *, int, int, int);
extern void CameraScript2(int);
extern int initialize_mode6_orbit_camera(void *);
extern void select_camera_mode_by_hero_alignment_and_distance(int);
extern void set_camera_position(void *);
extern void set_camera_rotation(void *);
extern int update_orbit_camera_from_target_moby(void *, void *);

#endif /* OVERLAYS_SHARED_CAMERA_MODES_AND_SCRIPTS_H */

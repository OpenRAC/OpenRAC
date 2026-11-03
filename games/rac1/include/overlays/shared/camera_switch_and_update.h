#ifndef OVERLAYS_SHARED_CAMERA_SWITCH_AND_UPDATE_H
#define OVERLAYS_SHARED_CAMERA_SWITCH_AND_UPDATE_H

/* Functions defined in src/overlays/shared/camera_switch_and_update.c. */

#include "common.h"

extern float Camera_SmoothAngle(void *, float, float, float, float, float);
extern void Camera_UpdateFrame(void);
extern void SwitchCamera(void *);
extern int UpdateAllCamerasLvl(void);
extern void compute_position_from_spherical_offset(void *, void *, void *);
extern void compute_spherical_offset_from_positions(void *, void *, void *);
extern void *find_camera_record_by_id(int);
extern void init_camera_system(void);
extern void initialize_camera_record_handoff_defaults(void);
extern void resolve_camera_movement_collisions(void *, float);

#endif /* OVERLAYS_SHARED_CAMERA_SWITCH_AND_UPDATE_H */

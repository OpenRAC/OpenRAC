#ifndef GAME_GAMEPLAY_CAMERA_H
#define GAME_GAMEPLAY_CAMERA_H

/* Functions defined in src/game/gameplay/camera.c. */

#include "common.h"

extern void BackupCurrentCam(void);
extern float Cam_InterpValues(float, float, float *, float, float, float);
extern int Camera_ActivationCheckPriority(Camera *, Camera *);
extern void Camera_Pos2Polar3d(float *out, void *p0, void *p1, void *dir0, void *dir1, void *axis);
extern void Camera_handleCollWithHero(Camera *);
extern void Camera_runSetupToNewCam(Camera *);
extern void advance_timed_camera_control(void);
extern void apply_camera_shake(CameraShake *, int);
extern void restore_camera_vectors(void);
extern void save_camera_vectors(void);
extern void update_camera(void);
extern void update_camera_blend(Camera *);
extern void update_camera_tracking_state(void);
extern void update_camera_underwater_flag(void);

#endif /* GAME_GAMEPLAY_CAMERA_H */

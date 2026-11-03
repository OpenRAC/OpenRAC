#ifndef OVERLAYS_L13_GEMLIK_QWARK_STARFIGHTER_AND_FORCEFIELD_H
#define OVERLAYS_L13_GEMLIK_QWARK_STARFIGHTER_AND_FORCEFIELD_H

/* Functions defined in src/overlays/l13_gemlik/qwark_starfighter_and_forcefield.c. */

#include "common.h"

extern char *create_attached_rocket(struct Moby *, short);
extern void launch_attached_rocket(Moby *, char *, int, float);
extern char *spawn_gemlik_projectile(float, float *, float *, void *);
extern char *spawn_moby_joint_attachment(char *, int, int);

#endif /* OVERLAYS_L13_GEMLIK_QWARK_STARFIGHTER_AND_FORCEFIELD_H */

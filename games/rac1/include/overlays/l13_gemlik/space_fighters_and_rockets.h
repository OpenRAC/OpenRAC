#ifndef OVERLAYS_L13_GEMLIK_SPACE_FIGHTERS_AND_ROCKETS_H
#define OVERLAYS_L13_GEMLIK_SPACE_FIGHTERS_AND_ROCKETS_H

/* Functions defined in src/overlays/l13_gemlik/space_fighters_and_rockets.c. */

#include "common.h"

extern int advance_moby_along_selected_path(void *, void *, int, int);
extern void detach_moby_joint_attachment(void *, void *, int);
extern void emit_gated_moby_joint_effects(void *, void *);
extern void emit_randomized_moby_joint_effects(void *, void *);
extern void *fire_mine_83_projectile(void *, void *);
extern void fire_starfighter_homing_rocket(void *, void *);
extern void process_moby_attachment_damage(void *, void *);
extern int queue_starfighter_voice_line(void *, void *, int, int);
extern void *spawn_owner_linked_class_160_moby(void *);
extern void update_moby_joint_attachments(void *, void *);
extern int update_moby_path_following_away_from_hero(void *, void *, int, int);
extern void update_moby_spring_steering_and_banking(void *, void *, float, float);

#endif /* OVERLAYS_L13_GEMLIK_SPACE_FIGHTERS_AND_ROCKETS_H */

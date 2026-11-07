#ifndef GAME_SYSTEM_LOADERS_H
#define GAME_SYSTEM_LOADERS_H

/* Functions defined in src/game/system/loaders.c. */

#include "common.h"

extern void LoadHudBanks(void);
extern void ParseParticleTexs(void *, void *, void *, int);
extern void initialize_tfrag_render_data(void *, void *);
extern int load_level_chunk_from_disc(void);
extern void parse_space_scene_chunk(s32 index);
extern void register_moby_class(void *, void *, void *, int);
extern void register_object_render_class(void *, void *, void *, int);
extern void register_shrub_render_class(void *, void *, void *, void *, int);
extern void func_00203B18(void *, int);
extern void relocate_sky_definition(void *);
extern void select_world_object_resource_tables(s32, s32);
extern int service_level_archive_load(void);
extern void unpack_point_records(void *, int);
extern void update_world_object_animation(void);
extern int upload_mip_texture(int, void *);
extern void upload_texture_images(void *, int, void *);

#endif /* GAME_SYSTEM_LOADERS_H */

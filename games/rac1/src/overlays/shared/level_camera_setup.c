/* Starts at 0x001ED6D8 (was camera_001ED6D8.c). */
/* Shared level code: func_L00_001ED6D8 in a file of its own, built with -fno-force-mem
 * (config/file_cflags.txt). GCC 2.95 takes options per translation unit,
 * and the functions of the file it came from do not build with this one
 * (docs/BUILD_FIDELITY.md, "Flags"). */
#include "common.h"
#include "include_asm.h"
#include "camera.h"

extern CameraSystem player_camera;
extern char hero_globals_base[];
extern void vec_scale_to_length(void *, void *, float);
extern Camera gCameraSlotsLvl[];
extern int gCameraSlotUsedLvl[];
extern int gCamPostUpdCountLvl MACRO_ADDR;
extern int gCameraDefCountLvl MACRO_ADDR;
extern short follow_cam_turn_blend_weight;
extern CameraDef *camera_descriptors MACRO_ADDR;
extern void clear_u64_value(void *);
extern void init_camera_system(void);

/* Initialises the camera slot tables and the camera state from the hero's transform. */
void setup_level_cameras(void) {
    CameraSystem *g = &player_camera;
    CameraSystem *g1;
    CameraFollow *q;
    Vec4 *p1;
    char *base;
    int i;
    int k;

    g->underwater = 0;
    g->frames = 0;
    *(float *)&follow_cam_turn_blend_weight = 0.75f;
    gCamPostUpdCountLvl = 0;
    for (k = 0; k < 48; k++) {
        gCameraSlotsLvl[k].hard_cut = 0;
        gCameraSlotUsedLvl[k] = 0;
    }
    g1 = &player_camera;
    base = hero_globals_base + 0xE1D;
    q = &g1->follow;
    p1 = &g1->follow.up;
    g1->previous = 0;
    g1->blend.phase = 0;
    vec_scale_to_length(p1, (char *)(*(int *)(base + 0x2080) + 0xE0), 1.0f);
    qcopy(&g1->follow.up_target, p1);
    qcopy(&g1->follow.up_target_prev, p1);
    clear_u64_value(g1->follow.up_vel);
    qcopy(&g1->follow.hero_prev, base + 0x80);
    {
        float f0 = *(float *)(base + 0x98);
        float f1 = *(float *)(base + 0x88);
        q->hero_coll_moby = 0;
        q->unkCC = 0;
        q->coll_bits = 0;
        q->fade_rate = 0;
        q->unkD0 = 0;
        q->yaw_hist[4] = f0;
        q->yaw_hist[3] = f0;
        q->yaw_hist[2] = f0;
        q->yaw_hist[1] = f0;
        q->ground_moby = 0;
        q->ground_dz = 0;
        q->yaw_hist[0] = f0;
        q->ground_z = f1;
    }
    i = 0;
    if (gCameraDefCountLvl > 0) {
        do {
            int off = i * 32;
            CameraDef *e = (CameraDef *)((char *)camera_descriptors + off);
            CameraInfo *r = e->info;
            Camera *el = &gCameraSlotsLvl[i];
            int h;
            el->def_index = i;
            r->slot = el;
            el->hard_cut = 0;
            el->hit_wall = 0;
            el->snap = 0;
            el->unk82 = -1;
            el->unk80 = -1;
            el->handoff_state = 0;
            el->transition_state = 0;
            el->blend_time = -1.0f;
            el->priority = r->priority;
            el->activation = r->activation;
            h = e->id;
            gCameraSlotUsedLvl[i] = 1;
            el->id = h;
            el->type = find_camera_table_index_by_id((short)h);
            i++;
        } while (i < gCameraDefCountLvl);
    }
    init_camera_system();
    {
        CamBlend *b = &player_camera.blend;
        s32 *u = player_camera.unk380;
        b->phase = 0;
        u[0] = 0;
        u[3] = 0;
        u[1] = 0;
        u[4] = 0;
    }
}

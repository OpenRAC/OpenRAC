#include "common.h"
#include "callees.h"
#include "structs/records.h"
#include "structs/moby.h"
#include "structs/misc.h"
#include "asm_functions/executable.h"
#include "data/executable.h"
#include "game/moby/mobyutil.h"
#include "game/moby/mobyfunc.h"
#include "game/audio/music.h"

/*
 * update.cpp in the original source; text 0x238D88-0x239628.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/reference/DECOMP_PROGRESS.md. Compiled as C for now.
 */

extern char camera_position[];

extern void truncate_float_to_s32(float);

/* Empty debug profiler hook. */
void DebugProfiler_Hook(void) {
}

#include "structs/vector.h"
typedef struct {
    char unk_000[0x190];
    float scale[2];
} ViewCtx;
typedef struct {
    int unk_00;
    int unk_04;
    int half_w;
    int half_h;
} ScreenExt;
extern ViewCtx view_context;
extern ScreenExt g_screen_extent;

void project_graphics_bounds(Vec4 *a, Vec4 *b, int *w, int *h, int *x, int *y) {
    Vec4 p;
    Vec4 o;
    Vec4 *q;
    float sx;
    float sy;
    char *cam;

    q = &o;
    p.q = a->q;
    q->q = b->q;
    cam = camera_position;
    FastVecSub(&p, &p, cam);
    FastVecSub(q, q, cam);
    cam = cam + (*(unsigned char *)x >> 8);
    FastVecScale(&p, &p, 1024.0f);
    FastVecScale(q, q, 1024.0f);
    q->f[3] = 1.0f;
    p.f[3] = 1.0f;
    transform_vector(&p, &p, cam - 0x40);
    transform_vector(q, q, cam - 0x40);
    p.f[0] *= 1.0f / p.f[3];
    p.f[1] *= 1.0f / p.f[3];
    q->f[0] *= 1.0f / q->f[3];
    q->f[1] *= 1.0f / q->f[3];
    sx = view_context.scale[0];
    sy = view_context.scale[1];
    p.f[0] *= sx;
    p.f[1] *= sy;
    q->f[0] *= sx;
    q->f[1] *= sy;
    q = (Vec4 *)((char *)&o + (*(unsigned char *)x >> 8));
    *x = ((int (*)(float))truncate_float_to_s32)(p.f[0] * 0.25f + (float)g_screen_extent.half_w);
    *y = ((int (*)(float))truncate_float_to_s32)(p.f[1] * 0.25f + (float)g_screen_extent.half_h);
    *w = ((int (*)(float))truncate_float_to_s32)((o.f[0] - p.f[0]) * 0.25f);
    *h = ((int (*)(float))truncate_float_to_s32)((q->f[1] - p.f[1]) * 0.25f);
}

void project_graphics_bounds_float(Vec4 *a, Vec4 *b, float *w, float *h, float *x, float *y) {
    Vec4 p;
    Vec4 o;
    Vec4 *q;
    float sx;
    float sy;
    char *cam;

    q = &o;
    p.q = a->q;
    q->q = b->q;
    cam = camera_position;
    FastVecSub(&p, &p, cam);
    FastVecSub(q, q, cam);
    cam = cam + (*(unsigned char *)x >> 8);
    FastVecScale(&p, &p, 1024.0f);
    FastVecScale(q, q, 1024.0f);
    q->f[3] = 1.0f;
    p.f[3] = 1.0f;
    transform_vector(&p, &p, cam - 0x40);
    transform_vector(q, q, cam - 0x40);
    p.f[0] *= 1.0f / p.f[3];
    p.f[1] *= 1.0f / p.f[3];
    q->f[0] *= 1.0f / q->f[3];
    q->f[1] *= 1.0f / q->f[3];
    sx = view_context.scale[0];
    sy = view_context.scale[1];
    p.f[0] *= sx;
    p.f[1] *= sy;
    q->f[0] *= sx;
    q = (Vec4 *)((char *)&o + (*(unsigned char *)x >> 8));
    q->f[1] *= sy;
    *x = p.f[0] * 0.25f + (float)g_screen_extent.half_w;
    *y = p.f[1] * 0.25f + (float)g_screen_extent.half_h;
    *w = (o.f[0] - p.f[0]) * 0.25f;
    q = (Vec4 *)((char *)q + (*(unsigned char *)x >> 8));
    *h = (q->f[1] - p.f[1]) * 0.25f;
}

LINKER_REMNANT("asm/remnants/text", func_00239180);

extern char str_scrolling_status_fmt[];

/* Formats arg0 into the scrolling status message and resets its offset. */
void set_scrolling_status_message(int arg0) {
    char *b = vendor_state;
    sprintf(*(int *)(b + 0x2C), str_scrolling_status_fmt, arg0);
    *(int *)(b + 0x44) = 0;
}

typedef struct {
    u8 pad0[0x34];
    s32 timer;
    s32 base;
    s32 pick;
} BossVars;
typedef struct {
    u8 pad0;
    u8 active;
    u8 pad2[0x1E];
    f32 x;
    f32 y;
    f32 z;
} Cam;
extern s32 queued_dialogue_id;
extern s16 dialogue_playback_phase NOT_SDA;
extern u8 D_0015EEB0[] MACRO_ADDR;
extern s32 resident_stream_resume_frames_s4 SDATA(resident_stream_resume_frames_s4);
extern s32 resident_stream_resume_frames_s5 SDATA(resident_stream_resume_frames_s5);
extern s32 resident_stream_resume_frames_s6 SDATA(resident_stream_resume_frames_s6);
extern s32 resident_dialogue_started MACRO_ADDR;
extern s32 resident_stream_pending MACRO_ADDR;
extern BossVars vendor_state_r __asm__("D_001E66C0");
extern s32 vendor_state_dialogue_pick;
extern Cam resident_cheat_scale_manip;

/* Per-frame state machine of one boss-like moby: keeps a manipulator node attached while gCheats is set,
   then steps the state byte at +0x20 through its animation and timer states.
   Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/gameplay/vendor/fun_00237ed0.c, FUN_00237ed0. */
void update_resident_dialogue_animation_state(Moby *m) {
    s32 r;
    s32 a;

    if (gCheats[0] != 0) {
        if (resident_cheat_scale_manip.active == 0) {
            AttachManipulator(m, 0, &resident_cheat_scale_manip);
        }
        resident_cheat_scale_manip.x = 2.37f;
        resident_cheat_scale_manip.y = 2.37f;
        resident_cheat_scale_manip.z = 2.37f;
    } else if (resident_cheat_scale_manip.active != 0) {
        DetachManipulator(m, &resident_cheat_scale_manip);
    }
    switch (m->state) {
        case 0:
            if (vendor_state_r.timer > scale_game_frame_count(600)) {
                r = random_integer_below(2);
                vendor_state_r.pick = r;
                queued_dialogue_id = (r * 3 + 1) * 6 + vendor_state_r.base + 10000;
                resident_dialogue_started = 1;
                resident_stream_pending = 0;
                if (m->prev_seq != 2) {
                    MobyBlendAnimation(m, 2, 0, scale_game_frame_count(0x12));
                }
                m->state = 12;
                vendor_state_r.timer = 0;
            }
            vendor_state_r.timer++;
            break;
        case 2:
            if (vendor_state_r.timer > scale_game_frame_count(600)) {
                if (m->prev_seq != 3) {
                    MobyBlendAnimation(m, 3, 0, scale_game_frame_count(0x12));
                }
                m->state = 3;
                vendor_state_r.timer = 0;
            }
            vendor_state_r.timer++;
            break;
        case 3:
            if (m->anim_flags & 2) {
                if (m->prev_seq != 0) {
                    MobyBlendAnimation(m, 0, 0, scale_game_frame_count(0x12));
                }
                m->state = 0;
            }
            break;
        case 4:
            if (IsFrameWithinWindow(m, ((&resident_stream_resume_frames_s4))[vendor_state_r.pick]) != 0 &&
                resident_stream_pending != 0) {
                resident_stream_pending = 0;
                Music_ContinueStreamIfReady();
            }
            if (m->anim_flags & 2) {
                if (m->prev_seq != 2) {
                    MobyBlendAnimation(m, 2, 0, scale_game_frame_count(0x12));
                }
                m->state = 2;
                vendor_state_r.timer = 0;
            }
            break;
        case 5:
            if (IsFrameWithinWindow(m, ((&resident_stream_resume_frames_s5))[vendor_state_r.pick]) != 0 &&
                resident_stream_pending != 0) {
                resident_stream_pending = 0;
                Music_ContinueStreamIfReady();
            }
            if (m->anim_flags & 2) {
                if (m->prev_seq != 2) {
                    MobyBlendAnimation(m, 2, 0, scale_game_frame_count(0x12));
                }
                m->state = 2;
                vendor_state_r.timer = 0;
            }
            break;
        case 6:
            if (IsFrameWithinWindow(m, ((&resident_stream_resume_frames_s6))[vendor_state_r.pick]) != 0 &&
                resident_stream_pending != 0) {
                resident_stream_pending = 0;
                Music_ContinueStreamIfReady();
            }
            if (m->anim_flags & 2) {
                if (m->prev_seq != 2) {
                    MobyBlendAnimation(m, 2, 0, scale_game_frame_count(0x12));
                }
                m->state = 2;
                vendor_state_r.timer = 0;
            }
            break;
        case 10:
            if (dialogue_playback_phase == 3 && resident_dialogue_started != 0) {
                resident_dialogue_started = 0;
                resident_stream_pending = 1;
            }
            if (m->anim_flags & 2) {
                a = vendor_state_dialogue_pick * 3 + 4;
                if (m->prev_seq != a) {
                    MobyBlendAnimation(m, a, 0, scale_game_frame_count(0xC));
                }
                m->state = 4;
            }
            break;
        case 12:
            if (dialogue_playback_phase == 3 && resident_dialogue_started != 0) {
                resident_stream_pending = 1;
                resident_dialogue_started = 0;
                a = vendor_state_dialogue_pick * 3 + 5;
                if (m->prev_seq != a) {
                    MobyBlendAnimation(m, a, 0, scale_game_frame_count(0x12));
                }
                m->state = 5;
            }
            break;
        case 11:
            if (dialogue_playback_phase == 3 && resident_dialogue_started != 0) {
                resident_stream_pending = 1;
                resident_dialogue_started = 0;
                a = vendor_state_dialogue_pick * 3 + 6;
                if (m->prev_seq != a) {
                    MobyBlendAnimation(m, a, 0, scale_game_frame_count(0x12));
                }
                m->state = 6;
            }
            break;
    }
}

LINKER_REMNANT("asm/remnants/text", func_00239610);

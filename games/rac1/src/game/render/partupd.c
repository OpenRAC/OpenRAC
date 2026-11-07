#include "common.h"
#include "callees.h"
#include "structs/records.h"
#include "structs/misc.h"
#include "game/moby/mobyutil.h"
#include "asm_functions/executable.h"

/*
 * partupd.cpp in the original source; text 0x219780-0x219C08.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/reference/DECOMP_PROGRESS.md. Compiled as C for now.
 */

typedef u32 u128 __attribute__((mode(TI), aligned(16)));
struct Sub {
    s32 a;
    s32 b;
    s32 c;
    s32 d;
    s16 h10;
    s16 h12;
    s16 h14;
    s16 h16;
    s16 h18;
};
struct Obj {
    u8 pad_0;
    u8 b1;
    u8 b2;
    u8 b3;
    s32 w4;
    u8 b8;
    u8 b9;
    s16 hA;
    f32 fC;
    u128 q10;
    struct Sub sub;
};
extern u8 *particle_texture_ptrs[];
extern u8 *default_spark_lookup[];

extern s32 truncate_float_to_s32(f32);

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/rendering/fun_00218888.c, FUN_00218888. */
struct Obj *PartType02Spawn(u128 *arg0, f32 *arg1, f32 *arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, s32 arg7,
                            s32 arg8) {
    struct Obj *o;
    struct Sub *s;
    f32 t0[4];
    f32 t1[4];

    o = create_part(2);
    if (o != 0) {
        qcopy(&o->q10, arg0);
        o->w4 = arg3;
        o->b9 = truncate_float_to_s32(4.0f) + 0x40;
        o->fC = 0.0f;
        o->b1 = 0;
        o->b3 = 0x44;
        o->b8 = truncate_float_to_s32(random_float_between(o->fC, 255.0f));
        if (arg8 == -1) {
            o->b2 = *default_spark_lookup[0];
        } else {
            o->b2 = *particle_texture_ptrs[(u16)arg8];
            if (arg8 & 0x10000) {
                o->b3 = 0x48;
            }
        }
        o->hA = arg5;
        s = &o->sub;
        s->h16 = truncate_float_to_s32(arg1[3] * 210000.0f / 1000.0f);
        s->h18 = truncate_float_to_s32(arg2[3] * 210000.0f / 1000.0f);
        pack_vector_xyz_with_shared_scale(arg1, &s->a);
        pack_vector_xyz_with_shared_scale(arg2, &s->b);
        load_display_text_resource_entry(t0, &s->a);
        load_display_text_resource_entry(t1, &s->b);
        s->c = arg3;
        s->d = arg4;
        s->h10 = arg5;
        s->h12 = arg6;
        s->h14 = arg7;
    }
    return o;
}

LINKER_REMNANT("asm/remnants/text", func_00219958);

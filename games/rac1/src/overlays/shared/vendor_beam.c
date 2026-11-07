/* Starts at 0x00303430 (was vendor_00303430.c). */
/* Level 14 code: draw_electric_beam in a file of its own, built with -fno-sched-spec
 * (config/file_cflags.txt). The functions of vendor_002B2A28.c do not build
 * with it (emit_moby_offset_particles changes), so it was another object in retail
 * (docs/reference/BUILD_FIDELITY.md, rule 2). */
#include "common.h"
#include "structs/moby.h"
#include "data/level_14.h"
#include "asm_functions/executable.h"
#include "game/math/fastfunc.h"
#include "asm_functions/shared_levels.h"

typedef struct Pkt {
    float corner[4][4];
    unsigned int color[4];
    float uv[4][2];
    long zero;
    long tex;
    long state;
    long flags;
} Pkt;

u64 GetEffectTex_v(s32) __asm__("func_001F4868");

typedef struct {
    float v[8][4];
} Frame;
extern char *l14_cuboids MACRO_ADDR;

extern float electric_beam_base_uv[][2];
extern int electric_beam_effect_texture_index_gp SDATA(D_L14_001621AC);
extern int electric_beam_blend_minuend_selector_gp SDATA(D_L14_001621B0);
extern int electric_beam_blend_subtrahend_selector_gp SDATA(D_L14_001621B4);
extern int electric_beam_blend_alpha_factor_selector_gp SDATA(D_L14_001621B8);
extern int electric_beam_blend_addend_selector_gp SDATA(D_L14_001621BC);

typedef struct {
    int idx;
    char pad4[0x10];
    int color;
    unsigned char cnt[4];
    char pad1c[4];
    float pts[64][4];
    float seg[4][4][4];
} D_03430;

/* Draws the vendor's beam effect. */
void draw_electric_beam(char *m) {
    D_03430 *d = ((D_03430 *)((Moby *)m)->pvars);
    Pkt P[3];
    float M1[16] __attribute__((aligned(16)));
    float M2[16] __attribute__((aligned(16)));
    float v230[4] __attribute__((aligned(16)));
    float v240[4] __attribute__((aligned(16)));
    float v250[4] __attribute__((aligned(16)));
    float v260[4] __attribute__((aligned(16)));
    float v270[4] __attribute__((aligned(16)));
    long tex;
    long flags;
    int j, n, k;
    int c;
    int t;

    euler_to_matrix(M1, ((Frame *)l14_cuboids)[d->idx].v[7]);
    matrix_from_basis_rows(M2, M1);
    qcopy_nc(&M1[12], ((Frame *)l14_cuboids)[d->idx].v[3]);
    FastVecSub(v230, oltanis_camera_pos, ((Frame *)l14_cuboids)[d->idx].v[3]);
    transform_vector_by_basis(v230, v230, M2);
    tex = GetEffectTex_v(electric_beam_effect_texture_index_gp);
    flags = (long)electric_beam_blend_minuend_selector_gp | ((long)electric_beam_blend_subtrahend_selector_gp << 2) |
            ((long)electric_beam_blend_alpha_factor_selector_gp << 4) |
            ((long)electric_beam_blend_addend_selector_gp << 6) | (0x8000L << 24);
    P[2].state = 0xFF9000000260L;
    P[1].state = 0xFF9000000260L;
    P[0].state = 0xFF9000000260L;
    P[2].tex = tex;
    P[1].tex = tex;
    P[0].tex = tex;
    P[2].zero = 0;
    P[1].zero = 0;
    P[0].zero = 0;
    P[2].flags = flags;
    P[1].flags = flags;
    P[0].flags = flags;
    for (t = 0; t < 4; t++) {
        float v = electric_beam_base_uv[t][1];
        int col;
        P[2].uv[t][1] = v;
        P[1].uv[t][1] = v;
        P[0].uv[t][1] = v;
        col = d->color;
        P[2].color[t] = col;
        P[1].color[t] = col;
        P[0].color[t] = col;
    }
    clear_u64_value(v250);
    {
        int i;
        for (i = 0; i < 31; i++) {
            qcopy(v240, v250);
            if (i < 30) {
                FastVecSub(v270, d->pts[i], d->pts[i + 2]);
                FastVecSub(v260, d->pts[i + 1], v230);
                FastVecCross(v250, v260, v270);
                vec_scale_to_length(v250, v250, 0.1f);
            } else {
                clear_u64_value(v250);
            }
            for (k = 0; k < 4; k++) {
                if (k < 2) {
                    qcopy(P[0].corner[k], d->pts[i]);
                    if (k & 1)
                        FastVecAdd(P[0].corner[k], P[0].corner[k], v240);
                    else
                        FastVecSub(P[0].corner[k], P[0].corner[k], v240);
                } else {
                    qcopy(P[0].corner[k], d->pts[i + 1]);
                    if (k & 1)
                        FastVecAdd(P[0].corner[k], P[0].corner[k], v250);
                    else
                        FastVecSub(P[0].corner[k], P[0].corner[k], v250);
                }
                P[0].uv[k][0] = electric_beam_base_uv[k][0] + IntToFloat(i - i / 4 * 4) * 0.25f;
            }
            draw_world_quad(&P[0], M1, 0);
        }
    }
    for (j = 0; j < 4; j++) {
        clear_u64_value(v250);
        c = d->cnt[j];
        if (c >= 0x1F)
            continue;
        if (c <= -1)
            continue;
        for (n = 0; n < 3; n++) {
            qcopy(v240, v250);
            if (n < 2) {
                FastVecSub(v270, d->seg[n][j], d->seg[n + 2][j]);
                FastVecSub(v260, d->seg[n + 1][j], v230);
                FastVecCross(v250, v260, v270);
                vec_scale_to_length(v250, v250, 0.1f);
            } else {
                clear_u64_value(v250);
            }
            if (n == 0)
                qcopy(v240, v250);
            for (k = 0; k < 4; k++) {
                if (k < 2) {
                    qcopy(P[0].corner[k], d->seg[n][j]);
                    if (k & 1)
                        FastVecAdd(P[0].corner[k], P[0].corner[k], v240);
                    else
                        FastVecSub(P[0].corner[k], P[0].corner[k], v240);
                } else {
                    qcopy(P[0].corner[k], d->seg[n + 1][j]);
                    if (k & 1)
                        FastVecAdd(P[0].corner[k], P[0].corner[k], v250);
                    else
                        FastVecSub(P[0].corner[k], P[0].corner[k], v250);
                }
                P[0].uv[k][0] = electric_beam_base_uv[k][0] + IntToFloat(n - n / 4 * 4) * 0.25f;
            }
            draw_world_quad(&P[0], M1, 0);
        }
    }
}

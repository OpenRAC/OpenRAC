/* Starts at 0x00309730 (was vendor_00309730.c). */
/* Level 12 code: func_L12_00309730 in a file of its own, built with no extra flag.
 * vendor_002EDAA0.c needs -fno-sched-interblock (config/file_cflags.txt), and
 * this function only matches without it (docs/BUILD_FIDELITY.md, rule 2). */
#include "common.h"
/* functional: medium. Draw callback: builds a view matrix from the moby (+0xC0), sets up two quad packets (only the first is drawn) and draws it 5 x 6 times as a strip of jittering sparks (corner z grows by 1.0 per outer pass, y gets +-0.2 random jitter, u gets +-0.5 jitter per pass). Packet field meanings guessed from the layout. */
typedef struct Pkt {
    float corner[4][4];
    unsigned int color[4];
    float uv[4][2];
    long zero;
    long tex;
    long state;
    long flags;
} Pkt;
typedef struct Vq { float x, y, z, w; } __attribute__((aligned(16))) Vq;

extern long GetEffectTex(int);
extern void copy_matrix_rotation(void *, void *);
extern float random_float_between(float, float);
extern void draw_world_quad(void *, void *, int);
extern float hoven_jitter_quad_uvs[][2];
extern Vq hoven_jitter_quad_back_corners[];
extern Vq hoven_jitter_quad_front_corners[];
extern short hoven_jitter_quad_color;
extern short hoven_jitter_quad_back_color;
extern short hoven_jitter_quad_blend_a;
extern short hoven_jitter_quad_blend_b;
extern short hoven_jitter_quad_blend_c;
extern short hoven_jitter_quad_blend_d;
extern short hoven_jitter_quad_back_blend_a;
extern short hoven_jitter_quad_back_blend_b;
extern short hoven_jitter_quad_back_blend_c;
extern short hoven_jitter_quad_back_blend_d;

/* Draw five layers of randomly jittered textured quads in the moby's frame. */
void draw_hoven_jittered_quad_layers(char *m) {
    Pkt P[2] __attribute__((aligned(16)));
    float M[16] __attribute__((aligned(16)));
    long tex;
    int c0, c1;
    int a, b, j, k;
    float f20;

    copy_matrix_rotation(M, m + 0xC0);
    qcopy(&M[12], m + 0x10);
    M[15] = 1.0f;
    tex = GetEffectTex(0xE);
    P[0].tex = P[1].tex = tex;
    c0 = *(int *)&hoven_jitter_quad_color;
    c1 = *(int *)&hoven_jitter_quad_back_color;
    P[0].state = P[1].state = 0xFF9000000260L;
    P[0].flags = (long)*(int *)&hoven_jitter_quad_blend_a | ((long)*(int *)&hoven_jitter_quad_blend_b << 2) | ((long)*(int *)&hoven_jitter_quad_blend_c << 4)
                 | ((long)*(int *)&hoven_jitter_quad_blend_d << 6) | (0x8000L << 24);
    P[1].flags = (long)*(int *)&hoven_jitter_quad_back_blend_a | ((long)*(int *)&hoven_jitter_quad_back_blend_b << 2) | ((long)*(int *)&hoven_jitter_quad_back_blend_c << 4)
                 | ((long)*(int *)&hoven_jitter_quad_back_blend_d << 6) | (0x8000L << 24);
    P[0].color[0] = P[0].color[1] = P[0].color[2] = P[0].color[3] = c0;
    P[0].zero = P[1].zero = 0;
    P[1].color[0] = P[1].color[1] = P[1].color[2] = P[1].color[3] = c1;
    for (j = 0; j < 4; j++) {
        float u, v;
        qcopy(P[1].corner[j], &hoven_jitter_quad_back_corners[j]);
        qcopy(P[0].corner[j], &hoven_jitter_quad_front_corners[j]);
        u = hoven_jitter_quad_uvs[j][0];
        v = hoven_jitter_quad_uvs[j][1];
        P[0].uv[j][0] = u;
        P[0].uv[j][1] = v;
        P[1].uv[j][0] = u;
        P[1].uv[j][1] = v;
    }
    for (a = 0; a < 5; a++) {
        float f3 = random_float_between(-0.5f, 0.5f);
        for (k = 0; k < 4; k++) {
            float (*r0)[4] = P[0].corner;
            float (*r1)[4] = P[1].corner;
            r0[k][2] = r0[k][2] + 1.0f;
            r1[k][2] = r1[k][2] + 1.0f;
            P[0].uv[k][0] = hoven_jitter_quad_uvs[k][0] + f3;
            P[0].uv[k][1] = hoven_jitter_quad_uvs[k][1];
        }
        f20 = 0.0f;
        b = -3;
        do {
            int nb = b + 1;
            int more = b < 4;
            float f0 = (float)b + f20;
            f20 = 0.0f;
            P[0].corner[0][1] = f0;
            P[0].corner[1][1] = f0;
            if (more) f20 = random_float_between(-0.2f, 0.2f);
            f0 = (float)nb + f20;
            P[0].corner[2][1] = f0;
            P[0].corner[3][1] = f0;
            b = nb;
            draw_world_quad(&P[0], M, 0);
        } while (b < 3);
    }
}

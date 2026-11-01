#include "include_asm.h"
#include "common.h"
extern u8 D_L03_001A8F20[] MACRO_ADDR;
extern int func_00283D38(int, int, float);
typedef struct { float v[4]; } __attribute__((aligned(16))) VecF720;
typedef struct { VecF720 v[4]; int color[4]; float uv[4][2]; u64 r70, tex, r80, r88; } QuadF720;
extern float D_L03_001B1C10[4] __attribute__((sda));
extern float D_L03_001B1C20[4] __attribute__((sda));
extern VecF720 D_L03_001BFDE0, D_L03_001B9980;
extern int D_L03_001B1C78[2] __attribute__((sda));
extern int D_L03_001B1C80 __attribute__((sda)), D_L03_001B1C84 __attribute__((sda));
extern int D_L03_001B1C88 __attribute__((sda));
extern float D_L03_001B1C90 __attribute__((sda));
extern float D_L03_001B1C98[2] __attribute__((sda));
extern float D_L03_001B1CA0[8] __attribute__((sda));
extern float D_L03_001B1CA4[] MACRO_ADDR;
extern int D_L03_001B1CC0[2] __attribute__((sda));
extern int D_L03_001B2548, D_L03_001B1C8C;
extern void func_00283098(void *, void *, void *);
extern void func_00282CC0(void *, void *, void *);
extern void func_00282CF0(void *, void *, void *);
extern void func_00282DD8(void *, void *, void *);
extern void func_00282F20(void *, void *, float);
extern void func_00282D30(void *, void *, float);
extern void func_002A8C00(void *, void *, int, void *, float);
extern int func_002761F8(void *, void *, int, void *, void *);
extern u64 func_0027C878(int);
extern void func_00280B90(void *, int, int);
extern int func_002E5CF8(int, int, void *);
extern int func_002E59B0(void *, int);
void func_L03_0041F720(u8 *obj)
{
    QuadF720 q[2];
    VecF720 cross, delta, side, start, end;
    u8 hit[0x30] __attribute__((aligned(16)));
    u8 *state = *(u8 **)(obj + 0x68);
    int j, k, n, c, trail, mask;
    float alpha, scale, w;

    D_L03_001B1C20[0] = *(float *)(state + 0x33C);
    func_00283098(&start, &D_L03_001B1C10, obj + 0xC0);
    func_00282CC0(&start, &start, obj + 0x10);
    func_00283098(&end, &D_L03_001B1C20, obj + 0xC0);
    func_00282CC0(&end, &end, obj + 0x10);
    {
        VecF720 *verts = q[0].v;
        int i;

        mask = 1;
        if (obj[0x95] == 2) mask = 0x10001;
        func_002A8C00(hit, obj, mask, &D_L03_001A8F20, (float)state[0x34]);
        {
            int found = func_002761F8(&start, &end, 0, obj, hit);
            QuadF720 *quads = q;

            if (found) qcopy(&end, &D_L03_001BFDE0);
            scale = 1.0f;
            func_00282CF0(&delta, &D_L03_001B9980, &start);
            func_00282CF0(&side, &end, &start);
            func_00282DD8(&cross, &side, &delta);
            func_00282F20(&cross, &cross, scale);
            for (i = 0; i < 2; i++) {
                for (j = 0; j < 4; j++) {
                    w = 1.0f;
                    q[i].color[j] = func_00283D38(D_L03_001B1CC0[i] & 0xFF000000,
                                                 D_L03_001B1CC0[i], *(float *)(state + 0x330));
                    q[i].uv[j][0] = D_L03_001B1CA0[j * 2];
                    q[i].uv[j][1] = D_L03_001B1CA4[j * 2];
                    if (j & 1) func_00282D30(&q[i].v[j], &cross, D_L03_001B1C98[i]);
                    else func_00282D30(&q[i].v[j], &cross, -D_L03_001B1C98[i]);
                    if (j < 2) {
                        func_00282CC0(&q[i].v[j], &q[i].v[j], &start);
                    } else {
                        func_00282CC0(&q[i].v[j], &q[i].v[j], &end);
                    }
                    q[i].v[j].v[3] = w;
                }
                q[i].tex = func_0027C878(D_L03_001B1C78[i]);
                q[i].r80 = 0xFF9000000260ULL;
                q[i].r88 = ((u64)D_L03_001B1C80 << 2) | ((u64)D_L03_001B1C84 << 6) | 0x8000000000ULL;
                q[i].r70 = 0;
            }
            func_00280B90(&q[0], 0, 1);
            func_00280B90(quads + 1, 0, 1);
            for (n = 3, j = 0; n >= 0; n--, j++) { q[0].uv[j][0] = .5f; q[0].uv[j][1] = .5f; }
            alpha = .1f;
            if (D_L03_001B2548 & 1) alpha = 0;
            c = func_00283D38(D_L03_001B1C88, D_L03_001B1C8C, alpha);
            q[0].color[0] = q[0].color[1] = c;
            c = func_00283D38(D_L03_001B1C88, D_L03_001B1C8C, alpha + D_L03_001B1C90);
            q[0].color[2] = q[0].color[3] = c;
            {
                int span = *(s16 *)(state + 0x336) - 4;

                k = (*(s16 *)(state + 0x334) - span) % 4;
            }
            for (trail = 0; trail < *(s16 *)(state + 0x336); trail++) {
                qcopy(&q[0].v[0], state + 0x2A0 + k * 32);
                qcopy(&verts[1], state + 0x2B0 + k * 32);
                k = (k + 1) % 4;
                qcopy(&verts[2], state + 0x2A0 + k * 32);
                qcopy(&verts[3], state + 0x2B0 + k * 32);
                func_00280B90(&q[0], 0, 1);
                alpha += D_L03_001B1C90;
                c = func_00283D38(D_L03_001B1C88, D_L03_001B1C8C, alpha);
                q[0].color[0] = q[0].color[1] = c;
                c = func_00283D38(D_L03_001B1C88, D_L03_001B1C8C, alpha + D_L03_001B1C90);
                q[0].color[2] = q[0].color[3] = c;
            }
            qcopy(&q[0].v[0], state + 0x2A0 + k * 32);
            qcopy(&verts[1], state + 0x2B0 + k * 32);
            qcopy(&verts[2], &start);
            qcopy(&verts[3], &end);
            func_00280B90(&q[0], 0, 1);
            if (*(int *)(state + 0x340) == -1)
                *(int *)(state + 0x340) = func_002E5CF8(5, 4, obj);
            else if (!func_002E59B0(obj, *(int *)(state + 0x340)))
                *(int *)(state + 0x340) = func_002E5CF8(5, 4, obj);
        }
    }
}

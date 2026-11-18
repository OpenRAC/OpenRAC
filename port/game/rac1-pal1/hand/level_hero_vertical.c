/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Native PAL vertical movement update, from the complete 0x5A0 retail body
 * and nonmatching/shared/func_L00_00213C50.c. Not a PS2 match.
 * The candidate's camera interpretation was incorrect: +0xE0 is hero
 * velocity and 002342F8 reads its vertical component, not its length. */
extern char D_0013F350[];
extern char D_0013A4E0[];
extern unsigned char D_0013D4C2;
extern void func_001F9558(void *, void *, void *);
extern float D_0015ED6C MACRO_ADDR;
extern float D_0015ED70 MACRO_ADDR;
extern int D_0015ED84 MACRO_ADDR;
extern int func_L00_0020CFE0(int);
extern int func_001F91B8(int);
extern int func_L00_002169C8(int, int);
extern float func_L00_00233658(float *);
extern float func_002139D0(float *, float, float);
extern void func_L00_00233700(float *dst, float *src, float z);
extern float func_001F94B8(float);
extern void func_L00_002333F0(float *dst, float *src, float dz);
extern void func_L00_002330B0(float *dst, float *src, float h);

typedef struct { float f; float g; int h; } HeroVerticalStep;
typedef struct { char pad[0x14]; HeroVerticalStep *arr; int idx; int cnt; float val; } HeroVerticalSequence;

/* Jump impulse, timed vertical acceleration and gravity in the current
 * movement frame. The vector helpers project/set velocity along its up axis. */
void func_L00_00213C50(void) {
    char *g = D_0013F350;
    float s;
    float vec[4] __attribute__((aligned(16)));
    float len;
    int near;
    int z;

    if (*(int *)(g + 0x2084) == 14) {
        near = 0;
        s = D_0015ED6C * 7.0f;
        if (D_0013D4C2 != 0) {
            if (*(short *)(g + 0x22D8) == 0) {
                near = func_L00_0020CFE0(3) == 3;
            }
        }
        if (D_0015ED84 != 14) {
            if (func_L00_0020CFE0(3) == 3) {
                char *k = D_0013F350;
                if (*(int *)(k + 0x198) == func_001F91B8(5)) {
                    func_L00_002169C8(1, 0);
                }
            }
        }
        func_001F9558(vec, D_0013F350 + 0x80, D_0013F350 + 0x80 + 0x380);
        {
            float d = func_L00_00233658(vec);
            if (near) {
                if (d > 2.7f) s *= 0.47f;
                else if (d > 2.1f) s *= 0.7f;
            } else {
                if (d > 2.1f) s *= 0.45f;
                else if (d > 1.7f) s *= 0.7f;
            }
        }
        if (func_L00_0020CFE0(3) == 3) {
            char *k = D_0013F350;
            if (*(short *)(k + 0x22D8) == 0) {
                s *= 1.25f;
            }
        }
        {
            char *h = D_0013F350;
            if (*(int *)(h + 0x198) < func_001F91B8(8)) {
                float l;
                char *q = h + 0xE0;
                l = func_L00_00233658((float *)q);
                len = l;
                if (l < s) {
                    func_002139D0(&len, s, D_0015ED70 * 150.0f);
                }
                func_L00_00233700((float *)q, (float *)q, len);
            }
        }
    } else {
        z = 0;
        if (*(float *)(g + 0x428) == 0.0f && *(float *)(g + 0x42C) == 0.0f) z = 1;
        if ((*(int *)(D_0013A4E0 + 0x2600) & 0x40) || z != 0
            || *(int *)(g + 0x2084) == 0x12) {
            char *h = D_0013F350;
            if (*(float *)(h + 0x430) < *(float *)(h + 0x48C)) {
                if (*(int *)(h + 0x198) <= func_001F91B8(15)) {
                    *(float *)(h + 0x430) += (*(float *)(h + 0x48C) - *(float *)(h + 0x488)) / (float)*(short *)(h + 0x498);
                    if (*(float *)(h + 0x48C) < *(float *)(h + 0x430)) {
                        *(float *)(h + 0x430) = *(float *)(h + 0x48C);
                    }
                    *(float *)(h + 0x428) += func_001F94B8(*(float *)(h + 0x430) * 2.0f * *(float *)(h + 0x4A0))
                        - *(float *)(h + 0x42C) - *(float *)(h + 0x428);
                }
            }
        }
        {
            char *h = D_0013F350;
            if (!(*(int *)(h + 0x198) < *(int *)(h + 0x420)) && 0.0f < *(float *)(h + 0x428)) {
                func_L00_002333F0((float *)(h + 0xE0), (float *)(h + 0xE0), *(float *)(h + 0x428));
                *(float *)(h + 0x42C) += *(float *)(h + 0x428);
                *(float *)(h + 0x428) = 0.0f;
            }
        }
        {
            char *h = D_0013F350;
            if (*(short *)(h + 0x41C) != 0) {
                int t = *(int *)(h + 0x198);
                if (t >= *(int *)(h + 0x3D0) && t < *(int *)(h + 0x3D4)) {
                    HeroVerticalSequence *p = (HeroVerticalSequence *)(h + 0x3D0);
                    p->cnt++;
                    while (1) {
                        if (p->idx != -1 && p->cnt < func_001F91B8(p->arr[p->idx].h)) break;
                        p->cnt = 0;
                        p->idx++;
                        if (p->arr[p->idx].f != -999999.0f) {
                            p->val = p->arr[p->idx].f * D_0015ED70;
                        }
                    }
                    if (p->cnt > 0) {
                        p->val += p->arr[p->idx].g * D_0015ED70;
                    }
                    func_L00_002333F0((float *)(D_0013F350 + 0xE0), (float *)(D_0013F350 + 0xE0), p->val);
                }
            }
        }
    }
    {
        char *h = D_0013F350;
        int st = *(int *)(h + 0x2084);
        if ((st == 0x1C || st == 0x4C) && *(int *)(h + 0x198) < func_001F91B8(10)) {
            return;
        }
    }
    {
        char *h = D_0013F350;
        if (*(int *)(h + 0x198) < *(int *)(h + 0x420)) {
            float *v = (float *)(h + 0xE0);
            func_L00_00233700(v, v, 0.0f);
            func_L00_002330B0(v, v, D_0015ED70 * 48.0f);
        } else {
            float *v = (float *)(h + 0xE0);
            float a, b, c;
            func_L00_002330B0(v, v, *(float *)(h + 0x4A0));
            a = func_L00_00233658(v);
            b = func_L00_00233658((float *)(h + 0x110));
            c = b - 0.1f;
            if (a < c) a = c;
            c = -(D_0015ED6C * 50.0f);
            if (a < c) a = c;
            func_L00_00233700(v, v, a);
        }
    }
}

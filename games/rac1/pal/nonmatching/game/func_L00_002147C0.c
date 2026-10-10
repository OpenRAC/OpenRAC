/* NON_MATCHING func_L00_002147C0 -- src/overlays/shared/help_0020CDF0.c
 * Best so far: SIZE ours 1436 / retail 1440, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Camera length/offset easing (state 14 scales a length by distance and eases the camera offset vector; other st
 *   Only difference: in the tail clamp `if (a < b - 0.1f) a = b - 0.1f;` retail keeps the second func_L00_002342F8
 *   Idioms that mattered: `char *k = D_0013E633 + 0xE1D;` re-declared per block (no folding into %hi(sym+off)); a 
 */
extern char D_0013E633[];
extern float D_0015EE6C MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;
extern int D_0015EE84_m __asm__("D_0015EE84") MACRO_ADDR;
extern int func_L00_0020DB30(int);
extern int func_001F9850(int);
extern int func_L00_00217570(int, int);
extern float func_L00_002342F8(float *);
extern float func_00214D28(float *, float, float);
extern void func_L00_002343A0(float *dst, float *src, float z);
extern float func_001F9B50(float);
extern void func_L00_00234090(float *dst, float *src, float dz);
extern void func_L00_00233D50(float *dst, float *src, float h);

typedef struct { float f; float g; int h; } Ent_147C0;
typedef struct { char pad[0x14]; Ent_147C0 *arr; int idx; int cnt; float val; } Seq_147C0;

// Per-frame camera length/offset update: scales and eases the camera's offset vector.
void func_L00_002147C0(void) {
    char *g = D_0013E633 + 0xE1D;
    float s;
    float vec[4] __attribute__((aligned(16)));
    float len;
    int near;
    char *k;
    int z;

    if (*(int *)(g + 0x2084) == 14) {
        near = 0;
        s = D_0015EE6C * 7.0f;
        if (D_0013D5CA[0] != 0) {
            if (*(short *)(g + 0x22D8) == 0) {
                near = func_L00_0020DB30(3) == 3;
            }
        }
        if (D_0015EE84_m != 14) {
            if (func_L00_0020DB30(3) == 3) {
                char *k = D_0013E633 + 0xE1D;
                if (*(int *)(k + 0x198) == func_001F9850(5)) {
                    func_L00_00217570(1, 0);
                }
            }
        }
        func_001F9BF0(vec, D_0013E633 + 0xE9D, D_0013E633 + 0xE9D + 0x380);
        {
            float d = func_L00_002342F8(vec);
            if (near) {
                if (d > 2.7f) s *= 0.47f;
                else if (d > 2.1f) s *= 0.7f;
            } else {
                if (d > 2.1f) s *= 0.45f;
                else if (d > 1.7f) s *= 0.7f;
            }
        }
        if (func_L00_0020DB30(3) == 3) {
            char *k = D_0013E633 + 0xE1D;
            if (*(short *)(k + 0x22D8) == 0) {
                s *= 1.25f;
            }
        }
        {
            char *h = D_0013E633 + 0xE1D;
            if (*(int *)(h + 0x198) < func_001F9850(8)) {
                float l;
                char *q = h + 0xE0;
                l = func_L00_002342F8((float *)q);
                len = l;
                if (l < s) {
                    func_00214D28(&len, s, D_0015EE70 * 150.0f);
                }
                func_L00_002343A0((float *)q, (float *)q, len);
            }
        }
    } else {
        z = 0;
        if (*(float *)(g + 0x428) == 0.0f && *(float *)(g + 0x42C) == 0.0f) z = 1;
        if ((*(int *)(D_0013A5E0 + 0x2600) & 0x40) || z != 0
            || (k = D_0013E633 + 0xE1D, *(int *)(k + 0x2084) == 0x12)) {
            char *h = D_0013E633 + 0xE1D;
            if (*(float *)(h + 0x430) < *(float *)(h + 0x48C)) {
                if (*(int *)(h + 0x198) <= func_001F9850(15)) {
                    *(float *)(h + 0x430) += (*(float *)(h + 0x48C) - *(float *)(h + 0x488)) / (float)*(short *)(h + 0x498);
                    if (*(float *)(h + 0x48C) < *(float *)(h + 0x430)) {
                        *(float *)(h + 0x430) = *(float *)(h + 0x48C);
                    }
                    *(float *)(h + 0x428) += func_001F9B50(*(float *)(h + 0x430) * 2.0f * *(float *)(h + 0x4A0))
                        - *(float *)(h + 0x42C) - *(float *)(h + 0x428);
                }
            }
        }
        {
            char *h = D_0013E633 + 0xE1D;
            if (!(*(int *)(h + 0x198) < *(int *)(h + 0x420)) && 0.0f < *(float *)(h + 0x428)) {
                func_L00_00234090((float *)(h + 0xE0), (float *)(h + 0xE0), *(float *)(h + 0x428));
                *(float *)(h + 0x42C) += *(float *)(h + 0x428);
                *(float *)(h + 0x428) = 0.0f;
            }
        }
        {
            char *h = D_0013E633 + 0xE1D;
            if (*(short *)(h + 0x41C) != 0) {
                int t = *(int *)(h + 0x198);
                if (t >= *(int *)(h + 0x3D0) && t < *(int *)(h + 0x3D4)) {
                    Seq_147C0 *p = (Seq_147C0 *)(h + 0x3D0);
                    p->cnt++;
                    while (1) {
                        if (p->idx != -1 && p->cnt < func_001F9850(p->arr[p->idx].h)) break;
                        p->cnt = 0;
                        p->idx++;
                        if (p->arr[p->idx].f != -999999.0f) {
                            p->val = p->arr[p->idx].f * D_0015EE70;
                        }
                    }
                    if (p->cnt > 0) {
                        p->val += p->arr[p->idx].g * D_0015EE70;
                    }
                    func_L00_00234090((float *)(D_0013E633 + 0xEFD), (float *)(D_0013E633 + 0xEFD), p->val);
                }
            }
        }
    }
    {
        char *h = D_0013E633 + 0xE1D;
        int st = *(int *)(h + 0x2084);
        if ((st == 0x1C || st == 0x4C) && *(int *)(h + 0x198) < func_001F9850(10)) {
            return;
        }
    }
    {
        char *h = D_0013E633 + 0xE1D;
        if (*(int *)(h + 0x198) < *(int *)(h + 0x420)) {
            float *v = (float *)(h + 0xE0);
            func_L00_002343A0(v, v, 0.0f);
            func_L00_00233D50(v, v, D_0015EE70 * 48.0f);
        } else {
            float *v = (float *)(h + 0xE0);
            float a, b, c;
            func_L00_00233D50(v, v, *(float *)(h + 0x4A0));
            a = func_L00_002342F8(v);
            b = func_L00_002342F8((float *)(h + 0x110));
            c = b - 0.1f;
            if (a < c) a = c;
            c = -(D_0015EE6C * 50.0f);
            if (a < c) a = c;
            func_L00_002343A0(v, v, a);
        }
    }
}

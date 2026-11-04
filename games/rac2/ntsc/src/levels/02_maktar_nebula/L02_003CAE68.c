#include "include_asm.h"

/* func_L02_003CAE68 is a unit of its own: it needs the opt-in quadword-copy profile
   (-frac2-qcopy, #262), which gives retail's grouped `lq`/`sq` with register addresses. */

typedef unsigned char u8;
typedef unsigned short u16;
typedef short s16;
typedef int TI128 __attribute__((mode(TI)));
typedef struct {
    float x, y, z, w;
} Vec4;

extern Vec4 *D_L02_001FF1C0[];
extern u16 *D_L02_001FC060[];
extern u8 *D_L02_001B359C_1[] __attribute__((sda));
extern int D_L02_001B35B8 __attribute__((sda));

extern void func_00282D08(Vec4 *, Vec4 *, Vec4 *, float);
extern float func_00282E48(Vec4 *, Vec4 *);
extern float func_002A7940(void);
extern float func_00283180(float);
extern float func_00283198(float);
extern void func_00282CC0(char *, char *, Vec4 *);
extern void func_L02_0032EF10(Vec4 *, char *, char *, int, float);
extern void func_L01_00311C18(Vec4 *, char *, char *, u8 *);
extern float func_00282E80(void *, u8 *);

/* Builds the actor's target point: the path's middle point plus an offset of `len` (the distance
   from that middle point to the path's first point) at a random angle (func_002A7940), adjusted by
   func_L02_0032EF10.  For path step 0 it then walks the group's id list, and when the way from
   the actor to the point passes within 3.0 of an entity that the actor is not itself within 3.0
   of, the point becomes the actor's own position (the 16-byte copy).
   Spelling: the walk is the matched id-list walk (a `for` loop with the redundant `p != 0 &&`
   in its test, which keeps jump.c from duplicating the test), and `p` is looked up from the raw
   group word before `g` takes it; that order gives retail's registers for the entry block. */
/* The update caller also supplies an output pointer; this body derives it from self. */
void func_L02_003CAE68(char *self, char *unused_output) {
    char *state = *(char **)(self + 0x68);
    char *rec;
    Vec4 *path;
    Vec4 v, tmp;
    u16 *p;
    u8 *e;
    float len, ang;
    int g;

    rec = state + (*(int *)(state + 0x3C4) << 5);
    path = D_L02_001FF1C0[*(int *)(rec + 0x290)];
    func_00282D08(&v, path + 1, path + 1 + (*(int *)path >> 1), 0.5f);
    len = func_00282E48(&v, path + 1);
    ang = func_002A7940();
    *(float *)(state + 0x3A0) = func_00283180(ang) * len;
    *(float *)(state + 0x3A4) = func_00283198(ang) * len;
    *(int *)(state + 0x3A8) = 0;
    func_00282CC0(state + 0x3A0, state + 0x3A0, &v);
    func_L02_0032EF10(&v, state + 0x3A0, state + 0x3A0, *(int *)(rec + 0x290), 2.0f);
    if (*(int *)(state + 0x3C4) != 0)
        return;
    for (p = D_L02_001FC060[*(int *)(state + 0x39C)], g = *(int *)(state + 0x39C);
         (u8)g != 0xFF && g <= D_L02_001B35B8 && p != 0 &&
         ((p != 0 && (s16)p[-1] >= 0) || p == D_L02_001FC060[g]);
         p++) {
        e = D_L02_001B359C_1[0] + ((*p & 0x7FFF) << 8);
        if (e == 0)
            return;
        func_L01_00311C18(&tmp, self + 0x10, state + 0x3A0, e + 0x10);
        if (func_00282E80(&tmp, e + 0x10) < 3.0f && func_00282E80(self + 0x10, e + 0x10) > 3.0f)
            *(TI128 *)(state + 0x3A0) = *(TI128 *)(self + 0x10);
        g = *(int *)(state + 0x39C);
    }
}

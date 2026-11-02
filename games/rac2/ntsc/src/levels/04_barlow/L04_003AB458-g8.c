#include "include_asm.h"

/* Small data reached through $gp: only one instruction at -G8. */
extern int D_L04_001B0710 __attribute__((sda));
extern int D_L04_001B0714 __attribute__((sda));
extern int D_L04_001B0718 __attribute__((sda));

extern char func_L04_00406C00[];

extern void func_002A0488(void *, int, int, int);
extern void func_L00_002E6CF0(void *, void *);

extern int D_L04_001BFAD8 __attribute__((nosda));
extern int D_0018C0B0 __attribute__((nosda));
extern char D_L04_001A8F20[1];
extern int D_L04_001B0CF8 __attribute__((sda));

extern void func_002A8C28(void *, void *, int, int, float, float, float, int, int);
extern int func_00277258(void *, int, float, void *);
extern void func_002AB250(void *owner, char *offset, void *filter);
extern int func_00282948(void *);

extern int D_L04_001B07B0 __attribute__((sda));
extern int D_L04_001B07B4 __attribute__((sda));
extern int D_L04_001B07B8 __attribute__((sda));
extern char D_L04_001B9680[];
extern char func_L04_00407B80[];
extern void func_L00_002E6EF8(void *, void *);

void func_L04_00407000(char *a0)
{
    char *s0 = a0;

    if (*(unsigned char *)(s0 + 0x20) == 0) {
        func_002A0488(s0, D_L04_001B0710, D_L04_001B0714, D_L04_001B0718);
        *(unsigned char *)(s0 + 0x20) = 1;
    } else {
        func_002A0488(s0, D_L04_001B0710, D_L04_001B0714, D_L04_001B0718);
        func_L00_002E6CF0(func_L04_00406C00, s0);
        *(unsigned short *)(s0 + 0x34) = (*(unsigned short *)(s0 + 0x34) & ~1) | 1;
        *(unsigned short *)(s0 + 0x32) = 0;
    }
}

extern int D_L04_001B24DC __attribute__((sda));
extern char D_L04_001FB1E0[];

void func_L04_00422240(char *a0)
{
    char *s0 = a0;
    char *s1 = s0 + 0x10;

    func_002A8C28(s0, s1, 0x10001, 5, 1.0f, 1.0f, 1.0f, 1, 0);
    if (func_00277258(s1, 1, 1.0f, s0) != 0) {
        if (D_L04_001BFAD8 != 0 && D_L04_001BFAD8 == D_0018C0B0) {
            if (D_L04_001B0CF8 <= 0) {
                D_L04_001B0CF8 = 0x3C;
            }
        }
    }
    if (D_L04_001B0CF8 > 0) {
        func_002AB250(D_0018C0B0, D_L04_001A8F20, 0xFFFF);
        func_00282948(&D_L04_001B0CF8);
    }
}

typedef int TI128 __attribute__((mode(TI)));

/* The flat quad the effect submits: four positions, four colours, four uv
   pairs and the four GS words (primitive, texture, mode, registers). */
typedef struct {
    TI128 position[4];
    unsigned int color[4];
    float uv[4][2];
    unsigned long primitive, texture, mode, registers;
} EffectQuad;

typedef struct {
    TI128 saved;
    EffectQuad quad;
    float a[4];
    float b[4];
    float c[4];
    TI128 corner;
} EffectQuadFrame;

extern void func_00282CF0(void *, void *, void *);
extern void func_00282CC0(void *, void *, void *);
extern void func_00282DD8(void *, void *, void *);
extern void func_00282F20(void *, void *, float);
extern unsigned long func_0027C878(int);
extern void func_00280B90(void *, void *, int);
extern char D_L04_001B9680[];
/* The two uv columns: macro-addressed tables, so each base is one la. */
extern float D_L04_001AFDA0[] __attribute__((sda));
extern float D_L04_001AFDA4[] __attribute__((sda));

/* The quad between the two end points at +0x260 and +0x270: each side
   is pulled a tenth of the way toward the level's anchor, then the four
   corners are crossed out from the span and submitted. */
void func_L04_003E53D0(char *obj, void *q)
{
    EffectQuadFrame f;
    char *st;
    int i;

    f.saved = *(TI128 *)q;
    st = *(char **)(obj + 0x68);
    func_00282CF0(f.c, st + 0x260, st + 0x270);
    func_00282CF0(f.a, st + 0x260, D_L04_001B9680);
    func_00282CF0(f.b, st + 0x270, D_L04_001B9680);
    func_00282DD8(f.a, f.a, f.c);
    func_00282F20(f.a, f.a, 0.1f);
    func_00282DD8(f.b, f.b, f.c);
    func_00282F20(f.b, f.b, 0.1f);
    f.quad.texture = func_0027C878(7);
    f.quad.mode = 0x0000ff9000000260UL;
    f.quad.registers = 0x7000000048UL;
    f.quad.primitive = 0;
    f.quad.color[0] = f.quad.color[1] = f.quad.color[2] = f.quad.color[3] = 0x2F0000FF;
    for (i = 0; i < 4; i++) {
        f.quad.uv[i][0] = D_L04_001AFDA0[i * 2];
        f.quad.uv[i][1] = D_L04_001AFDA4[i * 2];
    }
    func_00282CF0(&f.corner, st + 0x260, f.a);
    f.quad.position[0] = f.corner;
    func_00282CC0(&f.corner, st + 0x260, f.a);
    f.quad.position[1] = f.corner;
    func_00282CF0(&f.corner, st + 0x270, f.b);
    f.quad.position[2] = f.corner;
    func_00282CC0(&f.corner, st + 0x270, f.b);
    f.quad.position[3] = f.corner;
    func_00280B90(&f.quad, 0, 0);
}

extern char D_L04_001B9680[];
/* The two uv columns: macro-addressed tables, so each base is one la. */
extern float D_L04_001AFDA0[] __attribute__((sda));
extern float D_L04_001AFDA4[] __attribute__((sda));

/* The quad between the two end points at +0x260 and +0x270: each side
   is pulled a twentieth of the way toward the level's anchor, then the four
   corners are crossed out from the span and submitted (then drawn again as the overlay). */
void func_L04_003E55E0(char *obj, void *q)
{
    EffectQuadFrame f;
    char *st;
    int i;

    f.saved = *(TI128 *)q;
    st = *(char **)(obj + 0x68);
    func_00282CF0(f.c, st + 0x260, st + 0x270);
    func_00282CF0(f.a, st + 0x260, D_L04_001B9680);
    func_00282CF0(f.b, st + 0x270, D_L04_001B9680);
    func_00282DD8(f.a, f.a, f.c);
    func_00282F20(f.a, f.a, 0.05f);
    func_00282DD8(f.b, f.b, f.c);
    func_00282F20(f.b, f.b, 0.05f);
    f.quad.texture = func_0027C878(0x13);
    f.quad.mode = 0x0000ff9000000260UL;
    f.quad.registers = 0x8000000048UL;
    f.quad.primitive = 0;
    f.quad.color[0] = f.quad.color[1] = f.quad.color[2] = f.quad.color[3] = 0x2FFFFFFF;
    for (i = 0; i < 4; i++) {
        f.quad.uv[i][0] = D_L04_001AFDA0[i * 2];
        f.quad.uv[i][1] = D_L04_001AFDA4[i * 2];
    }
    func_00282CF0(&f.corner, st + 0x260, f.a);
    f.quad.position[0] = f.corner;
    func_00282CC0(&f.corner, st + 0x260, f.a);
    f.quad.position[1] = f.corner;
    func_00282CF0(&f.corner, st + 0x270, f.b);
    f.quad.position[2] = f.corner;
    func_00282CC0(&f.corner, st + 0x270, f.b);
    f.quad.position[3] = f.corner;
    func_00280B90(&f.quad, 0, 0);
    f.quad.texture = func_0027C878(0x10);
    f.quad.mode = 0x0000ff9000000260UL;
    f.quad.registers = 0x8000000048UL;
    f.quad.primitive = 0;
    func_00280B90(&f.quad, 0, 0);
}

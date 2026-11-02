#include "include_asm.h"
#include "common.h"

/* The level's resident spawn counters, in small data. */
extern int D_L09_001B0F50 __attribute__((sda));
extern int D_L09_001B0F54 __attribute__((sda));
extern int D_L09_001B0F58 __attribute__((sda));
extern unsigned short D_L09_001B0F4C __attribute__((sda));

extern int func_002A7820(int a0, int a1);
extern int func_00282978(char *a0);
extern void func_002B0990(int a0, void *a1, int a2, int a3, f32 f12);

/* Small data the run's step needs: the resident mode word (0x001A79F0), the level's
   release word (0x001A9040) and the level's record table (0x001B285C). */
extern int D_001A79F0 __attribute__((sda));
extern int D_L09_001A9040 __attribute__((sda));
extern char *D_L09_001B285C __attribute__((sda));

extern void func_L09_003EA798(char *object);
extern float func_00282E48(char *a, char *b);
extern void func_002A93B0(void *object);
extern char D_L09_001B9A00[];
extern int func_002B0ED8(char *object, char *frame, int flag);
extern char *func_L09_00326E90(char *entry);
extern void func_002A7750(char *object, int a1, int a2);
extern void func_002B3BA8(char *object, int a1, int a2, int a3, f32 f12, f32 f13);

extern void func_0029FCC0(char *object, int joint, void *out);
extern float func_00282E80(void *a, void *b);
extern void func_00282F20(void *dst, void *src, float scale);
extern int func_002A8C28(void *owner, void *start, int queryMode, int first, int second, int mode,
                         f32 radius, f32 distance, f32 scale);
extern int func_002A77E0(int a0);
extern float func_002A7878(float lo, float hi);
extern char *func_002BAE50(void *pos, void *vel, unsigned int color, int a3, int size, int life,
                           int fade, int a7);
extern char D_00189EA0[];
/* Small data: the query range, drop and radius are read $gp-relative; the two level weights
   and the toggle word are plain absolute loads (a weight load the scheduler moves into a delay
   slot comes out $gp-relative). */
extern float D_L09_001B0C40 __attribute__((sda));
extern float D_L09_001B0C44 __attribute__((sda));
extern float D_L09_001B0C48 __attribute__((sda));
extern float D_L09_001B2390 MACRO_ADDR;
extern float D_L09_001B238C MACRO_ADDR;
extern int D_L09_001B2388 MACRO_ADDR;

typedef union {
    int q __attribute__((mode(TI)));
    float f[4];
} Quad_L09_003E9AA0;

/* For joints 5 to 7: queries near D_00189EA0 when close enough, then spawns falling particles
   (two colours by the toggle word, then a third). While neither level weight is above 0.95
   and +0x31 is set, joints 3 and 4 also spawn particles along the +0xC0 vector. */
void func_L09_003E9AA0(char *object) {
    char *data = *(char **)(object + 0x68);
    Quad_L09_003E9AA0 pos, vel, zero;
    int i, j, size, life;

    for (i = 5; i < 8; i++) {
        func_0029FCC0(object, i, &pos);
        if (func_00282E80(&pos, D_00189EA0) < D_L09_001B0C40) {
            qcopy(&vel, &pos);
            vel.f[2] -= D_L09_001B0C44;
            func_002A8C28(object, &vel, 0x10001, 0, 1, 0, D_L09_001B0C48,
                          (float)*(unsigned char *)(data + 0x35), 2.0f);
        }
        if ((!(D_L09_001B2390 > 0.95f) && !(D_L09_001B238C > 0.95f) && *(unsigned char *)(object + 0x31))
            || func_002A77E0(2)) {
            zero.q = 0;
            zero.f[2] = -(func_002A7878(4.5f, 5.5f) * (1.0f / 60.0f));
            vel.q = zero.q;
            if (D_L09_001B2388 & 1)
                func_002BAE50(&pos, &vel, 0xCF0000FF, 207, func_002A7820(18, 22), 180, -10, 1);
            else
                func_002BAE50(&pos, &vel, 0x6000FFFF, 128, func_002A7820(20, 24), 350, -10, 1);
            life = func_002A7820(20, 90);
            size = func_002A7820(10, 15);
            func_002BAE50(&pos, &vel, 0xEFFF7F4F, 0xFF0000, size, life, -life, 1);
        }
    }
    if (!(D_L09_001B2390 > 0.95f) && !(D_L09_001B238C > 0.95f) && *(unsigned char *)(object + 0x31)) {
        for (j = 3; j < 5; j++) {
            func_00282F20(&vel, object + 0xC0, -(func_002A7878(4.5f, 5.5f) * (1.0f / 60.0f)));
            func_0029FCC0(object, j, &pos);
            if (D_L09_001B2388 & 1)
                func_002BAE50(&pos, &vel, 0xCF0000FF, 207, func_002A7820(18, 22), 50, -10, 1);
            else
                func_002BAE50(&pos, &vel, 0x6000FFFF, 128, func_002A7820(20, 24), 100, -10, 1);
            life = func_002A7820(10, 25);
            size = func_002A7820(10, 15);
            func_002BAE50(&pos, &vel, 0xEFFF7F4F, 0xFF0000, size, life, -life, 1);
        }
    }
}

/* Steps the run: refreshes the object's frame, drops the record it still points at when
   the resident mode word says 9, and rebuilds the effect. */
void func_L09_003EAA18(char *object) {
    char *frame = *(char **)(object + 0x68);
    short id;
    int result;

    if (*(unsigned char *)(object + 0x95) == 0)
        func_L09_003EA798(object);

    if (*(unsigned char *)(object + 0x31) != 0) {
        if (func_00282E48(object + 0x10, D_L09_001B9A00) < 32.0f) {
            func_002A93B0(object);
            *(unsigned char *)(object + 0x6F) = 0x1E;
        }
    }

    result = func_002B0ED8(object, frame + 0x180, 0);
    if (result >= 4) {
        if (D_001A79F0 == 9) {
            id = *(short *)(frame + 0x1F0);
            if (id != -1) {
                if (func_L09_00326E90(D_L09_001B285C + (id << 8)) == 0)
                    D_L09_001A9040 = 0;
                *(short *)(frame + 0x1F0) = -1;
            }
        }
        func_002A7750(object, 6, -1);
    }
    func_002B3BA8(object, *(int *)(frame + 0x354), 0, 0, 0.0f, 0.0f);
}

/* The walks below read the group count through $gp; the group table of id lists is absolute
   and the entity base (the level's record table, D_L09_001B285C above through $gp) is the bare
   `lui`/`lw` pair here, a second name sized over 8 bytes with `sda` in this -G8 unit. */
extern int D_L09_001B2878 __attribute__((sda));

extern unsigned char *D_L09_001B285C_1[3] __attribute__((sda));

extern unsigned short *D_L09_001FB560[];

/* Counts the class-0xD0F entities of the object's group whose private state has bit 0 of its
   byte at +0x302 clear and the word a1 at +0x420; -1 when a1 is 0. */
int func_L09_003F71C8(unsigned char *a0, int a1) {
    unsigned short *p;
    unsigned char *e;
    unsigned char *s;
    int n = 0;

    if (a1 == 0)
        return -1;
    for (p = D_L09_001FB560[a0[0x21]];
         a0[0x21] != 0xFF && a0[0x21] <= D_L09_001B2878 && p != 0 &&
         ((p != 0 && (short)p[-1] >= 0) || p == D_L09_001FB560[a0[0x21]]);
         p++) {
        e = D_L09_001B285C_1[0] + ((*p & 0x7FFF) << 8);
        if (e == 0)
            break;
        s = *(unsigned char **)(e + 0x68);
        if (*(short *)(e + 0xAA) == 0xD0F) {
            if (s != 0 && ((s[0x302] ^ 1) & 1)) {
                if (*(int *)(s + 0x420) == a1)
                    n++;
            }
        }
    }
    return n;
}

/* The largest word at +0x420 (0 at least) in the private states, whose byte at +0x302 is 0, of
   the class-0xD0F entities of the object's group. */
int func_L09_003F72A8(unsigned char *a0) {
    unsigned short *p;
    unsigned char *e;
    unsigned char *s;
    int best = 0;
    int v;

    for (p = D_L09_001FB560[a0[0x21]];
         a0[0x21] != 0xFF && a0[0x21] <= D_L09_001B2878 && p != 0 &&
         ((p != 0 && (short)p[-1] >= 0) || p == D_L09_001FB560[a0[0x21]]);
         p++) {
        e = D_L09_001B285C_1[0] + ((*p & 0x7FFF) << 8);
        if (e == 0)
            break;
        s = *(unsigned char **)(e + 0x68);
        if (*(short *)(e + 0xAA) == 0xD0F) {
            if (s != 0 && s[0x302] == 0) {
                v = *(int *)(s + 0x420);
                if (v >= best)
                    best = v;
            }
        }
    }
    return best;
}

/* Re-arms the frame's four spawn slots, bumps the object's counter and re-runs the stage. */
void func_L09_003F8898(char *object) {
    char *frame = *(char **)(object + 0x68);

    if (*(unsigned char *)(frame + 0x2EA) != 0) {
        *(unsigned char *)(frame + 0x2EA) = 0;
        if (*(short *)(frame + 0x2E0) < D_L09_001B0F50)
            *(short *)(frame + 0x2E0) = func_002A7820(D_L09_001B0F54, D_L09_001B0F58);
    }
    if (func_00282978(frame + 0x2E0) == 0)
        *(unsigned short *)(frame + 0x2E8) = D_L09_001B0F4C;
    func_00282978(frame + 0x2E2);
    func_00282978(frame + 0x2E6);
    func_00282978(frame + 0x2E8);
    if (*(unsigned short *)(object + 0x96) <= 0xFFFE)
        *(unsigned short *)(object + 0x96) = *(unsigned short *)(object + 0x96) + 1;
    func_002B0990(*(int *)(frame + 0x2F8), frame + 0x430, 4, 1, 1.9f);
}

#include "include_asm.h"

/* Small-data file: these bodies reach their globals through $gp / the
 * one-instruction model, so they only reproduce at -G8. */

extern unsigned char D_L22_001B2A90[] __attribute__((sda));
extern int *D_L22_001B2AB4[4] __attribute__((sda));
extern int D_L22_001AF314 __attribute__((sda));

extern void ref_L22_003A1E50_00(char *, char *);
extern void ref_L22_003A1E50_01(void *);
extern void ref_L22_003A3C70_00(void *, int, int, int);

void func_L22_003A1E50(char *a0) {
    char *sub = *(char **)(a0 + 0x68);
    unsigned short *counter;
    ref_L22_003A1E50_00(a0, sub + 0x50);
    counter = (unsigned short *)(a0 + 0x96);
    if (*counter <= 0xFFFE) (*counter)++;
    if (*(int *)(sub + 0x37C) != 0) *(int *)(sub + 0x37C) = *(int *)(sub + 0x37C) - 1;
    ref_L22_003A1E50_01(sub + 0x464);
    ref_L22_003A1E50_01(D_L22_001B2A90);
}

void func_L22_003A2A08(void) {
    int *p = D_L22_001B2AB4[0];
    if (p[3] > 0) p[3] = p[3] - 1;
}

void func_L22_003A3C70(int a0, int a1) {
    ref_L22_003A3C70_00(D_L22_001AF314, a0, a1, 0);
}

/* ---- 0x003A48F8 / 0x003A4C68: the same scan of the small-data table at
 * $gp+0x3C0, bounded by the count at 0x001B2AD4 (absolute, hence `nosda`).
 * No guard in the source: retail's leading `blez` is the loop's own entry
 * test, which is what splits the count between $v0 (the load) and $a2 (the
 * loop's bound copy). ---- */

extern int D_L22_001B2AD4 __attribute__((nosda));
extern int D_L22_001AF3B0[1] __attribute__((sda));

int func_L22_003A48F8(int key) {
    int i;
    for (i = 0; i < D_L22_001B2AD4; i++) {
        if (D_L22_001AF3B0[i] == key) return 0;
    }
    return 1;
}

/* The same scan, looking for the second argument and reporting the opposite
 * way. */
int func_L22_003A4C68(int unused, int key) {
    int i;
    for (i = 0; i < D_L22_001B2AD4; i++) {
        if (D_L22_001AF3B0[i] == key) return 1;
    }
    return 0;
}

/* ---- 0x003A45F0: the orbit's own update. ---- */

/* The six level words the update reads: the same addresses in every program
 * that carries the function, so they keep their level-22 names. */
extern float D_L22_001AF3C8 __attribute__((sda));
extern float D_L22_001AF3CC __attribute__((sda));
extern float D_L22_001AF3D0 __attribute__((sda));
extern float D_L22_001AF3D4 __attribute__((sda));
extern float D_L22_001AF3D8 __attribute__((sda));
extern float D_L22_001AF3DC __attribute__((sda));

/* The two boot helpers the update calls; each sits at its own address in
 * every program that carries the function. */
extern void ref_L22_003A45F0_00(void *, float, float, float, float); /* func_002B60B0 */
extern void ref_L22_003A45F0_01(void *, int, int, int);               /* func_002A7470 */

/* One turn of the orbit: the first flag bit runs the object's own update, the
 * second one hands the result over to the child object. */
void func_L22_003A45F0(char *a0, int a1, int a2, int a3, int a4, float a5) {
    char *obj = a0;
    char *sub = *(char **)(a0 + 0x68);
    char *child;
    float f;
    float p0;
    float p1;
    float p2;
    float p3;
    float g0;
    float g1;

    if (a4 & 1) {
        if (*(short *)(sub + 0x34E) == 0) {
            *(int *)(sub + 0x3EC) = *(unsigned char *)(obj + 0x43);
            if (a1 == 11) {
                p0 = D_L22_001AF3D0 * 0.00027777778f;
                p1 = D_L22_001AF3D4 * 0.016666668f;
                p2 = D_L22_001AF3D8 * 4.8481374e-06f;
                p3 = D_L22_001AF3DC * 0.00029088825f;
                ref_L22_003A45F0_00(obj, p0, p1, p2, p3);
                *(float *)(obj + 0x48) = 1.5f;
                *(float *)(sub + 0x138) = 0.5f;
            } else {
                p0 = D_L22_001AF3C8 * 0.00027777778f;
                p1 = D_L22_001AF3CC * 0.016666668f;
                p2 = D_L22_001AF3D8 * 4.8481374e-06f;
                p3 = D_L22_001AF3DC * 0.00029088825f;
                ref_L22_003A45F0_00(obj, p0, p1, p2, p3);
                *(float *)(obj + 0x48) = 1.3f;
                *(float *)(sub + 0x138) = 0.33333334f;
            }

            g0 = *(float *)(sub + 0x128);
            g1 = *(float *)(sub + 0x134);
            g0 = g0 + g0;
            g1 = g1 * 8.0f;
            *(float *)(sub + 0x128) = g0;
            *(float *)(sub + 0x134) = g1;
            if (*(unsigned char *)(obj + 0x43) != a1) {
                ref_L22_003A45F0_01(obj, a1, a2, a3);
            }
        }
    }
    if (*(short *)(sub + 0x34C) != 0) {
        return;
    }
    if (*(unsigned char *)(obj + 0x43) < 11 || *(unsigned char *)(obj + 0x43) > 12) {
        *(float *)(obj + 0x48) = a5;
    }
    f = *(float *)(obj + 0x48);
    *(float *)(*(char **)(sub + 0x440) + 0x48) = f;
    if (a4 & 2) {
        child = *(char **)(sub + 0x440);
        if (child != 0) {
            ref_L22_003A45F0_01(child, a1, a2, a3);
        }
    }
}

/* ---- 0x003A3B50: spawn one object of a random kind. ---- */

typedef struct Hero3B50 {
    unsigned char pad[0x43];
    unsigned char state;
} Hero3B50;

/* In .data, so that -G8 keeps the split lui/lw the entry block allocates around. */
extern Hero3B50 *D_0018C0B0 __attribute__((section(".data")));
extern int D_L22_0023B530[3][60];
/* The target object: the same $gp slot in both levels, under a name each level binds itself. */
extern int ref_L22_003A3B50_00 __attribute__((sda));
extern int func_002A77E0(int);
extern int func_L22_003A39C8(char *, char *, int, int);

/* Picks a kind at random (one of two when the hero's or the target's state says so), scans that
 * kind's row of 50 slots and spawns the object at POS. */
void func_L22_003A3B50(char *pos, int arg) {
    int n;
    int mode = 0;
    int i;
    int *row;

    n = 3;
    if (D_0018C0B0->state == 6 || ((Hero3B50 *)ref_L22_003A3B50_00)->state == 4) n = 2;
    switch (func_002A77E0(n)) {
    case 0:
        mode = 1;
        break;
    case 1:
        mode = 2;
        break;
    case 2:
        mode = 0;
        break;
    }
    row = D_L22_0023B530[mode];
    i = 0;
    if (*row != 0) {
        int *walk = row;
        do {
            i++;
            if (i > 0x31) break;
            walk++;
            if (*walk == 0) break;
        } while (i != 0x31);
    }
    func_L22_003A39C8((char *)ref_L22_003A3B50_00, pos, mode, arg);
}

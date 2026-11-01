#include "include_asm.h"

/* Level 23 globals the retail code reaches through $gp. */
extern char *D_L23_001B1484 __attribute__((sda));

extern int D_L23_001B13EC __attribute__((sda));

extern int D_L23_001B1658 __attribute__((sda));

extern int D_L23_001B15A4 __attribute__((sda));

extern int D_L23_001B178C __attribute__((sda));

extern int D_L23_001B1790 __attribute__((sda));

extern int D_L23_001B1794 __attribute__((sda));

extern int D_L23_001B1798 __attribute__((sda));

extern int D_L23_001B3788;

extern int D_L23_001B38C0;

extern int func_002E5CF8(int index, int a1, char *a2);

extern void func_0027DDC0(int, int, int, int);

extern int D_L23_001B1694 __attribute__((sda));

extern int D_L23_001B1698 __attribute__((sda));

extern float D_L23_001B14E4 __attribute__((sda));

extern void func_31A108(int);

extern void func_L23_0041A648(char *);

extern void func_L23_0042AC70(void);

/* Reports whether a word of the object reached through the pointer global is set. */
int func_L23_0041B948(void) {
    return *(int *)(*(char **)(D_L23_001B1484 + 0x68) + 0x604) != 0;
}

extern void func_0029F978(char *, int, char *);

extern void func_00283898(void *, int, float);

extern float func_00283BE0(float, float);

extern float D_L23_001B1570 __attribute__((sda));

extern float D_L23_001B1574 __attribute__((sda));

/* Relatches the two measured pairs at +0x580 and +0x584 from the two float arguments. */
void func_L23_0041BF18(char *arg0, float arg1, float arg2) {
    char *state = *(char **)(D_L23_001B1484 + 0x68);

    if (*(unsigned char *)(state + 0x4F1) == 0)
        func_0029F978(arg0, 0x28, state + 0x4F0);
    func_00283898(state + 0x500, 0, *(float *)(state + 0x580));
    *(float *)(state + 0x580) = func_00283BE0(*(float *)(state + 0x580), arg1 * D_L23_001B1570 * 0.00029088825f);
    if (*(unsigned char *)(state + 0x531) == 0)
        func_0029F978(arg0, 0x29, state + 0x530);
    func_00283898(state + 0x540, 0, *(float *)(state + 0x584));
    *(float *)(state + 0x584) = func_00283BE0(*(float *)(state + 0x584), arg2 * D_L23_001B1574 * 0.00029088825f);
}

typedef int TI128 __attribute__((mode(TI)));

extern void func_00283098(char *, char *, char *);

extern void func_00282CC0(char *, char *, char *);

extern void func_0032C700(char *, int, int, int, int, int, int, int, float, float, int, int, int, int);

extern char D_00189E20[];

/* Builds the measured vector when the state's first word names the argument, and lays out the call. */
void func_L23_0041C578(char *arg0) {
    union {
        TI128 q;
        float f[4];
    } vec;
    char *base = D_00189E20;

    if (*(char **)(base + 0x2290) == arg0) {
        vec.q = 0;
        vec.f[0] = 1.3f;
        vec.f[2] = 4.5f;
        vec.f[3] = 1.0f;
        func_00283098((char *)&vec, (char *)&vec, arg0 + 0xC0);
        func_00282CC0((char *)&vec, (char *)&vec, base + 0x80);
        func_0032C700((char *)&vec, 0, 10, 10, 10, 25, 30, 0, 3.0f, 20.0f, 0, 1, 0, 0);
    } else {
        func_0032C700(arg0 + 0x10, 0, 25, 20, 20, 15, 25, 1, 1.5f, 20.0f, 0, 0, 0, 0);
    }
    if (*(short *)(arg0 + 0xAA) != 0x118F)
        func_002E5CF8(3, 0, D_L23_001B1484);
}

/* Zero unless the counter is positive and below the limit. */
int func_L23_0041C0C0(void) {
    if (D_L23_001B1694 > 0 && D_L23_001B1694 < D_L23_001B1698)
        return 1;
    return 0;
}

/* One for the two measured state bytes, zero otherwise. */
int func_L23_0041E260(void) {
    unsigned char state = *(unsigned char *)(D_L23_001B1484 + 0x20);

    if (state == 0x10 || state == 0xC)
        return 1;
    return 0;
}



/* Stores one in the state at +0x68, after the measured shared call. */
void func_L23_0041B8F8(char *arg0) {
    char *state = *(char **)(arg0 + 0x68);

    if (D_L23_001B13EC == 0 && D_L23_001B1484 != 0)
        func_002E5CF8(0x10, 0, D_L23_001B1484);
    *(int *)(state + 0x604) = 1;
}

/* Clears the measured slot of the four state pointers and of the argument state. */
void func_L23_0041BFF0(char *arg0) {
    char *state = *(char **)(D_L23_001B1484 + 0x68);
    int *slot;
    int i;

    slot = *(int **)(state + 0x610);
    if (slot != 0)
        *(int *)((char *)slot + 0x98) = 0;
    slot = *(int **)(state + 0x614);
    if (slot != 0)
        *(int *)((char *)slot + 0x98) = 0;
    slot = (int *)(state + 0x358);
    for (i = 4; i >= 0; i--) {
        if (*slot != 0)
            *(int *)(*slot + 0x98) = 0;
        slot++;
    }
    *(int *)(arg0 + 0x98) = 0;
}

extern int D_L23_001B150C __attribute__((sda));

extern float D_L23_001B17A0 __attribute__((sda));

extern float D_L23_001B17A4 __attribute__((sda));

extern float D_L23_001B17A8 __attribute__((sda));

extern float D_L23_001B179C __attribute__((sda));

extern void func_00291450(int);

extern char D_00189EA0[];

extern void func_00282CC0(char *, char *, char *);

extern void func_00282D30(char *, char *, float);

extern float func_002A7878(float, float);

extern int func_L00_002FCF18(char *, float, float, float, float, float);

extern void func_L23_00418650(char *, float);

extern void func_L23_0041E3C8(void);

extern void func_0027D098(void (*)(void), int);

/* Releases the held handle, makes a new one from jittered values when the difference has low bits set, then registers the callback. */
void func_L23_0041E290(char *arg0) {
    char tmp[16];
    int n;
    float r;
    float f21;
    float f20;
    float f16;

    n = D_L23_001B15A4 - D_L23_001B3788;
    if (D_L23_001B150C != -1)
        func_00291450(D_L23_001B150C);
    if ((n & 3) != 0) {
        func_00282CC0(tmp, arg0 + 0x10, D_00189EA0);
        func_00282D30(tmp, tmp, 0.5f);
        func_L23_00418650(tmp, 5.0f);
        r = func_002A7878(0.0f, D_L23_001B17A0 * 0.33333334f);
        f21 = D_L23_001B17A0 + r;
        r = func_002A7878(0.0f, D_L23_001B17A4 * 0.33333334f);
        f20 = D_L23_001B17A4 + r;
        r = func_002A7878(0.0f, D_L23_001B17A8 * 0.33333334f);
        f16 = D_L23_001B17A8 + r;
        D_L23_001B150C = func_L00_002FCF18(tmp, 35.0f, D_L23_001B179C, f21, f20, f16);
    }
    func_0027D098(func_L23_0041E3C8, (int)arg0);
}

/* Calls the measured shared helper with the measured ratio expression. */
void func_L23_0041E3C8(void) {
    func_0027DDC0(D_L23_001B1790, D_L23_001B1794, D_L23_001B1798,
                  (D_L23_001B15A4 - D_L23_001B3788) * D_L23_001B178C / D_L23_001B15A4);
}

/* Stores the incoming float in the level global reached through $gp. */
void func_L23_0041B960(float arg0) { D_L23_001B14E4 = arg0; }

/* Copies the measured word of every live slot into the object the argument names. */
void func_L23_0041C040(char *arg0) {
    char *state = *(char **)(D_L23_001B1484 + 0x68);
    char *slot;
    char **p;
    int i;

    slot = *(char **)(state + 0x610);
    if (slot != 0)
        *(int *)(slot + 0x98) = *(int *)(*(char **)(slot + 0x24) + 0x10);
    slot = *(char **)(state + 0x614);
    if (slot != 0)
        *(int *)(slot + 0x98) = *(int *)(*(char **)(slot + 0x24) + 0x10);
    p = (char **)(state + 0x358);
    for (i = 4; i >= 0; i--) {
        if (*p != 0)
            *(int *)(*p + 0x98) = *(int *)(*(char **)(*p + 0x24) + 0x10);
        p++;
    }
    *(int *)(arg0 + 0x98) = *(int *)(*(char **)(arg0 + 0x24) + 0x10);
}

/* Releases the three measured handles of the level state. */
void func_L23_0041D930(void) {
    char *state;

    func_L23_0041A648(D_L23_001B1484);
    state = *(char **)(D_L23_001B1484 + 0x68);
    if (*(char **)(state + 0x620) != 0) {
        func_31A108((int)*(char **)(state + 0x620));
        *(char **)(state + 0x620) = 0;
    }
    if (*(char **)(state + 0x624) != 0) {
        func_L23_0042AC70();
        func_31A108((int)*(char **)(state + 0x624));
        *(char **)(state + 0x624) = 0;
    }
    if (*(char **)(state + 0x614) != 0) {
        func_31A108((int)*(char **)(state + 0x614));
        *(char **)(state + 0x614) = 0;
    }
}

/* Zero while the counter is negative. */
int func_L23_0041C0B0(void) {
    if (D_L23_001B1694 < 0)
        return 1;
    return 0;
}

/* Zero when the value is among the first D_L23_001B38C0 words of the table, else one. */
int func_L23_00419CA0(int arg0) {
    int *tab = &D_L23_001B1658;
    int i;

    for (i = 0; i < D_L23_001B38C0; i++) {
        if (tab[i] == arg0)
            return 0;
    }
    return 1;
}

extern int D_L23_001B1424 __attribute__((sda));

extern unsigned char D_001395B8[];


extern void func_0029B758(int, int);

extern void func_0029CE80(void);

/* Releases the five slot objects and the level object, then raises three flags. The flags are
 * written through a pointer set after the calls, as retail's `lui`/`addiu` base with the 0x88 and
 * 0xA1 displacements; written as D_001395B8[0xA1] and [0x88], the high half is hoisted into a
 * saved register (160 bytes). */
void func_L23_0041D9B0(void) {
    char **p = (char **)(*(char **)(D_L23_001B1484 + 0x68) + 0x358);
    unsigned char *flags;
    int i;

    for (i = 4; i >= 0; i--) {
        if (*p != 0) {
            func_0029F338(*p);
            *p = 0;
        }
        p++;
    }
    if (D_L23_001B1484 != 0) {
        func_0029B758(0, 0);
        func_0029CE80();
        func_0029F338(D_L23_001B1484);
        D_L23_001B1484 = 0;
    }
    flags = D_001395B8;
    D_L23_001B1424 = 1;
    flags[0xA1] = 1;
    flags[0x88] = 1;
}

/* The state at +0x68 as func_L23_0041BE70 reads it. Indexing the slot array through the
 * struct keeps the base first in the address sum (`addu $v0,$a0,$v0`); pointer arithmetic
 * puts the scaled index first. */
typedef struct Vars_0041BE70 {
    char pad0[0x358];
    char *slot[5];            /* 0x358 */
    char pad1[0x60C - 0x36C];
    int index;                /* 0x60C */
    char *held;               /* 0x610 */
} Vars_0041BE70;

/* A scalar: declared as an array, its address is scheduled differently in the two calls. */
extern int D_L23_001B36E0;

extern void func_002A90C8(void *, int, int, void *, float);

/* Attaches the level object, the indexed slot or the held object, with the argument's two
 * words. The two early returns give retail's arm layout: [no held object][slot][held]. */
void func_L23_0041BE70(char *arg0) {
    char *state = D_L23_001B1484;
    Vars_0041BE70 *vars = *(Vars_0041BE70 **)(state + 0x68);
    int index = vars->index;
    char *p;

    if (index == -1) {
        p = vars->held;
        if (p == 0) {
            func_002A90C8(state, *(int *)(arg0 + 0x20), 0x10000, state + 0x10,
                          *(float *)(arg0 + 0x2C));
            return;
        }
    } else {
        func_002A90C8(vars->slot[index], *(int *)(arg0 + 0x20), 0x10000, &D_L23_001B36E0,
                      *(float *)(arg0 + 0x2C));
        return;
    }
    if ((*(unsigned short *)(p + 0x34) | 0x4000) != 0)
        func_002A90C8(p, *(int *)(arg0 + 0x20), 0x10000, &D_L23_001B36E0,
                      *(float *)(arg0 + 0x2C));
}

extern float D_L23_001B1400 __attribute__((sda));

extern float D_L23_001B1404 __attribute__((sda));

extern float D_L23_001B1408 __attribute__((sda));

/* Returns the new value in $f0. Declared void, the call leaves $f0 free and the measure
 * reloaded right after it takes $f0 where retail has $f1. */
extern float func_002AA548(float *, float *, float, float, float, float);

extern float func_00282C48(float);

/* Springs the first measure toward its target at the three scaled global rates, and latches
 * it once it is within a tenth of the target. */
void func_L23_0041B498(char *arg0) {
    char *state = *(char **)(arg0 + 0x68);

    if (*(int *)(state + 0x5DC) != 0)
        return;
    func_002AA548((float *)(state + 0x5C8), (float *)(state + 0x5CC), *(float *)(state + 0x5D0),
                  D_L23_001B1400 * 0.00027777778f, D_L23_001B1404 * 0.00027777778f,
                  D_L23_001B1408 * 0.016666668f);
    if (func_00282C48(*(float *)(state + 0x5C8) - *(float *)(state + 0x5D0)) < 0.1f) {
        *(int *)(state + 0x5DC) = 1;
        *(int *)(state + 0x5CC) = 0;
    }
}

extern void func_0029F338(void *);

extern void func_002E59F8(int);

extern void func_00291450(int);

extern int D_L23_001B1418 __attribute__((sda));

extern short D_L23_001B146C __attribute__((sda));

/* the two 8 x 3 handle tables */
extern void *D_L23_00259320[8][3];

extern void *D_L23_00259380[8][3];

/* four handles; declared at 8 bytes so this -G8 unit loads its address as the bare
   lui/addiu pair into one register, as retail does (an incomplete or 16-byte array splits it) */
extern void *D_L23_001B3790[2];

/* Releases the measured handles, then every object handle of the three tables, and clears them. */
void func_L23_0041A648(char *arg0) {
    char *state = *(char **)(arg0 + 0x68);
    int i;
    int j;

    if (D_L23_001B1418 != -1) {
        func_002E59F8(D_L23_001B1418);
        D_L23_001B1418 = -1;
    }
    if (D_L23_001B146C != -1) {
        func_00291450(D_L23_001B146C);
        D_L23_001B146C = -1;
    }
    if (*(int *)(state + 0x5B0) != 0) {
        *(int *)(state + 0x5C4) = 0;
        for (i = 0; i < 8; i++) {
            for (j = 0; j < 3; j++) {
                if (D_L23_00259320[i][j] != 0) {
                    func_0029F338(D_L23_00259320[i][j]);
                    D_L23_00259320[i][j] = 0;
                }
            }
        }
        for (i = 0; i < 4; i++) {
            if (D_L23_001B3790[i] != 0) {
                func_0029F338(D_L23_001B3790[i]);
                D_L23_001B3790[i] = 0;
            }
        }
        for (i = 0; i < 8; i++) {
            for (j = 0; j < 3; j++) {
                if (D_L23_00259380[i][j] != 0) {
                    func_0029F338(D_L23_00259380[i][j]);
                    D_L23_00259380[i][j] = 0;
                }
            }
        }
        *(int *)(state + 0x5B0) = 0;
    }
}

/* boot data: a pointer whose byte at +0x43 selects the draw range; nosda keeps its lui/lw pair */
extern unsigned char *D_0018C0B0 __attribute__((nosda));

extern int D_L23_00259050[3][60];

extern int func_002A77E0(int);

extern int func_L23_0041C8C0(char *, void *, int, int);

/* Draws a row (0, 1 or 2, the last one only when the byte at +0x43 is not 6) and hands the
 * position to func_L23_0041C8C0 with that row. The search for a free slot is retail's: its
 * result is never used. The three cases make cc1 build the decision tree rooted on 1 (with two,
 * it emits a comparison chain from 0). */
void func_L23_0041C7D8(void *pos, int arg1) {
    int type = 0;
    int i;

    switch (func_002A77E0(D_0018C0B0[0x43] == 6 ? 2 : 3)) {
    case 0:
        type = 1;
        break;
    case 1:
        type = 2;
        break;
    case 2:
        type = 0;
        break;
    }
    for (i = 0; i < 50; i++) {
        if (D_L23_00259050[type][i] == 0)
            break;
        if (i == 49) {
            i = -1;
            break;
        }
    }
    func_L23_0041C8C0(D_L23_001B1484, pos, type, arg1);
}
/* The fields of the level state, the object and the hero this function reads. */
typedef struct {
    char pad0[0x408];
    int handle;
    char pad40C[0x588 - 0x40C];
    float limit;
    char pad58C[0x5B8 - 0x58C];
    float dist;
    float range;
    float level;
} C0E8State;

typedef struct {
    char pad0[0x10];
    TI128 pos;
    unsigned char state;
} C0E8Moby;

typedef struct {
    char pad0[0x80];
    float pos[4];
    char pad90[0x1DC - 0x90];
    short busy;
    char pad1DE[0x33C - 0x1DE];
    int armed;
    int charge;
    char pad344[0x34E - 0x344];
    short heat;
} C0E8Hero;

extern int D_L23_001B141C __attribute__((sda));

extern float D_L23_001B168C __attribute__((sda));

extern float D_L23_001B1690 __attribute__((sda));

extern int D_L23_001B169C __attribute__((sda));

extern int D_L23_001B16A4 __attribute__((sda));

extern int D_L23_001B16A8 __attribute__((sda));

extern int D_L23_001B16AC __attribute__((sda));

extern float D_L23_001B16B0 __attribute__((sda));

extern float D_L23_001B36D0[4] __attribute__((sda));

extern int D_L23_001B371C;

extern int func_00282948(int *);

extern void func_00282CF0(float *, float *, float *);

extern void func_00282F20(float *, float *, float);

extern void func_L23_0041E5B0(void *, int, int, TI128 *, float *, int);

extern int func_L23_00426248(void *, void *, int, int, int, int, int, int, float, int, int, int, int, int);

void func_L23_0041C0E8(C0E8Moby *arg0) {
    C0E8State *state = *(C0E8State **)(D_L23_001B1484 + 0x68);
    float v[4] __attribute__((aligned(16)));
    TI128 t;

    func_00282948(&D_L23_001B371C);
    if (arg0->state == 0x10 && state->dist < D_L23_001B168C && state->level > state->limit &&
        func_00282C48(state->level - state->limit) < D_L23_001B1690) {
        C0E8Hero *hero = (C0E8Hero *)D_00189E20;

        if (hero->charge >= 11 && D_L23_001B1694 == 0) {
            D_L23_001B16A4 = 1;
            D_L23_001B1694 = D_L23_001B169C;
            if (D_L23_001B16A8 != 0) {
                D_L23_001B16A8 = 0;
                D_L23_001B1694 = D_L23_001B1698;
            }
        }
    }
    if (D_L23_001B1694 > 0) {
        if (D_L23_001B1694 == D_L23_001B1698) {
            if (D_L23_001B16A4 != 0)
                D_L23_001B371C = 0;
            else
                D_L23_001B1694 += 2;
        } else {
            C0E8Hero *hero = (C0E8Hero *)D_00189E20;

            if (hero->heat >= 11)
                D_L23_001B16A8 = 1;
        }
    }
    if (D_L23_001B16A8 != 0 && D_L23_001B1694 > D_L23_001B1698) {
        C0E8Hero *hero = (C0E8Hero *)D_00189E20;

        if (hero->charge >= 11)
            D_L23_001B1694 = D_L23_001B1698;
    }
    {
        C0E8Hero *hero = (C0E8Hero *)D_00189E20;

        if (hero->charge != 0 && hero->armed == 0)
            D_L23_001B16A8 = 0;
    }
    {
        C0E8Hero *hero = (C0E8Hero *)D_00189E20;

        if (hero->charge >= 11 && D_L23_001B16A4 == 0 && D_L23_001B1694 > 0 && D_L23_001B1694 <= D_L23_001B1698)
            D_L23_001B1694 = D_L23_001B1698 + 5;
    }
    if (D_L23_001B1694 == D_L23_001B1698) {
        func_002E5CF8(10, 0, (char *)arg0);
        func_002E5CF8(0x11, 0, (char *)arg0);
    }
    if (D_L23_001B1694 < 0) {
        D_L23_001B1694++;
        if (D_L23_001B371C == 0) {
            C0E8Hero *hero = (C0E8Hero *)D_00189E20;

            if (hero->charge != 0 && state->range < 10.0f && state->level > state->limit) {
                func_00282CF0(v, hero->pos, (float *)&arg0->pos);
                func_00282F20(v, v, 3.0f);
                t = arg0->pos;
                v[3] = 5627.9248046875f;
                func_L23_0041E5B0((void *)state->handle, 0x10007, 0x46, &t, v, 0);
                D_L23_001B371C = D_L23_001B141C;
                func_0032C700((char *)D_L23_001B36D0, 1, 5, 0x46, 3, 15, 20, 1, 3.0f, 20.0f, 0, 1, 0, 0);
            }
        }
    } else if (D_L23_001B1694 > 0) {
        func_00282948(&D_L23_001B1694);
        if (D_L23_001B1694 == 0) {
            state->handle = func_L23_00426248(&arg0->pos, arg0, 1, 0x19, 0x14, 0x14, 0x14, 0x2D, 1.0f, 1, 1, 0, 0, 0x40);
            D_L23_001B1694 = -D_L23_001B16AC;
            func_002E5CF8(7, 0, (char *)arg0);
            func_002E5CF8(12, 0, (char *)arg0);
            if (state->range < 15.0f && state->level > state->limit) {
                C0E8Hero *hero = (C0E8Hero *)D_00189E20;

                if (hero->busy == 0) {
                    func_00282CF0(v, hero->pos, (float *)&arg0->pos);
                    func_00282F20(v, v, D_L23_001B16B0);
                    t = arg0->pos;
                    v[3] = 5627.9248046875f;
                    func_L23_0041E5B0(arg0, 0x10007, 0x46, &t, v, 0);
                    func_0032C700((char *)D_L23_001B36D0, 1, 5, 0x46, 3, 15, 20, 1, 3.0f, 20.0f, 0, 1, 0, 0);
                }
            }
        }
    }
}

#include "include_asm.h"

extern int D_L02_001B04DC __attribute__((sda));

extern void func_L02_003DBA70();

extern void func_0029FCC0(char *arg0, int arg1, char *arg2);

extern char D_L02_001B0F60[4] __attribute__((sda));

extern int D_L02_001B0FA0 __attribute__((sda));

extern int D_L02_001B0FA4;

extern void func_2EEFB0(char *, int, char *, char *, int, int, int, int, float);

/* Hands the actor's four widget slots to the effect scheduler. */
void func_L02_003F16A8(char *arg0) {
    char *state = *(char **)(arg0 + 0x68);

    func_2EEFB0(D_L02_001B0F60, 4, state + 0x2A0, state + 0x3A0,
                *(int *)(state + 0x4D4), *(int *)(state + 0x4D0), D_L02_001B0FA0,
                D_L02_001B0FA4, *(float *)(state + 0x4D8));
}

/* Rebuild the four widget descriptions from the resident template. */
void func_L02_003DB9E0(char *arg0) {
    char buffer[16];

    func_0029FCC0(arg0, 2, buffer);
    func_L02_003DBA70(buffer, D_L02_001B04DC);
    func_0029FCC0(arg0, 4, buffer);
    func_L02_003DBA70(buffer, D_L02_001B04DC);
    func_0029FCC0(arg0, 8, buffer);
    func_L02_003DBA70(buffer, D_L02_001B04DC);
    func_0029FCC0(arg0, 6, buffer);
    func_L02_003DBA70(buffer, D_L02_001B04DC);
}

/* The entity array's base: `sda` with a size over 8 bytes gives the bare-symbol load, which the
   assembler expands to retail's one-register `lui`/`lw` pair instead of a $gp access. */
extern unsigned char *D_L02_001B359C[3] __attribute__((sda));

extern int D_L02_001B35B8 __attribute__((sda));

extern unsigned short *D_L02_001FC060[];

/* For each entity of the object's group that has class 0xAB0, sets its state byte (+0x20) from
   the first word of its private data: 1 gives 6, 2 gives 7, anything else 3. */
void func_L02_003EAD38(unsigned char *arg0) {
    unsigned short *q;
    unsigned char *e;

    if (arg0[0x21] == 0xFF)
        return;
    q = D_L02_001FC060[arg0[0x21]];
    while (arg0[0x21] != 0xFF && arg0[0x21] <= D_L02_001B35B8 && q != 0 &&
           ((q != 0 && (short)q[-1] >= 0) || q == D_L02_001FC060[arg0[0x21]])) {
        e = D_L02_001B359C[0] + ((*q & 0x7FFF) << 8);
        if (e == 0)
            break;
        if (*(short *)(e + 0xAA) == 0xAB0) {
            int t = **(int **)(e + 0x68);

            e[0x20] = (t == 1) ? 6 : ((t == 2) ? 7 : 3);
        }
        q++;
    }
}

/* 0 when an entity of the object's group has class 0xAB0 and a state byte (+0x20) other than
   6 or 7, else 1. */
int func_L02_003EAFC8(unsigned char *arg0) {
    unsigned short *q;
    unsigned char *e;

    if (arg0[0x21] == 0xFF)
        return 1;
    q = D_L02_001FC060[arg0[0x21]];
    while (arg0[0x21] != 0xFF && arg0[0x21] <= D_L02_001B35B8 && q != 0 &&
           ((q != 0 && (short)q[-1] >= 0) || q == D_L02_001FC060[arg0[0x21]])) {
        e = D_L02_001B359C[0] + ((*q & 0x7FFF) << 8);
        if (e == 0)
            break;
        q++;
        if (*(short *)(e + 0xAA) == 0xAB0 && (unsigned char)(e[0x20] - 6) >= 2)
            return 0;
    }
    return 1;
}

extern int D_L02_001B35B8 __attribute__((sda));

/* The word D_L02_001B359C, read here through $gp; the walks above reach it as a bare symbol. */
extern char *D_L02_001B359C_gp;

extern unsigned short *D_L02_001FC060[];

extern char D_00189E20[];

extern int func_002AD0B0(char *);

extern void func_002AD6F0(void *, void *, void *, void *);

/* For every entity of the object's group, copies the word at +0x10 of its +0x24 record into
   its word at +0x98. */
void func_L02_003EB098(unsigned char *a0) {
    unsigned short *p;
    char *e;

    for (p = D_L02_001FC060[a0[0x21]];
         a0[0x21] != 0xFF && a0[0x21] <= D_L02_001B35B8 && p != 0 &&
         ((p != 0 && (short)p[-1] >= 0) || p == D_L02_001FC060[a0[0x21]]);
         p++) {
        e = D_L02_001B359C_gp + ((*p & 0x7FFF) << 8);
        if (e == 0)
            break;
        *(int *)(e + 0x98) = *(int *)(*(char **)(e + 0x24) + 0x10);
    }
}

/* Walks the entities of the actor's group (D_L02_001FC060[actor[0x21]], ids masked with 0x7FFF,
   0x100-byte records at D_L02_001B359C, the list ends after an id with bit 15 set). Each entity
   of class 0xAAF has its effect (func_002AD0B0) handed to func_002AD6F0; the result says whether
   one of them is the entity D_00189E20 tracks at +0x33C while its flag at +0x340 is set.

   The loop condition tests p twice, the second time as the guard of the p[-1] read, as a list
   macro written on top of a pointer check produces. That second test is what makes retail's
   loop: its `||` leaves a label in the exit test, so jump.c keeps the test at the bottom instead
   of copying it in front of the loop (retail enters with `b test`); GCSE's %hi copy then lands
   after the loop note, which makes loop.c skip the loop (no hoisted constants) and the
   scheduler leave the copy last (the delay slot of `b test`); the loop depth puts that copy in
   $s3 and a1 in $s4. CSE removes the repeated test afterwards. Without it the test is copied in
   front of the loop and the function grows to 364 bytes. */
int func_L02_003EB138(unsigned char *a0, void *a1) {
    int result = 0;
    unsigned short *p;
    char *e;

    for (p = D_L02_001FC060[a0[0x21]];
         a0[0x21] != 0xFF && a0[0x21] <= D_L02_001B35B8 && p != 0 &&
         ((p != 0 && (short)p[-1] >= 0) || p == D_L02_001FC060[a0[0x21]]);
         p++) {
        e = D_L02_001B359C_gp + ((*p & 0x7FFF) << 8);
        if (e == 0)
            break;
        if (*(short *)(e + 0xAA) == 0xAAF) {
            char *g = D_00189E20;
            void *v;

            if (e == *(char **)(g + 0x33C)) {
                if (*(int *)(g + 0x340) != 0)
                    result = 1;
            }
            v = (void *)func_002AD0B0(e);
            if (v != 0)
                func_002AD6F0(v, a1, a0 + 0xF0, a0 + 0xF0);
        }
    }
    return result;
}

extern unsigned char D_001395B8[0x100];

extern int D_0018C2EC __attribute__((nosda));

extern int D_001A7A00;

/* The six weights, read through $gp: `sda` with a size of at most 8 bytes, indexed past it. */
extern int D_L02_001B0938[2] __attribute__((sda));

extern char D_L02_001B09E0[];

extern char D_L02_001B09F0[];

extern void func_0026FEB8(const char *, ...);

extern int func_002A77E0(int);

extern int func_002A7820(int, int);

/* Rolls the state's mode (+0x5C) against the six weights, turns mode 4 into 1 when the two
   reference words agree and mode 5 into 1 while the counter at D_001395B8[0x8F] is under 4,
   copies it into the three slots at +0x90, and for mode 0 rolls three values from 1 to 5 until
   they are not all the same. The jump pass peels the loop's first test in front of it; the unit
   is built with -frac2-gcse-unsplit-edges (config/cflags.txt), so the two high halves gcse hoists
   sit above that copy as in retail. */
void func_L02_003E79F8(char *arg0) {
    char *state = *(char **)(arg0 + 0x68);
    int r;
    int i;
    int v;

    r = func_002A77E0(1000);
    func_0026FEB8(D_L02_001B09E0, r);
    *(int *)(state + 0x5C) = 0;
    for (i = 0; i < 6; i++) {
        r -= D_L02_001B0938[i];
        if (r < 0) {
            *(int *)(state + 0x5C) = i;
            break;
        }
    }
    if (*(int *)(state + 0x5C) == 4) {
        if (D_0018C2EC == D_001A7A00)
            *(int *)(state + 0x5C) = 1;
    }
    if (D_001395B8[0x8F] < 4) {
        if (*(int *)(state + 0x5C) == 5)
            *(int *)(state + 0x5C) = 1;
    }
    v = *(int *)(state + 0x5C);
    *(int *)(state + 0x90) = *(int *)(state + 0x94) = *(int *)(state + 0x98) = v;
    func_0026FEB8(D_L02_001B09F0, v);
    if (*(int *)(state + 0x5C) == 0) {
        int a;

        *(int *)(state + 0x90) = func_002A7820(1, 5);
        *(int *)(state + 0x94) = func_002A7820(1, 5);
        do {
            a = func_002A7820(1, 5);
            *(int *)(state + 0x98) = a;
        } while (*(int *)(state + 0x90) == *(int *)(state + 0x94) &&
                 *(int *)(state + 0x90) == a);
    }
}

extern unsigned char D_001A7AFA;
extern char D_00189EA0[];
extern unsigned short D_0018C31C[];
extern int func_002ACBC8(void *, int);
extern void func_L02_003DBFD8(void);
extern void func_002A7750(char *, int, int);

void func_L02_003DBF40(char *arg0) {
    char *state = *(char **)(arg0 + 0x68);

    switch (*(unsigned char *)(arg0 + 0x20)) {
    case 0:
        *(unsigned char *)(arg0 + 0x30) = 0xFF;
        if (D_001A7AFA == 0)
            D_0018C31C[0] = 1;
        func_002A7750(arg0, 1, -1);
        break;
    case 1:
        func_L02_003DBFD8();
        if (func_002ACBC8(D_00189EA0, *(int *)state))
            *(unsigned short *)(D_00189EA0 + 0x19A) = 5;
        break;
    }
}

/* D_L02_001CAA80 is read as the first word of a larger object: cc1 then splits the address and
   retail's `lui $v0` / `lw $v1,%lo($v0)` come out in two registers, where a one-word extern is a bare
   symbol that gas expands in one. */
extern int D_L02_001CAA80[3];

extern int D_L02_001AEC78;

extern int D_L02_001B41DC;

extern int D_L02_001B30C8;

extern int D_L02_001B0A60 __attribute__((sda));

extern float D_L02_001B0A20 __attribute__((sda));

extern float D_L02_001B0A28 __attribute__((sda));

extern float D_L02_001B0A30 __attribute__((sda));

extern void func_002A7750();

extern void *func_002A9640(void *, int, int);

extern void func_L00_002E6CF0(void *, void *);

extern void func_L02_003E80F8();

extern void func_L00_0032A258(void *, void *, void *, void *, int, int, int, int, int, int, int);

extern void func_L00_00326110(void *, int, float);

extern void func_0032C700(void *, int, int, int, int, int, int, int, float, float, int, int, int, int);

/* One step of an actor, by its state byte (+0x20). State 0 marks the actor (+0x30), moves it to
   state 1 and lowers the counter D_L02_001AEC78 when its private word is set. Unless
   D_L02_001CAA80 is 1, state 1 registers the callback func_L02_003E80F8 once per value of
   D_L02_001B30C8 and, when func_002A9640 finds something and the private word is clear, lowers
   the counter and moves to state 2; it always ends by setting the byte at +0xA8. State 2 starts
   the effects and moves to state 3, which sets the half-word at +0x7C. */
void func_L02_003E7F28(unsigned char *a0) {
    int state = a0[0x20];
    int *s = *(int **)(a0 + 0x68);

    switch (state) {
    case 0:
        a0[0x30] = 0xFF;
        func_002A7750(a0, 1, -1);
        if (*s != 0)
            D_L02_001AEC78--;
        break;
    case 1:
        if (D_L02_001CAA80[0] != 1) {
            if (D_L02_001B41DC != D_L02_001B30C8) {
                D_L02_001B41DC = D_L02_001B30C8;
                func_L00_002E6CF0(func_L02_003E80F8, a0);
            }
            if (func_002A9640(a0, 0x4010000, 0) != 0 && *s == 0) {
                D_L02_001AEC78--;
                func_002A7750(a0, 2, -1);
            }
            a0[0xA8] = 0xFF;
        }
        break;
    case 2:
        func_L00_0032A258(a0, &D_L02_001B0A30, &D_L02_001B0A28, &D_L02_001B0A20, 0xFFFF, 0,
                          D_L02_001B0A60, 1, 0xFFFF, 0xFF, 0);
        func_L00_00326110(a0, 5, 0.5f);
        func_0032C700(a0 + 0x10, 1, 0x19, 0x14, 0x14, 0x14, 0x2D, 1, 1.0f, 20.0f, 1, 1, 0, 0);
        func_002A7750(a0, 3, -1);
        break;
    case 3:
        *(unsigned short *)(a0 + 0x7C) = 0x8020;
        break;
    }
}

extern int func_L00_002B3998(float);

extern float func_00283180(float);

extern float func_00283198(float);

extern float func_002A89A8(void *, int, float);

extern float func_002A7878(float, float);

extern void func_002C3D80(void *, int, float, float);

extern float D_L02_001B0530 __attribute__((sda));

extern float D_L02_001B0534;

/* Looks for a trigger in the hero block (D_00189E20): its half-word at +0x1490 is 10, its word at
   +0x340 is set, and either its words at +0x1B8 and +0x2294 are 1 and 2 (the selector then comes
   from func_002A77E0) or the object at +0x2290 has kind 3 or 4 (byte +0x43), the word at +0xC2C
   is clear and func_L00_002B3998 accepts one of two distances (selector 0 or 1). On a trigger it
   places a point on the object's joint (selector + 0x16), moved by the hero's heading (+0x98)
   and raised, and hands it to func_002C3D80. The block's address is taken three times, once for
   each of the tests, the kind switch and the effect, as three separate pointers. */
void func_L02_003DBFD8(void) {
    int found = 0;
    int sel = 0;
    float v[4];
    char *g;
    char *h;
    char *k;

    g = D_00189E20;
    if (*(short *)(g + 0x1490) == 10 && *(int *)(g + 0x340) != 0) {
        if (*(int *)(g + 0x1B8) == 1 && *(int *)(g + 0x2294) == 2) {
            sel = func_002A77E0(2);
            found = 1;
        } else {
            h = D_00189E20;
            switch (*(unsigned char *)(*(char **)(h + 0x2290) + 0x43)) {
            case 3:
                if (*(int *)(h + 0xC2C) == 0) {
                    if (func_L00_002B3998(49.5f) != 0) {
                        found = 1;
                        sel = 0;
                    }
                    if (func_L00_002B3998(17.0f) != 0) {
                        found = 1;
                        sel = 1;
                    }
                }
                break;
            case 4:
                if (*(int *)(h + 0xC2C) == 0) {
                    if (func_L00_002B3998(12.5f) != 0) {
                        found = 1;
                        sel = 0;
                    }
                    if (func_L00_002B3998(1.0f) != 0) {
                        found = 1;
                        sel = 1;
                    }
                }
                break;
            }
        }
    }
    if (found) {
        k = D_00189E20;
        func_0029FCC0(*(char **)(k + 0x2290), sel + 0x16, (char *)v);
        v[0] += func_00283180(*(float *)(k + 0x98)) * 0.15f;
        v[1] += func_00283198(*(float *)(k + 0x98)) * 0.15f;
        v[2] = func_002A89A8(v, 0, 0.5f) + 0.04f;
        func_002C3D80(v, -1, func_002A7878(D_L02_001B0530, D_L02_001B0534), 5250.0f);
    }
}

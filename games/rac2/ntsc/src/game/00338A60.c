#include "include_asm.h"

extern void func_00338810(char *a0);

extern void func_L00_0042E388_1(char *a0);

extern void func_L00_0042E138_1(char *a0);

extern void func_L00_0042BAD0_10(void *p);

extern void func_L00_0042C290_10(void *p);

extern void func_00336320_1(char *a);

extern void func_00336A78_1(char *a);

/* Returns early when a float field reached through a two-level pointer chain is exactly zero; otherwise dispatches on an integer state (0 or 2 selects one of two callees) and then always calls a third callee with the same object pointer. */
void func_00338A60(char *a0)
{
    if (*(float *)(*(int *)(a0 + 4)) == 0.0f)
        return;

    switch (*(int *)(a0 + 456))
    {
    case 0:
        func_00338810(a0);
        break;
    case 2:
        func_L00_0042E388_1(a0);
        break;
    }

    func_L00_0042E138_1(a0);
}


void func_00338F50(char *object)
{
}

/* func_00338F58 is matched in 00338F58.c. */

/* Initialises eight sub-objects at byte offsets 8, 84, 160, 236 (step 76) and 312, 384, 456, 528 (step 72) of the argument with two different helpers and returns the argument. */
void *func_00339008(char *a0)
{
    func_L00_0042BAD0_10(a0 + 8);
    func_L00_0042BAD0_10(a0 + 84);
    func_L00_0042BAD0_10(a0 + 160);
    func_L00_0042BAD0_10(a0 + 236);
    func_L00_0042C290_10(a0 + 312);
    func_L00_0042C290_10(a0 + 384);
    func_L00_0042C290_10(a0 + 456);
    func_L00_0042C290_10(a0 + 528);
    return a0;
}


/* func_00339078 is defined in 00339078-g16.c. */

typedef struct { float x, y; } Float2_00339320;

typedef struct {
    char pad[0x1C8];
    unsigned short count;
    unsigned short unused;
    unsigned int maximum;
    unsigned int levels;
} Statistics_00339320;

extern void func_00335E38(void *, float, float, float, float);
/* The retail callee hands a value back in v0; the declaration says so because v0
   then stays the call's return register and is not handed to the tail's reloads. */
extern int func_002DF1F8(void *, int);

/* The three position pairs live in the program's own data, so they have no address
   of their own: each program reads them from its own bytes. */
extern Float2_00339320 ref_L00_0042EC50_00;
extern Float2_00339320 ref_L00_0042EC50_01;
extern Float2_00339320 ref_L00_0042EC50_02;

extern int D_001B1688 __attribute__((sda));
extern int D_001C51D0;
extern unsigned int D_001A7390 __attribute__((sda));
extern int D_001A79F0 __attribute__((sda));
extern Statistics_00339320 D_0018C750;

/* Places the seven sub-objects of the argument at their world positions, hands the
   current level to the one at +0x258, and updates the statistics block: the visit
   count once the game time has reached 11, the highest level number seen and the
   bit of the level that was reached. */
int func_00339320(char *a0)
{
    Float2_00339320 z0 = {0.0f, 0.0f};
    Float2_00339320 z1 = {0.0f, 0.0f};
    Float2_00339320 z2 = {0.0f, 0.0f};
    Float2_00339320 z3 = {0.0f, 0.0f};
    Float2_00339320 g0 = ref_L00_0042EC50_00;
    Float2_00339320 g1 = ref_L00_0042EC50_01;
    Float2_00339320 g2 = ref_L00_0042EC50_02;

    func_00335E38(a0 + 8, z0.x + (*(float **)(a0 + 4))[0], z0.y + (*(float **)(a0 + 4))[1], 0.0f, 0.0f);
    func_00335E38(a0 + 0x54, z3.x + (*(float **)(a0 + 4))[0], z3.y + (*(float **)(a0 + 4))[1], 0.0f, 0.0f);
    func_00335E38(a0 + 0xA0, z1.x + (*(float **)(a0 + 4))[0], z1.y + (*(float **)(a0 + 4))[1], 0.0f, 0.0f);
    func_00335E38(a0 + 0xEC, z2.x + (*(float **)(a0 + 4))[0], z2.y + (*(float **)(a0 + 4))[1], 0.0f, 0.0f);
    func_00335E38(a0 + 0x138, g0.x + (*(float **)(a0 + 4))[0], g0.y + (*(float **)(a0 + 4))[1], 0.0f, 0.0f);
    func_00335E38(a0 + 0x180, g1.x + (*(float **)(a0 + 4))[0], g1.y + (*(float **)(a0 + 4))[1], 0.0f, 0.0f);
    func_00335E38(a0 + 0x1C8, g2.x + (*(float **)(a0 + 4))[0], g2.y + (*(float **)(a0 + 4))[1], 0.0f, 0.0f);
    func_002DF1F8(a0 + 0x258, D_001C51D0);
    if (D_001B1688 >= 11) {
        if (D_0018C750.count < 0xffff) D_0018C750.count++;
    }
    if (D_001A7390 > D_0018C750.maximum) D_0018C750.maximum = D_001A7390;
    D_0018C750.levels = (D_0018C750.levels | (1u << D_001A79F0)) | 0x80000000u;
    return 0;
}

/* early-returns when a 32-bit field is zero, otherwise calls one helper four times and another three times with fixed byte offsets off the argument pointer. */
void func_00339590(char *a0)
{
    if (*(int *)(a0 + 664) == 0)
        return;
    func_00336320_1(a0 + 8);
    func_00336320_1(a0 + 84);
    func_00336320_1(a0 + 160);
    func_00336320_1(a0 + 236);
    func_00336A78_1(a0 + 312);
    func_00336A78_1(a0 + 384);
    func_00336A78_1(a0 + 456);
}

char *func_003395F8(char *object)
{
    extern void func_0033AB50(void *);
    extern void *func_003361A8(void *);
    func_0033AB50(object + 8);
    func_003361A8(object + 0x2A8);
    return object;
}

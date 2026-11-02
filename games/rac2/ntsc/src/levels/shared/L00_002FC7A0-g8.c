#include "include_asm.h"

/* Measured partial layouts; original type and game roles are unknown. */
typedef unsigned int Rac2Native_3e18d05650488ad4_FallbackU32;

extern Rac2Native_3e18d05650488ad4_FallbackU32 D_001A7424;

extern int D_001A7420_1;

extern int D_L00_001A904C;

extern void func_0028A2F8(int);

extern int D_L00_001A904C;

extern void func_0028A670(int);

void func_L00_0030DC30(int argument)
{
    if (D_L00_001A904C != 0) {
        func_0028A2F8(argument);
    }
}

void func_L00_0030DC58(int argument)
{
    if (D_L00_001A904C != 0) {
        func_0028A670(argument);
    }
}

struct LoadSlots_L00 {
    char pad00[0x34];
    int slot[4];
};

extern struct LoadSlots_L00 D_00152CD0;
extern int ref_L00_00304AE8_00 __attribute__((sda));

/* The level's copy of the load-slot release: same body as func_00294130 with the slot cursor in the level's small data. */
void func_L00_00304AE8(int *p, int a1)
{
    if (a1 != 0) {
        if (p[1] >= 0) {
            D_00152CD0.slot[p[1]] = p[0];
            if (p[1] != ref_L00_00304AE8_00)
                goto out;
            ref_L00_00304AE8_00 = p[1] ^ 1;
        }
    }
out:
    *p = -1;
}

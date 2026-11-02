#include "include_asm.h"

/* The first table column uses a GP address; the other columns use absolute
 * addresses. Keep the compiler at -G8 and assemble with -G64 for this table. */
typedef struct { unsigned char pad[0x9C]; unsigned char bits; } Flags;
extern Flags D_001395B8;
extern int D_001A79F0 __attribute__((section(".sdata")));
extern int D_L00_001A9010 __attribute__((section(".sdata")));
extern int D_L00_001A99F0[] __attribute__((section(".sdata")));
extern int D_L00_001A9A00[] __attribute__((section(".sdata")));
extern int D_L00_001A9A10[16] __attribute__((sda));
extern int D_L00_001A9A14[] __attribute__((section(".sdata")));
extern int D_L00_001A9A18[] __attribute__((section(".sdata")));
extern int D_L00_001A9A1C[] __attribute__((section(".sdata")));
typedef struct { int x[4]; } IndexMap;
extern IndexMap D_L00_001A9A50;
extern void func_L00_00308408(int, int, int, int);
extern int func_003005A0(void);

/* Dispatch a level-specific event once for its corresponding state bit. */
int func_L00_00308538(void)
{
    IndexMap map = D_L00_001A9A50;
    if (D_001A79F0 == 2 && D_L00_001A9010 && ((D_001395B8.bits ^ 1) & 1)) {
        func_L00_00308408(D_L00_001A99F0[0], D_L00_001A99F0[1],
                            D_L00_001A99F0[2], D_L00_001A99F0[3]);
        D_001395B8.bits |= 1;
    } else if (D_001A79F0 == 11 && D_L00_001A9010 && !(D_001395B8.bits & 2)) {
        func_L00_00308408(D_L00_001A9A00[0], D_L00_001A9A00[1],
                            D_L00_001A9A00[2], D_L00_001A9A00[3]);
        D_001395B8.bits |= 2;
    } else if (D_001A79F0 == 7 && func_003005A0() >= 0) {
        int i = func_003005A0();
        int bit = map.x[i];
        if ((unsigned)bit >= 8 || (((D_001395B8.bits >> bit) ^ 1) & 1)) {
            if ((unsigned)bit < 8) D_001395B8.bits |= 1 << bit;
            func_L00_00308408(D_L00_001A9A10[i * 4], D_L00_001A9A14[i * 4],
                                D_L00_001A9A18[i * 4], D_L00_001A9A1C[i * 4]);
        }
    }
    return 0;
}

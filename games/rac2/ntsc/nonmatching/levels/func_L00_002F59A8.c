/* func_L00_002F59A8: M15 2026-10-26: 140/140 bytes, 4 lines off under -O2 -G0 -ffunction-sections (the trap-delay barrier default is in): writing the modulo before the BA6 decrement puts the division guard where retail has it. Left: the two table addresses come out hi,hi,lo,lo where retail builds each %hi/%lo pair in turn (permuter queued).
 */
#include "include_asm.h"
typedef unsigned char u8;
typedef short s16;
typedef int s32;

extern int func_00288E18(int, int);
extern s16 D_L00_001B1B88[];
extern s16 D_L00_001B1B98[];
extern u8 D_L00_001B1BA6 __attribute__((section(".sdata")));
extern u8 D_L00_001B1BA7 __attribute__((section(".sdata")));

void func_L00_002F59A8(void)
{
    if (D_L00_001B1BA6 == 0)
        return;
    if (func_00288E18(D_L00_001B1B88[D_L00_001B1BA7],
                    D_L00_001B1B98[D_L00_001B1BA7]) == 0)
        return;
    D_L00_001B1BA7 = (D_L00_001B1BA7 + 1) % 7;
    D_L00_001B1BA6 = D_L00_001B1BA6 - 1;
}

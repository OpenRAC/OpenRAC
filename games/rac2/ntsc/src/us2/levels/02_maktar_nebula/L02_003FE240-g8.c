#include "include_asm.h"
typedef struct {
    char pad0[0x80];
    char pos[8];
    float f88;
    char pad1[0x2236];
    unsigned char b22C2;
    char pad2[0x22D];
    unsigned char b24F0;
} L02_003F5710_Hero;
extern L02_003F5710_Hero D_00189E20;
typedef int s32;
extern s32 D_L02_001B35B8[] __attribute__((sda));
extern s32 func_00312B58(char *);
extern s32 func_L02_00416800(char *);
extern s32 func_002A77E0(s32);
extern unsigned char D_001395B8[0x100];
extern int D_001A7A08[2];
extern int D_001A79F8;
extern int func_L02_003F9B70(int);
extern int func_L02_00400650(char *, int);
extern void func_00288308(int);
extern void func_002AD860(int);
extern int func_L02_003FCF08(int);
extern int func_L02_003FCDF8(int);
extern void func_L00_0030C7F8(int);
extern void func_0029B3E0(int, int);

/* With a1 == 0 and no count yet for func_L02_003F9B70(0), plays the hero's scene 7 with its
   word at +0x22C8 kept; otherwise picks the scene from a1 (6, 0xB or a random one), adds the
   two scores and starts func_L00_0030C7F8(0x168). Then counts the entry. The unit is built with
   -frac2-gcse-unsplit-edges and -frac2-keep-block-order (config/cflags.txt): the high half of
   D_001395B8 is computed in the entry block, and the switch keeps its `b default` with the
   default body last, as in retail. `+= 1` keeps the counter's address out of the call. */
void func_US2_L02_003FE240(char *a0, int a1) {
    char *state = *(char **)(a0 + 0x68);
    int saved;
    char *hero;
    int v;

    if (a1 == 0 && D_001395B8[func_L02_003F9B70(0)] == 0) {
        hero = (char *)&D_00189E20;
        saved = *(int *)(hero + 0x22C8);
        func_L02_00400650(a0, 7);
        func_00288308(0x26);
        func_002AD860(0x26);
        *(int *)(hero + 0x22C8) = saved;
    } else {
        switch (a1) {
        case 6:
            func_L02_00400650(a0, 8);
            break;
        case 0xB:
            func_L02_00400650(a0, 9);
            break;
        default:
            func_L02_00400650(a0, func_002A77E0(5) + 2);
            break;
        }
        D_001A7A08[1] = D_001A7A08[1] + func_L02_003FCF08(a1);
        v = func_L02_003FCDF8(a1);
        *(int *)(state + 0x110) = v;
        D_001A79F8 = D_001A79F8 + v;
        func_L00_0030C7F8(0x168);
    }
    D_001395B8[func_L02_003F9B70(a1)] += 1;
    func_0029B3E0(0, -1);
}

#include "common.h"
#include "structs.h"

/*
 * bmain.cpp in the original source; text 0x1E9808-0x1E9E70.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/DECOMP_PROGRESS.md. Compiled as C for now.
 */

extern int D_0015EE80 MACRO_ADDR;
extern int D_0015EFD8 MACRO_ADDR;
extern int D_0015F6E8 MACRO_ADDR;
extern int D_0015EF88 MACRO_ADDR;
typedef struct {
    int b;
    int a;
} LevelLoad;
typedef struct {
    char pad0[0x1A78];
    LevelLoad normal[4];
    LevelLoad alt[4];
} Globals137C80;
extern Globals137C80 D_00137C80_g __asm__("D_00137C80");
extern char D_0013E650[];
extern char D_0013D390[];
extern char D_001941C0[];
extern void func_00118D80(int);
extern void func_0022EFE8(void);
extern void func_00216D88(void);
extern int func_001F98C0(int);
extern void func_001F4E08(int);
extern void func_0012EF48(int);
extern void func_00209E68(void);
extern void func_0023B670(int, int, int, int, int);
extern void func_00120F30(int);
extern void func_00122598(int);
extern void func_00120858(int, int);
extern void func_0012F308(void);
extern void func_00123168(void (*)(void));
extern void func_00201348(int, int, int, int, int, int);

/* Start level `level`: pick its two load parameters from the level
   table in D_00137C80 (the alternate set while D_0015EE80 is set), stop
   sound and wait for the loader (func_00209E68) to go idle, then load it
   (func_0023B670) and reset the display state. */
void func_001E9808(int level) {
    int a;
    int b;
    int t;

    if (level < 0) {
        return;
    }
    if (D_0015EE80 != 0) {
        b = D_00137C80_g.alt[level].b;
        a = D_00137C80_g.alt[level].a;
    } else {
        b = D_00137C80_g.normal[level].b;
        a = D_00137C80_g.normal[level].a;
    }
    D_0015EFD8 = 2;
    D_0013E650[0x6B] |= 8;
    func_00118D80(0);
    sound_StopAllSounds();
    music_Stop();
    FadeToBlack(func_001F98C0(12));
    D_0015F6E8 = 1;
    func_00118D80(0);
    sound_StopAllSounds();
    music_Stop();
    func_0012EF48(0);
    for (;;) {
        char *ld = D_0013D390;
        if (*(int *)(ld + 0xDC) < 3 && *(int *)(ld + 0xE4) < 0) {
            break;
        }
        func_00209E68();
    }
    {
        char *g = D_001941C0;
        t = *(int *)(g + 0x1C);
    }
    func_0023B670(b, a, t + 0x100000, t + 0x400000, 0);
    func_00120F30(0);
    func_00122598(0);
    func_00120858(0, 0);
    func_00123168(func_0012F308);
    Hud_sendTexture(0x1000000, D_0015EF88, 0x1B, 6, 6, 1);
    D_0015EFD8 = 0;
    FadeToBlack(4);
    D_0015F6E8 = 0;
    D_0013E650[0x6B] |= 0x10;
}

INCLUDE_ASM("asm/nonmatchings/text", func_001E99D8);

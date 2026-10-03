#include "common.h"
#include "structs.h"

/*
 * drawquad.cpp in the original source; text 0x1F7C60-0x1F9810.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/DECOMP_PROGRESS.md. Compiled as C for now.
 */

/* Declarations in scope here before the split. */
extern char D_0013E650[];
extern int D_0015F694;
extern void func_001F9A98(void *, void *, int);
extern char D_00189310[];
extern char D_001899D0[];
extern void *D_001871C0 NOT_SDA;
typedef struct {
    char unk_00[8];
    void (*fn_08)(void *);
    char unk_0C[4];
    void (*fn_10)(void *);
} DispatchRec;
extern DispatchRec D_001E8F80[];
extern int D_0018A3B0[];
extern void func_001F99B0();
extern void func_001F2BC8(void);
extern int D_0018C434 NOT_SDA;
extern char D_001940C0[];
extern long D_00151888[3];
extern int D_0015F6FC;
extern short D_0015F534;
extern void func_001FB530(void);
extern void func_001F3D78(void);
extern int D_0015F564;
extern int D_0018DD40[];
extern int D_0018DC40[];
extern short D_0015F59C;
extern int func_001F65B0(unsigned char *arg0, int arg1, void *arg2);
extern unsigned char D_001DF3D0[];
extern unsigned char D_001DF770[];
extern unsigned char D_001DFB10[];
extern void func_001F6668(void *, void *, void *, void *, void *, int,
                          unsigned char *);
extern int func_001F6600(unsigned char *, int);
extern int func_001F6620(unsigned char *, int);
extern int func_001F4868(int);
extern void func_001F7070(void *, void *, void *, void *, int, unsigned char *);
extern void func_001FB498(void);
extern void func_001F3008(void);
extern void func_001F3140(void);
extern int D_0018E840[];

ASM_FUNC("asm/handwritten/text", func_001F7C60);

INCLUDE_ASM("asm/nonmatchings/text", func_001F7DD8);

INCLUDE_ASM("asm/nonmatchings/text", func_001F7E98);

ASM_FUNC("asm/handwritten/text", func_001F7EF8);

ASM_FUNC("asm/handwritten/text", func_001F84AC);

ASM_FUNC("asm/handwritten/text", func_001F852C);

ASM_FUNC("asm/handwritten/text", func_001F856C);

ASM_FUNC("asm/handwritten/text", func_001F8B6C);

ASM_FUNC("asm/handwritten/text", func_001F91B8);

INCLUDE_ASM("asm/nonmatchings/text", func_001F9478);

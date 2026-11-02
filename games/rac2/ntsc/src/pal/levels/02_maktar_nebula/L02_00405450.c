#include "include_asm.h"

typedef short s16;
extern s16 func_L02_004024A0(char *);
extern char *D_L02_001B35A8 __attribute__((sda));
extern void func_003320F0(char *);
extern void func_00333048(char *);
extern void func_L02_00414D48(char *);
extern void func_L02_003F0628(char *);
extern void func_L02_00392C70(char *);
extern void func_L02_003948D0(char *);
extern void func_L02_003AA650(char *);
extern void func_L02_003DED08(char *);
extern void func_0029F338(char *);

/* Walks the list of objects at D_L02_001B35A8 (linked through +0x28) and hands each one to the
   handler of its kind, the half-word at +0xAA. */
void func_PAL_L02_00405450(void) {
    char *e;

    for (e = D_L02_001B35A8; e != 0; e = *(char **)(e + 0x28)) {
        switch (*(s16 *)(e + 0xAA)) {
        case 0xE7D:
            func_003320F0(e);
            break;
        case 0xEB0:
            func_00333048(e);
            break;
        case 0xCB2:
            func_L02_00414D48(e);
            break;
        case 0xB02:
            func_L02_003F0628(e);
            break;
        case 0x1366:
            func_L02_00392C70(e);
            break;
        case 0x1367:
            func_L02_003948D0(e);
            break;
        case 0x326:
            func_L02_003AA650(e);
            break;
        case 0xA5C:
        case 0x1192:
            func_L02_003DED08(e);
            break;
        case 0xAE0:
        case 0xAE1:
        case 0xAE2:
        case 0xAEC:
        case 0xB06:
        case 0xB09:
        case 0xCC0:
        case 0xCC1:
        case 0xCC2:
        case 0xCC3:
        case 0xCC4:
        case 0xCC5:
        case 0xCC6:
        case 0xCC7:
        case 0xCC8:
        case 0xCC9:
        case 0xCDB:
            func_0029F338(e);
            break;
        }
    }
}


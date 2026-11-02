#include "include_asm.h"
typedef unsigned char u8;
typedef signed short s16;
typedef signed int s32;
extern s32 D_L16_001B18F0 __attribute__((sda));
extern s32 D_L16_001B18E0 __attribute__((sda));
extern s32 D_L16_001B1704 __attribute__((sda));
extern char D_L16_00189E20[] __attribute__((nosda));
extern s32 func_00283CF0(float);
extern s32 func_002A9640(void *, s32, s32);
extern s32 func_00282978(void *);
extern s32 func_002B3BA8(void *, s32, float, float, s32, s32);
extern char D_L16_001A63A8[] __attribute__((nosda));
extern char D_L16_001B1850[] __attribute__((nosda));
extern char D_L16_001B1870[] __attribute__((nosda));
extern void func_0026FEB8(void *);
extern void func_002B6D28(void);
extern void func_00282948(void *);
extern void func_L16_00412C58(void *, s32, s32);
extern s32 func_002A7820(s32, s32);

/* Raises the level's one-shot flag while the counter is low, then hands the sub-object
   its tail record. */
void func_PAL_L16_00417C58(u8 *arg0)
{
    u8 *s1;
    u8 *p;
    u8 *q;

    s1 = *(u8 **)(arg0 + 0x68);
    p = D_L16_00189E20;
    if (*(s32 *)(p + 0x2294) == 0x39) {
        if (*(s32 *)(p + 0x1B8) == 0) {
            func_0026FEB8(D_L16_001B1850);
            func_L16_00412C58(arg0, 0x1C, 0x20);
            D_L16_001B1704++;
        }
    }
    q = D_L16_001A63A8;
    if (*(s16 *)(q + 0x72) == 3 && *(s16 *)(q + 0x6C) < 0x1770) {
        func_0026FEB8(D_L16_001B1870);
        func_002B6D28();
    }
    func_00282948(s1 + 0x350);
}

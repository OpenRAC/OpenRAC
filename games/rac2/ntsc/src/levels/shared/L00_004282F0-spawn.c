#include "common.h"

extern void *func_00270280(int a0);
extern void func_00270598(void *a0);
extern void func_L00_00428000(void *a0, void *a1);
extern void func_L00_002D7958(void *a0, void *a1, f32 f12);
extern void func_L00_002D7940(void *a0);
extern void func_L00_00427C78(void *a0);

/* Each helper accesses this five-float prefix only; the complete global
 * object extent remains unknown. GP access is proved independently by the
 * retail call sites, rather than inferred from this partial view size. */
typedef struct { f32 values[5]; } BlendPrefix;
extern BlendPrefix D_L00_001B15E0 __attribute__((sda));
extern BlendPrefix D_L00_001B15F8 __attribute__((sda));
extern BlendPrefix D_L00_001B1610 __attribute__((sda));

extern s16 D_L00_001B8F10 __attribute__((nosda));

void *func_L00_004282F0(u8 *a0) {
    u8 *s0 = a0;
    u8 *s2;
    u8 *p;
    f32 f20;

    s2 = func_00270280(6);
    D_L00_001B8F10 = 0;
    func_00270598(s2);
    func_L00_00428000(s2, s0);
    p = *(u8 **)(s2 + 0x70);
    qcopy(p + 0xB0, p + 0x80);
    f20 = 0.0f;
    func_L00_002D7958(p + 0x14, &D_L00_001B15E0, f20);
    func_L00_002D7958(s2 + 0x50, p + 0x14, f20);
    func_L00_00427C78(s2);
    *(int *)(p + 0x9C) = 1;
    func_L00_002D7958(p + 0x28, &D_L00_001B15F8, f20);
    func_L00_002D7958(p + 0x3C, &D_L00_001B1610, f20);
    func_L00_002D7940(p + 0x50);
    return s2;
}

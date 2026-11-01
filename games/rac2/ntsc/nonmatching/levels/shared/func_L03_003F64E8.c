/* from src/levels/03_endako/L03_003F09C0.c: C that compiles but does not match */
/* The level's two floats are read through $gp (0x20A0 and 0x20A4), so the
 * candidate needs a `-g8.c` unit; the constants it subtracts are the boot's
 * D_0012EA0 and D_0012EA4, and the pointer chain is D_001B59D0 and its word at
 * +0x70.  The `bnel` at 0x003F6514 carries the load of the state's word in its
 * slot, which the plain source does not reach. */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed int s32;
typedef float f32;

extern void func_2D7B78(s32 a0);
extern f32 func_2EF670(f32 a, f32 b);
extern f32 func_2F0000(f32 a, f32 b);
extern f32 func_2F0048(f32 a, f32 b);
extern void func_31F360(void *a0, void *a1, void *a2, f32 a3, f32 a4, f32 a5, f32 a6);

/* The level's own two floats. */
extern f32 D_L03_001B1090;
extern f32 D_L03_001B1094;

/* The boot's block and its pointer word. */
extern f32 D_0012EA0;
extern f32 D_0012EA4;
extern u32 D_L03_001B59D0;

void func_L03_003F64E8(u8 *entity) {
    u8 *state;
    u8 *obj;
    u8 *s0;
    f32 f0;
    f32 f12;
    f32 f13;
    f32 f14;
    f32 f15;
    f32 first;
    f32 local;

    if (entity[0x95] < 2)
        return;
    state = *(u8 **)(entity + 0x68);
    obj = *(u8 **)(state + 0x2C8);
    if (obj == 0) {
        func_2D7B78(5);
        obj = *(u8 **)(state + 0x2C8);
    }
    *(f32 *)(obj + 0) = D_L03_001B1090;
    *(f32 *)(obj + 4) = D_L03_001B1094;

    f12 = *(f32 *)(entity + 0x10) - D_0012EA0;
    f13 = *(f32 *)(entity + 0x14) - D_0012EA4;
    first = func_2EF670(f12, f13);

    s0 = *(u8 **)(D_L03_001B59D0 + 0x70);
    f12 = *(f32 *)(s0 + 0x160);
    f13 = *(f32 *)(s0 + 0x164);
    f0 = func_2EF670(f12, f13);

    f13 = 3.14159274f;
    f0 = func_2F0000(f0, f13);
    f0 = func_2F0048(first, f0);

    f13 = 0.0034907f;
    f14 = f13;
    f15 = 0.20939991f;
    local = f0;
    f12 = 0.0f;
    func_31F360(&local, state + 0xCC, 0, f12, f13, f14, f15);

    *(f32 *)(s0 + 0x1C) = 1.0f;
    *(f32 *)(s0 + 0x14) = -*(f32 *)(state + 0xCC);
}

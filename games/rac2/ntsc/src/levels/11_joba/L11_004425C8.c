typedef float f32;
typedef int s32;
typedef unsigned int u32;
typedef unsigned char u8;

extern s32 D_L11_001CAE40;

extern u8 *D_L11_001B368C __attribute__((sda));

extern u8 D_001395B8[];

extern s32 D_L11_001AA170 __attribute__((sda));

extern u8 D_00189EA0[];

extern void func_L11_00445D28(u8 *, s32);

extern void func_L11_004411C8(u8 *);

extern void func_002A7750(u8 *, s32, s32);

extern void func_L00_002D7140(s32);

extern void func_L00_00303530(s32, void *);

extern void func_00282C88(void *);

extern s32 func_L11_0043E108(s32);

extern void func_L00_002B9D60(s32, s32, s32, s32);

extern void func_002DF3F8(void *, void *);

extern void func_L01_00323310(void *, void *);

/* Restarts the actor's state machine, releases the level's helper objects and hands
   the state's slots to the two level handlers. */
void func_L11_004425C8(u8 *obj) {
    u8 *state = *(u8 **)(obj + 0x68);
    f32 buf[4];

    func_L11_00445D28(obj, 0x20);
    func_002A7750(obj, 1, -1);
    if (*(s32 *)(state + 0x170) != 0) {
        func_L00_002D7140(*(s32 *)(state + 0x170));
        *(s32 *)(state + 0x170) = 0;
    }
    if (D_L11_001CAE40 != 0) {
        func_L00_00303530(0, *(void **)(state + 0x2F4));
    }
    func_00282C88(buf);
    if (D_001395B8[func_L11_0043E108(1)] != 0) {
        buf[2] = 2.16f;
        func_L00_002B9D60((s32)(D_L11_001B368C + (*(s32 *)(state + 0x15C) << 7)) + 0x30, (s32)buf, 0, 1);
    } else {
        buf[2] = -2.52f;
        func_L00_002B9D60((s32)(D_L11_001B368C + (*(s32 *)(state + 0x158) << 7)) + 0x30, (s32)buf, 0, 1);
    }
    func_002DF3F8(D_00189EA0, D_00189EA0 + 0x10);
    func_L11_004411C8(obj);
    func_L01_00323310(*(void **)(state + 0x2F4), *(void **)(state + 0x2FC));
    D_L11_001AA170 = -1;

}

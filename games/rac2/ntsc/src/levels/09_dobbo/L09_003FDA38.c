/* Measures the two path lists' segment lengths and restarts both followers on the first list when the speed is set. */
typedef unsigned char u8;
typedef unsigned int u32;
typedef signed int s32;
typedef float f32;

extern u32 D_L09_001FE6C0[];
extern f32 func_002EA630(u8 *a0, u8 *a1);
extern void func_0033EB48(s32 a0, s32 a1, s32 a2, s32 a3, s32 a4, f32 f12);

void func_L09_003FDA38(u8 *arg0) {
    u8 *s3 = *(u8 **)(arg0 + 0x68);
    u8 *list;
    s32 i;
    f32 f;

    list = (u8 *)D_L09_001FE6C0[*(u32 *)(s3 + 0x318)];
    *(f32 *)(s3 + 0x38C) = 0.0f;
    for (i = 0; i < *(s32 *)list; i++) {
        f = func_002EA630(list + (i * 0x10 + 0x10),
                          list + (((i + 1) % *(s32 *)list) * 0x10 + 0x10));
        *(f32 *)(list + (i * 0x10 + 0x10) + 0xC) = f;
        if (i != *(s32 *)list - 1)
            *(f32 *)(s3 + 0x38C) = *(f32 *)(s3 + 0x38C) + f;
    }
    list = (u8 *)D_L09_001FE6C0[*(u32 *)(s3 + 0x314)];
    *(f32 *)(s3 + 0x3B0) = 0.0f;
    for (i = 0; i < *(s32 *)list; i++) {
        f = func_002EA630(list + (i * 0x10 + 0x10),
                          list + (((i + 1) % *(s32 *)list) * 0x10 + 0x10));
        *(f32 *)(list + (i * 0x10 + 0x10) + 0xC) = f;
        if (i != *(s32 *)list - 1)
            *(f32 *)(s3 + 0x3B0) = *(f32 *)(s3 + 0x3B0) + f;
    }
    if (*(f32 *)(s3 + 0x430) != 0.0f) {
        u8 *path = (u8 *)D_L09_001FE6C0[*(u32 *)(s3 + 0x318)];
        *(u32 *)(s3 + 0x324) = 0;
        *(f32 *)(s3 + 0x3EC) = 0.0f;
        func_0033EB48((s32)path, (s32)(arg0 + 0x10), (s32)(s3 + 0x324), (s32)(s3 + 0x3EC),
                      *(short *)(s3 + 0x32A), *(f32 *)(s3 + 0x430) * *(f32 *)(s3 + 0x38C));
        *(f32 *)(s3 + 0x32C) = 0.0f;
        *(u32 *)(s3 + 0x320) = 0;
        func_0033EB48((s32)path, (s32)(arg0 + 0x10), (s32)(s3 + 0x320), (s32)(s3 + 0x32C),
                      *(short *)(s3 + 0x32A), *(f32 *)(s3 + 0x430) * *(f32 *)(s3 + 0x3B0));
    } else {
        *(u32 *)(s3 + 0x320) = 0;
        *(u32 *)(s3 + 0x324) = 0;
        *(f32 *)(s3 + 0x32C) = 0.0f;
        *(f32 *)(s3 + 0x3EC) = 0.0f;
    }
}

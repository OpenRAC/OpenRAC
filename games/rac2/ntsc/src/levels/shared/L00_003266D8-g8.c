typedef int s32;

extern s32 D_L00_001A8FB4;
extern s32 D_L00_001A8FB8;
extern s32 D_L00_001A8FB0;
extern s32 ref_L00_003266D8_00 __attribute__((sda));
extern s32 ref_L00_003266D8_01 __attribute__((sda));
extern s32 D_L00_001B2088 __attribute__((sda));
extern s32 D_L00_001B2084 __attribute__((sda));
extern unsigned char D_L00_002F7530[];
extern unsigned char D_L00_002F7568[];
extern unsigned char D_L00_002F7588[];
extern s32 func_0028B3D0(s32, s32, void *, void *, void *, s32, s32);
extern s32 func_L00_002F7298(s32, s32);

void func_L00_003266D8(void) {
    if (D_L00_001A8FB4 == 0) {
        s32 n = D_L00_001A8FB8 + 1;

        ref_L00_003266D8_00 = n;
        if (n > 10)
            ref_L00_003266D8_00 = 10;
    } else {
        D_L00_001A8FB8 = 0;
    }
    if (D_L00_001A8FB0 != 0) {
        ref_L00_003266D8_01 = D_L00_001A8FB0 - 1;
        if (ref_L00_003266D8_01 == 0)
            D_L00_001A8FB4 = 0;
    }
    if (D_L00_001A8FB4 == 0 || D_L00_001B2088 == 0) {
        D_L00_001B2084 = -1;
    } else {
        if (D_L00_001B2084 == -1)
            D_L00_001B2084 = func_0028B3D0(12, 0, D_L00_002F7530, D_L00_002F7568,
                                           D_L00_002F7588, 0, 0);
        else
            func_L00_002F7298(D_L00_001B2084, 10);
    }
}

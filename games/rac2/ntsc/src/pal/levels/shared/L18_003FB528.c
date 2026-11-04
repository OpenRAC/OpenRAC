#include "include_asm.h"

#include "common.h"
typedef unsigned char BatchA_A18_u8;
typedef int BatchA_A18_s32;
extern BatchA_A18_u8 D_0018C0D4;
extern BatchA_A18_s32 ref_L18_002E4090_00(BatchA_A18_s32 a0, BatchA_A18_s32 a1);
extern BatchA_A18_s32 func_002A8AF0(BatchA_A18_s32 a0, BatchA_A18_s32 a1, BatchA_A18_s32 a2);
extern BatchA_A18_s32 D_001FE480[];
extern BatchA_A18_s32 D_001FE480[];
extern BatchA_A18_s32 func_L18_003F8E40(BatchA_A18_u8 *s);
extern BatchA_A18_s32 func_002A77E0(BatchA_A18_s32 a0);
extern BatchA_A18_s32 func_L18_003F75E8(BatchA_A18_s32 a0, BatchA_A18_u8 *a1);
extern BatchA_A18_s32 func_L09_0033E690(BatchA_A18_u8 *a0, BatchA_A18_s32 a1, float f12);
extern BatchA_A18_s32 func_L09_00329178(BatchA_A18_u8 *a0, BatchA_A18_s32 a1,
                                        BatchA_A18_s32 a2, BatchA_A18_s32 a3);
extern BatchA_A18_s32 func_L09_0033E658(void *a0);

/* runs the 0x1E gate and, when it clears, the record query into a local buffer;
   answers whether the query found anything */
BatchA_A18_s32 func_PAL_L18_003FB528(BatchA_A18_u8 *a0) {
    BatchA_A18_u8 buf[16];
    BatchA_A18_s32 p;

    if (func_002A77E0(0x19) == 0) {
        p = *(BatchA_A18_s32 *)(a0 + 0x68);
        return func_L18_003F75E8(*(BatchA_A18_s32 *)((char *)p + 0x25C), buf) != 0;
    }
    return 0;
}


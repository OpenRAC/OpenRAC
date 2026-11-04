#include "include_asm.h"
#include "common.h"
extern s16 D_001A63A8[];
extern void func_002B7578(void);
extern void func_00133230(void);
extern void func_00132028(void);
extern void func_00133220(void);
extern void func_00282A38(s32);

s16 func_PAL_002B8898(s32 arg0) {
    s16 *q;

    if (arg0 != 0) {
        while (1) {
            func_002B7578();
            func_00133230();
            func_00132028();
            func_00133220();
            if (D_001A63A8[2] == 0) {
                break;
            }
            func_00282A38(10000);
            if (D_001A63A8[2] == 0) {
                break;
            }
        }
    } else {
        func_002B7578();
        func_00133230();
        func_00132028();
        func_00133220();
    }
    q = D_001A63A8;
    return q[2];
}

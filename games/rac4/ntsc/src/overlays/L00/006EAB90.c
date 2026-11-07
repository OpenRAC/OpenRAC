/* cflags: -mno-split-addresses */
#include "common.h"

s32 func_006EBB70();
s32 func_L00_006EAC00();
s32 func_L00_006EAF38();

void func_L00_006EAB90(s32 arg0, s32 arg1) {
    switch (arg1) {
    case 0:
        func_L00_006EAC00();
        return;
    case 1:
        func_L00_006EAF38();
        break;
    case 2:
        func_006EBB70();
        break;
    }
}

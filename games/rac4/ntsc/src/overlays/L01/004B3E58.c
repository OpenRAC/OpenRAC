/* cflags: -mno-split-addresses */
#include "common.h"

s32 func_L01_004B3070();
s32 func_L01_004B3830();

s32 func_L01_004B3E58(s32 arg0, s32 arg1) {
    switch (arg1) {
    case 2:
        func_L01_004B3070();
        break;
    case 4:
        func_L01_004B3830();
        break;
    }
    return 1;
}

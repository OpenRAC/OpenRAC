#include "common.h"

s32 func_00127000();
s32 func_00127010();
s32 func_0012D098();

void func_0012D058(void) {
    if (func_00127000() == 0x02000000) {
        func_0012D098();
        return;
    }
    func_00127010();
}

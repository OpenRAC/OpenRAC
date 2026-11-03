/* Data owned by src/core/kernel/alarm.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

u32 alarm_kernel_patch_code[] DATA_SECTION(D_00130BF0) = {
#include "data_inc/D_00130BF0.inc"
};

u32 alarm_scratchpad_patch_code[] DATA_SECTION(D_00131330) = {
#include "data_inc/D_00131330.inc"
};

#include "data_inc/D_00131358.refs"
int alarm_syscall_patch_table[] DATA_SECTION(D_00131358) = {
#include "data_inc/D_00131358.inc"
};

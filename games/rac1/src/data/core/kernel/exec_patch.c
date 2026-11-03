/* Data owned by src/core/kernel/exec_patch.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

u32 osd_kernel_patch_code[] DATA_SECTION(D_00130428) = {
#include "data_inc/D_00130428.inc"
};

#include "data_inc/D_00130BD0.refs"
int osd_syscall_patch_table[] DATA_SECTION(D_00130BD0) = {
#include "data_inc/D_00130BD0.inc"
};

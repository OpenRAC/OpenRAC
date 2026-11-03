/* Data owned by src/core/sif/reset_iop.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

u32 tlb_functions_kernel_code[] DATA_SECTION(D_0012FDB8) = {
#include "data_inc/D_0012FDB8.inc"
};

int tlb_saved_syscall_3_entry[2] DATA_SECTION(D_00130130) = {0};

#include "data_inc/D_00130138.refs"
int tlb_functions_syscall_table[] DATA_SECTION(D_00130138) = {
#include "data_inc/D_00130138.inc"
};

u32 iop_reboot_udnl_prefix[] DATA_SECTION(D_00152A60) = {
#include "data_inc/D_00152A60.inc"
};

u32 iop_reboot_param_too_long_msg[] DATA_SECTION(D_00152A70) = {
#include "data_inc/D_00152A70.inc"
};

/* Data owned by src/core/kernel/kprintf.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

extern void serialPutchar();
extern u32 D_00152A48[];

int deci2_putchar_count DATA_SECTION(D_0012FCFC) = {0};

void *kprintf_putchar_hook DATA_SECTION(D_0012FD00) = serialPutchar;

int sif_cmd_init_done DATA_SECTION(D_0012FD04) = {0};

int RpcCommandState[] DATA_SECTION(D_0012FD08) = {
#include "data_inc/D_0012FD08.inc"
};

s32 fs_async_semaphores[] DATA_SECTION(D_0012FD10) = {
#include "data_inc/D_0012FD10.inc"
};

u32 fs_receive_bank DATA_SECTION(D_0012FD90) = 0;

int FsResetState DATA_SECTION(D_0012FD94) = {0};

s32 fs_double_buffered DATA_SECTION(D_0012FD98) = {0};

int fs_call_sema DATA_SECTION(D_0012FD9C) = {
#include "data_inc/D_0012FD9C.inc"
};

int fs_iob_sema DATA_SECTION(D_0012FDA0) = {
#include "data_inc/D_0012FDA0.inc"
};

int fs_iob_sema_2 DATA_SECTION(D_0012FDA4) = {
#include "data_inc/D_0012FDA4.inc"
};

void *fs_expected_version DATA_SECTION(D_0012FDA8) = D_00152A48;

int iop_heap_bind_state DATA_SECTION(D_0012FDAC) = {
#include "data_inc/D_0012FDAC.inc"
};

u32 D_00152A48[] DATA_SECTION(D_00152A48) = {
#include "data_inc/D_00152A48.inc"
};

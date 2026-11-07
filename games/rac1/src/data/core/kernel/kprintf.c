/* Data owned by src/core/kernel/kprintf.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

extern void serialPutchar();
extern u32 D_00152A48[];

int D_0012FCFC DATA_SECTION(D_0012FCFC) = {0};

void *kprintf_putchar_hook DATA_SECTION(D_0012FD00) = serialPutchar;

int sif_cmd_init_done DATA_SECTION(D_0012FD04) = {0};

int RpcCommandState[] DATA_SECTION(D_0012FD08) = {
#include "data_inc/D_0012FD08.inc"
};

s32 D_0012FD10[] DATA_SECTION(D_0012FD10) = {
#include "data_inc/D_0012FD10.inc"
};

u32 D_0012FD90 DATA_SECTION(D_0012FD90) = 0;

int FsResetState DATA_SECTION(D_0012FD94) = {0};

s32 fs_double_buffered DATA_SECTION(D_0012FD98) = {0};

int D_0012FD9C DATA_SECTION(D_0012FD9C) = {
#include "data_inc/D_0012FD9C.inc"
};

int D_0012FDA0 DATA_SECTION(D_0012FDA0) = {
#include "data_inc/D_0012FDA0.inc"
};

int D_0012FDA4 DATA_SECTION(D_0012FDA4) = {
#include "data_inc/D_0012FDA4.inc"
};

void *D_0012FDA8 DATA_SECTION(D_0012FDA8) = D_00152A48;

int iop_heap_bind_state DATA_SECTION(D_0012FDAC) = {
#include "data_inc/D_0012FDAC.inc"
};

u32 D_00152A48[] DATA_SECTION(D_00152A48) = {
#include "data_inc/D_00152A48.inc"
};

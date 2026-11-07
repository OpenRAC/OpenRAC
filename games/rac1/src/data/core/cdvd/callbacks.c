/* Data owned by src/core/cdvd/callbacks.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

int cd_callback_thread_id DATA_SECTION(D_001313D4) = {0};

int D_001313D8 DATA_SECTION(D_001313D8) = {0};

int D_001313DC DATA_SECTION(D_001313DC) = {0};

int cd_callback_sema DATA_SECTION(D_001313E0) = {
#include "data_inc/D_001313E0.inc"
};

int cd_poweroff_installing DATA_SECTION(D_001313E4) = {0};

int cd_ncmd_rpc_bound DATA_SECTION(D_001313F8) = {
#include "data_inc/D_001313F8.inc"
};

int cd_poweroff_installed DATA_SECTION(D_001313FC) = {
#include "data_inc/D_001313FC.inc"
};

int D_00131400 DATA_SECTION(D_00131400) = {
#include "data_inc/D_00131400.inc"
};

s32 cd_disk_ready_rpc_bound DATA_SECTION(D_00131404) = {
#include "data_inc/D_00131404.inc"
};

int cd_scmd_rpc_bound DATA_SECTION(D_00131408) = {
#include "data_inc/D_00131408.inc"
};

int D_0013140C DATA_SECTION(D_0013140C) = {
#include "data_inc/D_0013140C.inc"
};

int D_00131410 DATA_SECTION(D_00131410) = {0};

u32 D_00131418[10] DATA_SECTION(D_00131418) = {0};

int cd_ncmd_recv_buffer[32] DATA_SECTION(D_00131440) = {0};

int D_00132A00[258] DATA_SECTION(D_00132A00) = {0};

char cd_ncmd_sema_fail_msg[] DATA_SECTION(D_00152FB0) =
#include "data_inc/D_00152FB0.inc"
    ;

char D_00152FD8[] DATA_SECTION(D_00152FD8) =
#include "data_inc/D_00152FD8.inc"
    ;

u32 D_00152FF0[] DATA_SECTION(D_00152FF0) = {
#include "data_inc/D_00152FF0.inc"
};

u32 D_00153000[] DATA_SECTION(D_00153000) = {
#include "data_inc/D_00153000.inc"
};

char cd_scmd_sema_fail_msg[] DATA_SECTION(D_00153010) =
#include "data_inc/D_00153010.inc"
    ;

char D_00153038[] DATA_SECTION(D_00153038) =
#include "data_inc/D_00153038.inc"
    ;

char D_00153050[] DATA_SECTION(D_00153050) =
#include "data_inc/D_00153050.inc"
    ;

char cd_exit_message[] DATA_SECTION(D_00153070) =
#include "data_inc/D_00153070.inc"
    ;

char D_00153080[] DATA_SECTION(D_00153080) =
#include "data_inc/D_00153080.inc"
    ;

char D_00153090[] DATA_SECTION(D_00153090) =
#include "data_inc/D_00153090.inc"
    ;

u32 D_001530B0[] DATA_SECTION(D_001530B0) = {
#include "data_inc/D_001530B0.inc"
};

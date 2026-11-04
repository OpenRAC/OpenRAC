/* Data owned by src/core/mc/memory_card.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

int mc_current_command[] DATA_SECTION(D_00132EA8) = {
#include "data_inc/D_00132EA8.inc"
};

int pad2_initialized[2] DATA_SECTION(D_00132ED0) = {0};

#include "data_inc/D_00132ED8.refs"
int pad2_last_dma_counters[] DATA_SECTION(D_00132ED8) = {
#include "data_inc/D_00132ED8.inc"
};

u32 str_mc_bind_error[] DATA_SECTION(D_00153550) = {
#include "data_inc/D_00153550.inc"
};

char str_mc_old_mcserv[] DATA_SECTION(D_00153568) =
#include "data_inc/D_00153568.inc"
    ;

u32 str_mc_old_mcman[] DATA_SECTION(D_00153590) = {
#include "data_inc/D_00153590.inc"
};

u32 str_dbc_bind_failed[] DATA_SECTION(D_001535D8) = {
#include "data_inc/D_001535D8.inc"
};

u32 str_dbc_version_mismatch[] DATA_SECTION(D_001535F0) = {
#include "data_inc/D_001535F0.inc"
};

char str_dbc_version_pair_fmt[] DATA_SECTION(D_00153618) =
#include "data_inc/D_00153618.inc"
    ;

u32 str_dbc_sif_dma_addr_fmt[] DATA_SECTION(D_00153640) = {
#include "data_inc/D_00153640.inc"
};

char str_dbc_set_work_addr_error[] DATA_SECTION(D_00153658) =
#include "data_inc/D_00153658.inc"
    ;

u32 str_dbc_create_socket_error[] DATA_SECTION(D_00153678) = {
#include "data_inc/D_00153678.inc"
};

u32 str_dbc_get_dep_number_error[] DATA_SECTION(D_001536B8) = {
#include "data_inc/D_001536B8.inc"
};

u32 str_dbc_receive_data_error[] DATA_SECTION(D_001537A0) = {
#include "data_inc/D_001537A0.inc"
};

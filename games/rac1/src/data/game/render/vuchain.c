/* Data owned by src/game/render/vuchain.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

int vu1_chain_buffers[2] DATA_SECTION(D_00160FF8) = {0};

int previous_render_packet_cursor DATA_SECTION(D_00161004) = {0};

int vu1_chain_buffer_index DATA_SECTION(D_00161010) = {0};

int vu1_chain_peak_bytes DATA_SECTION(D_00161014) = {0};

int vif1_dma_handler_id DATA_SECTION(D_00161018) = {0};

int vu1_halt_handler_id DATA_SECTION(D_0016101C) = {0};

#include "data_inc/D_001DE338.refs"
int vu_chain_table[] DATA_SECTION(D_001DE338) = {
#include "data_inc/D_001DE338.inc"
};

u32 gs_regs_normal_packet[] DATA_SECTION(D_001DE740) = {
#include "data_inc/D_001DE740.inc"
};

u32 gs_regs_alt_packet[] DATA_SECTION(D_001DE770) = {
#include "data_inc/D_001DE770.inc"
};

u32 vu1_gs_regs_tex_flush[] DATA_SECTION(D_001DF180) = {
#include "data_inc/D_001DF180.inc"
};

u32 vu1_chain_overflow_msg[] DATA_SECTION(D_001E8CF8) = {
#include "data_inc/D_001E8CF8.inc"
};

char vu1_sync_timeout_msg[] DATA_SECTION(D_001E8D10) =
#include "data_inc/D_001E8D10.inc"
    ;

char vu1_halt_msg_header[] DATA_SECTION(D_001E8D38) =
#include "data_inc/D_001E8D38.inc"
    ;

char vu1_halt_register_fmt[] DATA_SECTION(D_001E8D50) =
#include "data_inc/D_001E8D50.inc"
    ;

char vu1_halt_msg_footer[] DATA_SECTION(D_001E8D68) =
#include "data_inc/D_001E8D68.inc"
    ;

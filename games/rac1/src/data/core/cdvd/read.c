/* Data owned by src/core/cdvd/read.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

u32 cd_read_request[1024] DATA_SECTION(D_001314C0) = {0};

u32 cd_read_reply_buffer[48] DATA_SECTION(D_001324C0) = {0};

int cd_read_status[4] DATA_SECTION(D_00132580) = {0};

u32 CoreDataTable[] DATA_SECTION(D_00132E40) = {
#include "data_inc/D_00132E40.inc"
};

u32 cd_read_call_msg[] DATA_SECTION(D_001530C8) = {
#include "data_inc/D_001530C8.inc"
};

u32 cd_read_end_msg[] DATA_SECTION(D_001530E0) = {
#include "data_inc/D_001530E0.inc"
};

u32 cd_read_clock_msg_1[] DATA_SECTION(D_001530F0) = {
#include "data_inc/D_001530F0.inc"
};

u32 cd_read_clock_msg_2[] DATA_SECTION(D_00153110) = {
#include "data_inc/D_00153110.inc"
};

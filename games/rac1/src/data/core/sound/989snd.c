/* Data owned by src/core/sound/989snd.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

#include "structs/data/core_sound_989snd.h"

u8 snd_sync_send_buffer[320] DATA_SECTION(D_00133240) = {0};

u32 snd_command_buffer1[1024] DATA_SECTION(D_00133380) = {0};

u32 snd_command_buffer2[1024] DATA_SECTION(D_00134380) = {0};

u32 snd_stream_buffer1[1024] DATA_SECTION(D_00135380) = {0};

u32 snd_stream_buffer2[1024] DATA_SECTION(D_00136380) = {0};

u32 snd_return_buffer1[272] DATA_SECTION(D_00137380) = {0};

u32 snd_return_buffer2[272] DATA_SECTION(D_001377C0) = {0};

CdSafeState sound_read_work[3] DATA_SECTION(D_00137C00) = {0};

int snd_cd_bank_load_request[16] DATA_SECTION(D_00137C40) = {0};

u32 snd_sif_bind_rpc_error_string[] DATA_SECTION(D_00153D50) = {
#include "data_inc/D_00153D50.inc"
};

u32 snd_989_snd_source_file[] DATA_SECTION(D_00153D78) = {
#include "data_inc/D_00153D78.inc"
};

u32 snd_no_return_error_string[] DATA_SECTION(D_00153D98) = {
#include "data_inc/D_00153D98.inc"
};

char snd_bank_load_by_loc_progress_string[] DATA_SECTION(D_00153EA8) =
#include "data_inc/D_00153EA8.inc"
    ;

char snd_bank_load_by_loc_cd_busy_string[] DATA_SECTION(D_00153ED8) =
#include "data_inc/D_00153ED8.inc"
    ;

u32 snd_bank_load_by_loc_rpc_error_string[] DATA_SECTION(D_00153EF8) = {
#include "data_inc/D_00153EF8.inc"
};

u32 snd_bank_load_from_ee_progress_string[] DATA_SECTION(D_00153FC0) = {
#include "data_inc/D_00153FC0.inc"
};

u32 snd_bank_load_from_ee_cd_busy_string[] DATA_SECTION(D_00153FF8) = {
#include "data_inc/D_00153FF8.inc"
};

short snd_current_buffer[2] DATA_SECTION(D_0015ED80) = {0};

u32 snd_current_buffer_index DATA_SECTION(D_0015ED84) = 0;

short snd_cd_load_error[2] DATA_SECTION(D_0015ED88) = {0};

u32 snd_cd_stream_active DATA_SECTION(D_0015ED8C) = 0;

short snd_cd_status_callback[2] DATA_SECTION(D_0015ED90) = {0};

u32 snd_cd_sync_pending DATA_SECTION(D_0015ED94) = 0;

short snd_cd_stream_end_pending[2] DATA_SECTION(D_0015ED98) = {0};

u32 snd_return_callback_pending DATA_SECTION(D_0015ED9C) = 0;

#include "data_inc/D_0015EDA0.refs"
u32 snd_batch_command_buffers DATA_SECTION(D_0015EDA0) =
#include "data_inc/D_0015EDA0.inc"
    ;

#include "data_inc/D_0015EDA4.refs"
u32 D_0015EDA4 DATA_SECTION(D_0015EDA4) =
#include "data_inc/D_0015EDA4.inc"
    ;

short snd_batch_free_bytes[] DATA_SECTION(D_0015EDA8) = {
#include "data_inc/D_0015EDA8.inc"
};

s32 D_0015EDAC DATA_SECTION(D_0015EDAC) = {
#include "data_inc/D_0015EDAC.inc"
};

#include "data_inc/D_0015EDB0.refs"
u32 snd_stream_buffers DATA_SECTION(D_0015EDB0) =
#include "data_inc/D_0015EDB0.inc"
    ;

#include "data_inc/D_0015EDB4.refs"
u32 D_0015EDB4 DATA_SECTION(D_0015EDB4) =
#include "data_inc/D_0015EDB4.inc"
    ;

#include "data_inc/D_0015EDB8.refs"
u32 snd_batch_return_buffers DATA_SECTION(D_0015EDB8) =
#include "data_inc/D_0015EDB8.inc"
    ;

#include "data_inc/D_0015EDBC.refs"
u32 snd_batch_return_buffers_1 DATA_SECTION(D_0015EDBC) =
#include "data_inc/D_0015EDBC.inc"
    ;

short snd_batch_index[2] DATA_SECTION(D_0015EDC0) = {0};

u32 D_0015EDC4 DATA_SECTION(D_0015EDC4) = 0;

short snd_cd_callback_pending[4] DATA_SECTION(D_0015EDC8) = {0};

short snd_cd_callback_fn[4] DATA_SECTION(D_0015EDD0) = {0};

u32 snd_cd_callback_data[2] DATA_SECTION(D_0015EDD8) = {0};

short snd_return_callback[4] DATA_SECTION(D_0015EDE0) = {0};

short snd_return_callback_data[12] DATA_SECTION(D_0015EDE8) = {0};

unsigned int snd_cd_callback_arg[16] DATA_SECTION(D_0015EE00) = {0};

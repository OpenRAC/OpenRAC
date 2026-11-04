/* Data owned by src/core/mpeg/demux.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

u32 mpeg_stream_type_keys[] DATA_SECTION(D_00132FD8) = {
#include "data_inc/D_00132FD8.inc"
};

u32 mpeg_default_intra_qm[] DATA_SECTION(D_001330C0) = {
#include "data_inc/D_001330C0.inc"
};

u32 mpeg_default_non_intra_qm[] DATA_SECTION(D_00133100) = {
#include "data_inc/D_00133100.inc"
};

u32 pes_optional_field_bits[] DATA_SECTION(D_00153AC8) = {
#include "data_inc/D_00153AC8.inc"
};

u32 pes_pack_header_field_flag_msg[] DATA_SECTION(D_00153AD8) = {
#include "data_inc/D_00153AD8.inc"
};

u32 mpeg_create_work_area_too_small_msg[] DATA_SECTION(D_00153B10) = {
#include "data_inc/D_00153B10.inc"
};

u32 mpeg_work_area_size_too_small_msg[] DATA_SECTION(D_00153B38) = {
#include "data_inc/D_00153B38.inc"
};

u32 mpeg_image_buffer_unaligned_msg[] DATA_SECTION(D_00153B58) = {
#include "data_inc/D_00153B58.inc"
};

u32 mpeg_second_field_missing_msg[] DATA_SECTION(D_00153BB8) = {
#include "data_inc/D_00153BB8.inc"
};

char mpeg_error_format[] DATA_SECTION(D_00153BD8) =
#include "data_inc/D_00153BD8.inc"
    ;

char mpeg_vertical_size_too_large_msg[] DATA_SECTION(D_00153BE8) =
#include "data_inc/D_00153BE8.inc"
    ;

u32 mpeg_chroma_format_not_420_msg[] DATA_SECTION(D_00153C00) = {
#include "data_inc/D_00153C00.inc"
};

u32 mpeg_unsupported_profile_level_msg[] DATA_SECTION(D_00153C28) = {
#include "data_inc/D_00153C28.inc"
};

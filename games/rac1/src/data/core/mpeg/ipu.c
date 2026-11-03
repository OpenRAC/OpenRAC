/* Data owned by src/core/mpeg/ipu.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

u32 ipu_init_quant_matrices[] DATA_SECTION(D_00133150) = {
#include "data_inc/D_00133150.inc"
};

u32 ipu_init_vq_clut[] DATA_SECTION(D_001331A0) = {
#include "data_inc/D_001331A0.inc"
};

u16 scf_t10k_time_zone[] DATA_SECTION(D_001331D0) = {
#include "data_inc/D_001331D0.inc"
};

u32 ScfLanguage DATA_SECTION(D_001331D4) = 0;

u8 RomNameStateData[] DATA_SECTION(D_001331D8) = {
#include "data_inc/D_001331D8.inc"
};

u32 scf_romver_path[] DATA_SECTION(D_00153D00) = {
#include "data_inc/D_00153D00.inc"
};

char scf_romver_open_failed_msg[] DATA_SECTION(D_00153D10) =
#include "data_inc/D_00153D10.inc"
    ;

char scf_romver_read_failed_msg[] DATA_SECTION(D_00153D28) =
#include "data_inc/D_00153D28.inc"
    ;

u32 MonthLengths[] DATA_SECTION(D_00153D40) = {
#include "data_inc/D_00153D40.inc"
};

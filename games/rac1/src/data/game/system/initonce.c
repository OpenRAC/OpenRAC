/* Data owned by src/game/system/initonce.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

u32 rebooted_iop_message[] DATA_SECTION(D_0015FB40) = {
#include "data_inc/D_0015FB40.inc"
};

u32 level_resource_buffers[2] DATA_SECTION(D_001941C0) = {0};

char iop_reboot_image_path[] DATA_SECTION(D_001E7E10) =
#include "data_inc/D_001E7E10.inc"
    ;

u32 loaded_irx_modules_message[] DATA_SECTION(D_001E7E28) = {
#include "data_inc/D_001E7E28.inc"
};

u32 unsupported_lang_message[] DATA_SECTION(D_001E7E40) = {
#include "data_inc/D_001E7E40.inc"
};

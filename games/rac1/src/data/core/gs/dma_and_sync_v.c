/* Data owned by src/core/gs/dma_and_sync_v.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

int DmaChannels[] DATA_SECTION(D_00132E70) = {
#include "data_inc/D_00132E70.inc"
};

char dma_sync_timeout_msg[] DATA_SECTION(D_001534E0) =
#include "data_inc/D_001534E0.inc"
    ;

int DmaChannelsUsed[] DATA_SECTION(D_001534F8) = {
#include "data_inc/D_001534F8.inc"
};

u8 dma_sts_channel_codes[] DATA_SECTION(D_00153520) = {
#include "data_inc/D_00153520.inc"
};

u8 dma_std_channel_codes[] DATA_SECTION(D_00153530) = {
#include "data_inc/D_00153530.inc"
};

u8 dma_mfd_channel_codes[] DATA_SECTION(D_00153540) = {
#include "data_inc/D_00153540.inc"
};

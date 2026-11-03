/* Data owned by src/game/system/stash.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

u32 stash_state[2] DATA_SECTION(D_001DD530) = {0};

u32 stash_rpc_client_address[12] DATA_SECTION(D_001DD538) = {0};

u32 stash_entries[] DATA_SECTION(D_001DD568) = {
#include "data_inc/D_001DD568.inc"
};

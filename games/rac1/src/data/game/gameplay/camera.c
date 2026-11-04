/* Data owned by src/game/gameplay/camera.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

#include "structs/camera.h"

u8 g_hero_platform_carry[16] DATA_SECTION(D_0013F590) = {0};

u8 hero_gravity_dir[16] DATA_SECTION(D_0013F6E0) = {0};

u32 cam_activation_rule7_table[2] DATA_SECTION(D_0015F040) = {0};

int cam_post_upd_routine_cnt DATA_SECTION(D_0015F08C) = {0};

u32 gCameraDefs[2] DATA_SECTION(D_0015F090) = {0};

int cam_coll_mode DATA_SECTION(D_0015F098) = {0};

int cam_coll_flag DATA_SECTION(D_0015F09C) = {0};

int cam_coll_flag_prev[24] DATA_SECTION(D_0015F0A0) = {0};

u32 D_001871A0[8] DATA_SECTION(D_001871A0) = {0};

u32 cur_cam[4] DATA_SECTION(D_001871C0) = {0};

u32 cam_coll_state[56] DATA_SECTION(D_001871D0) = {0};

short cam_trans_state[112] DATA_SECTION(D_001872B0) = {0};

Camera gCameraSlots[48] DATA_SECTION(D_00187510) = {0};

Camera backup_cam DATA_SECTION(D_00189310) = {0};

u32 gCamPostUpdFuncs[20] DATA_SECTION(D_001893B0) = {0};

u32 gCamPrevState[160] DATA_SECTION(D_00189750) = {0};

u32 gCamBackupState[160] DATA_SECTION(D_001899D0) = {0};

int gCameraSlotUsed[] DATA_SECTION(D_00189C50) = {
#include "data_inc/D_00189C50.inc"
};

int text_screen_active[120] DATA_SECTION(D_001E6700) = {0};

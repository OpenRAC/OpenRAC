#ifndef STRUCTS_DATA_GAME_RENDER_EFFECTS_H
#define STRUCTS_DATA_GAME_RENDER_EFFECTS_H

#include "common.h"

struct CameraEnvironmentRegion {
    u8 pad0[0x50];
    s32 flags;
    s32 negative_color;
    s32 positive_color;
    f32 negative_depth_start;
    f32 negative_value_start;
    f32 negative_depth_end;
    f32 negative_value_end;
    f32 positive_depth_start;
    f32 positive_value_start;
    f32 positive_depth_end;
    f32 positive_value_end;
    u8 pad7C[4];
};

#endif /* STRUCTS_DATA_GAME_RENDER_EFFECTS_H */

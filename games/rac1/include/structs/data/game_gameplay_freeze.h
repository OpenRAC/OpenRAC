#ifndef STRUCTS_DATA_GAME_GAMEPLAY_FREEZE_H
#define STRUCTS_DATA_GAME_GAMEPLAY_FREEZE_H

#include "common.h"

struct TextRegionP {
    s16 top;
    s16 bottom;
    s16 left;
    s16 right;
    s16 anchor_x;
    s16 anchor_y;
    s16 measured_width;
    s16 rendered_height;
    s16 line_advance;
    u16 flags;
    s16 subpixel_x_sixteenths;
    s16 subpixel_y_sixteenths;
};

#endif /* STRUCTS_DATA_GAME_GAMEPLAY_FREEZE_H */

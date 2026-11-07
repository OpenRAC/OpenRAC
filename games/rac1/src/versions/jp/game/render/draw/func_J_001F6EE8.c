/* NTSC-J FontPrint: dakuten marks 0xBD and 0xBE are placed from the previous character's offsets and only advance by their difference. */
void func_J_001F6EE8(void *x_arg, void *y_arg, void *color_arg, void *text_arg, void *limit_arg, long texture,
                     unsigned char *glyphs_arg) {
    s32 x = (s32)x_arg;
    s32 y = (s32)y_arg;
    u64 color = (long)color_arg;
    u8 *text = text_arg;
    s32 character_limit = (s32)limit_arg;
    struct Glyph_F6668 *glyphs = (struct Glyph_F6668 *)glyphs_arg;
    s32 character_count;
    u8 *cursor;
    u8 character;
    s32 gray_value;
    s32 gray_color;

    if (font_window_active_r == 0) {
        font_palette_colors_r[0] = color;
    }
    character_count = 0;
    if (character_limit == 0 || *text == 0) {
        return;
    }
    cursor = text;
    do {
        if (*cursor >= 8 && *cursor < 16) {
            if (D_0015F59C_F6668 != 0) {
                color &= 0xFF000000;
                color |= font_palette_colors_r[*cursor - 8] & 0xFFFFFF;
            }
        } else {
            character = *cursor;
            if (glyphs[character].adv != 0) {
                if (*cursor < 0x20) {
                    gray_value = (s32)((color & 0xFF) + ((color >> 8) & 0xFF) + ((color >> 16) & 0xFF)) / 3;
                    gray_color = (s32)(color & 0xFF000000);
                    gray_color += gray_value << 16;
                    gray_color += gray_value << 8;
                    gray_value += gray_color;
                    DrawTexturedQuad_r(x, y + glyphs[*cursor].top, 24, 16, glyphs[*cursor].u, glyphs[*cursor].v, 24, 16,
                                       gray_value, texture);
                } else if ((u8)(character + 0x43) < 2) {
                    s32 prev = cursor[-1];
                    struct Glyph_F6668 *mark = (struct Glyph_F6668 *)(((prev + 0x80) << 2) + (s32)glyphs);
                    s32 d;
                    s32 sum;

                    DrawTexturedQuad_r(x - glyphs[prev].adv + mark->adv, y + mark->top, 16, 16, glyphs[*cursor].u,
                                       glyphs[*cursor].v, 16, 16, color, texture);
                    d = mark->adv - glyphs[cursor[-1]].adv + 4;
                    if (*cursor == 0xBE) {
                        s32 next_x = x + 1;

                        sum = next_x + d;
                    } else {
                        sum = x + d;
                    }
                    x = sum;
                    goto next;
                } else if (*cursor > 0x20) {
                    DrawTexturedQuad_r(x, y + glyphs[*cursor].top, 16, 16, glyphs[*cursor].u, glyphs[*cursor].v, 16, 16,
                                       color, texture);
                }
                x += glyphs[*cursor].adv;
            }
        }
    next:
        cursor++;
        character_count++;
        if (character_count == character_limit) {
            break;
        }
    } while (*cursor != 0);
}

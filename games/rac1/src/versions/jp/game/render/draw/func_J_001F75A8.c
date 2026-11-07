/* NTSC-J scaled FontPrint: dakuten marks 0xBD and 0xBE are placed from the previous character's offsets and only advance by their difference. */
void func_J_001F75A8(u64 color, u8 *str, f32 x, s32 n, s64 tex, struct Glyph *g, f32 y, f32 scale) {
    u8 *s;
    s32 i;
    f32 size;
    f32 top;
    s32 avg;
    s32 mk;

    if (font_window_active == 0) {
        font_palette_colors[0] = color;
    }
    size = scale * 16.0f;
    i = 0;
    if (n == 0 || *str == 0) {
        return;
    }
    s = str;
    do {
        if (*s >= 8 && *s < 16) {
            if ((*(s32 *)&font_color_codes_enabled) != 0) {
                color &= 0xFF000000;
                color |= font_palette_colors[*s - 8] & 0xFFFFFF;
            }
        } else {
            u8 character;

            top = IntToFloat(g[*s].top) * scale;
            character = *s;
            if (g[character].adv != 0) {
                if (*s < 0x20) {
                    avg = (s32)((color & 0xFF) + ((color >> 8) & 0xFF) + ((color >> 16) & 0xFF)) / 3;
                    mk = (s32)(color & 0xFF000000);
                    mk += avg << 16;
                    mk += avg << 8;
                    avg += mk;
                    emit_textured_rectangle_r(x, y + top, scale * 24.0f, scale * 16.0f, g[*s].u, g[*s].v, 24, 16, avg,
                                              tex);
                } else if ((u8)(character + 0x43) < 2) {
                    s32 prev = s[-1];
                    struct Glyph *mark = (struct Glyph *)(((prev + 0x80) << 2) + (s32)g);
                    s32 d;
                    f32 sum;

                    emit_textured_rectangle_r(x + (mark->adv - g[prev].adv) * scale, y + mark->top * scale, size, size,
                                              g[*s].u, g[*s].v, 16, 16, color, tex);
                    d = mark->adv - g[s[-1]].adv + 4;
                    if (*s == 0xBE) {
                        sum = x + (d + 1) * scale;
                    } else {
                        sum = x + d * scale;
                    }
                    x = sum;
                    goto next;
                } else if (*s > 0x20) {
                    emit_textured_rectangle_r(x, y + top, size, size, g[*s].u, g[*s].v, 16, 16, color, tex);
                }
                x += IntToFloat(g[*s].adv) * scale;
            }
        }
    next:
        i++;
        if (i == n) {
            break;
        }
        s++;
    } while (*s != 0);
}

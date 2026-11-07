extern void func_J_001F6EE8(void *, void *, void *, void *, void *, long, unsigned char *);

/* NTSC-J FontPrintWindow: one pass of line breaking, no balancing retries. */
void func_J_001F81F8(void *win_p, long a1, void *text_p, int len, long a4, unsigned char *glyphs) {
    struct FontWindow *win = (struct FontWindow *)win_p;
    unsigned char *text = (unsigned char *)text_p;
    short start[32];
    short endi[32];
    short colr_arr[32];
    int W, line, i, brk, width, colr;
    int k, y, adv;

    VU1_setScissor(win->x_min, win->x_max - 1, win->y_min, win->y_max - 1);
    font_window_active = 1;
    if ((win->flags ^ 1) & 1) {
        W = win->x_max - win->x_anchor;
    } else {
        int l = win->x_anchor;
        int a = win->x_max - l;
        W = l - win->x_min;
        if (a < W)
            W = a;
        W = W * 2;
    }
    line = 0;
    colr = 0;
    i = 0;
    if (len != 0 && text[0] != 0) {
        for (;;) {
            start[line] = i;
            colr_arr[line] = colr;
            width = 0;
            brk = i;
            {
                while (width < W) {
                    if (text[i] == 0x20 || text[i] < 0x10)
                        brk = i;
                    if (*(int *)&font_color_codes_enabled != 0 && (unsigned char)(text[i] - 8) < 8)
                        colr = text[i] - 8;
                    if (text[i] < 2)
                        break;
                    adv = ((signed char *)glyphs)[text[i] * 4 + 3];
                    i++;
                    if (adv != 0)
                        width += adv;
                }
            }
            endi[line] = brk;
            if ((short)brk == start[line])
                endi[line] = i;
            i = endi[line];
            if (text[i] == 0x20 || text[i] < 0x10)
                endi[line] = endi[line] - 1;
            line++;
            if (text[i] == 0) {
                break;
            }
            i++;
            if (i == len || text[i] == 0)
                break;
        }
    }

    {
        struct FontWindow *dw = win;
        int cnt, w;
        dw->out_w = 0;
        y = dw->y_start;
        dw->out_h = line * dw->line_h;
        if (dw->flags & 2)
            y -= (line * dw->line_h) >> 1;
        for (k = 0; k < line; k++, y += dw->line_h) {
            if (y + dw->line_h < dw->y_min)
                continue;
            if (dw->y_max < y)
                continue;
            cnt = endi[k] - start[k] + 1;
            w = measure_text_width(text + start[k], cnt, glyphs);
            if (dw->out_w < w)
                dw->out_w = w;
            if (dw->flags & 4)
                continue;
            font_palette_colors[0] = a1;
            if (dw->flags & 8) {
                float fx;
                if (dw->flags & 1)
                    fx = (dw->x_anchor - (w >> 1)) + dw->x_sub * 0.0625f;
                else
                    fx = dw->x_anchor + dw->x_sub * 0.0625f;
                func_J_001F75A8(font_palette_colors[colr_arr[k]], text + start[k], fx, cnt, a4, (struct Glyph *)glyphs,
                                y + dw->y_sub * 0.0625f, 1.0f);
            } else if (dw->flags & 1) {
                func_J_001F6EE8((void *)(dw->x_anchor - (w >> 1)), (void *)y, (void *)font_palette_colors[colr_arr[k]],
                                text + start[k], (void *)cnt, a4, glyphs);
            } else {
                func_J_001F6EE8((void *)(int)dw->x_anchor, (void *)y, (void *)font_palette_colors[colr_arr[k]],
                                text + start[k], (void *)cnt, a4, glyphs);
            }
        }
    }
    font_window_active = 0;
    VU1_setScissor(0, g_screen_extent[0] - 1, 0, g_screen_extent[1] - 1);
}

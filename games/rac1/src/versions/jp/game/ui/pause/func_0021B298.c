extern void func_J_001F81F8(void);
#undef DRAW
#define DRAW(c, t) \
    ((void (*)(void *, long, char *, long, long, unsigned char *))func_J_001F81F8)(&win, c, t, -1, tex, glyphs)

int render_configured_text_label(MenuScreen *w) {
    extern void disable_font_color_codes(void);
    extern void enable_font_color_codes(void);
    char buf[0x40];
    int id;
    Win win;
    Win tmp;
    char *text = str_default;
    unsigned char *glyphs = normal_font_metrics;
    int font = 1;
    int f0, flags, g, f, content, variant = 0, a21, a23, colour, t, wflags;
    /* GetEffectTex's TEX0 is 64 bits, and FontPrintWindow reads all of them: on the console the
       register keeps them whatever the C says, on a PC an int keeps only the low half. */
    long tex;

    if (f0 & 8) {
        font = 1;
        glyphs = normal_font_metrics;
    }
    if (f0 & 0x10) {
        font = 1;
        glyphs = normal_font_metrics;
    }
    f0 = w->data.label.flags;
    VU1_addGSregister(0x42, 0x44);
    ((void (*)(int, long))VU1_addGSregister)(0x47, 0x2004B);

    flags = w->data.label.flags;
    if (flags & 0x20) {
        content = g_current_level_index - 1;
        if ((unsigned int)content >= 0x12) {
            content = -1;
        }
    } else if (flags & 0x40) {
        content = selected_level_index - 1;
    } else if (flags & 4) {
        t = scale_game_frame_count(menu_fade_duration);
        if (w->data.label.fade_timer < t) {
            w->data.label.fade_timer = scale_game_frame_count(menu_fade_duration);
        }
        content = 0;
        w->data.label.cached_value = 0;
        w->data.label.value_variant = 0;
    } else if (flags & 0x80) {
        content = *(int *)(ST + 0x40);
        if (flags & 0x8000) {
            variant = pal_mode != 0;
        }
    } else if (flags & 0x100) {
        char *cell;
        unsigned char *tbl;
        content = *(int *)(ST + 0x3C);
        cell = *(char **)(ST + 0x48) + content * 10;
        if (*(short *)(cell + 4) == 0) {
            tbl = (unsigned char *)&item_available;
            id = tbl[*(short *)(cell + 6)];
        } else {
            tbl = gSpecialItems;
            id = tbl[*(short *)(cell + 6)];
        }
        if (id == 0) {
            content = -1;
            t = 0;
        }
    } else if (flags & 0x1000) {
        char *st = ST;
        content = *(int *)(st + 0x40);
        id = *(short *)(*(char **)(st + 0x34) + content * 12);
        w->data.label.text_id = 0xFFFF;
        find_help_entry(id, 1, &w->data.label.text_id);
    } else {
        char *cell;
        int k;
        cell = (char *)(*(int *)(ST + 0x3C) * 10 + *(int *)(ST + 0x48));
        k = *(short *)(cell + 6);
        variant = gold_weapon_purchased[k] != 0;
        content = k;
    }

    if (w->data.label.fade_timer == -1) {
        w->data.label.fade_timer = scale_game_frame_count(menu_fade_duration);
        w->data.label.cached_value = content;
        w->data.label.value_variant = variant;
    }
    if (content != w->data.label.cached_value) {
        t = scale_game_frame_count(menu_fade_duration);
        if (t < w->data.label.fade_timer) {
            w->data.label.fade_timer = scale_game_frame_count(menu_fade_duration);
        }
        t = w->data.label.fade_timer;
        t = t < 1 ? 0 : t - 1;
        t = t < 1 ? 0 : t - 1;
        t = t < 1 ? 0 : t - 1;
        w->data.label.fade_timer = t;
        if (t != 0) {
            content = w->data.label.cached_value;
            variant = w->data.label.value_variant;
        } else {
            w->data.label.cached_value = content;
            w->data.label.value_variant = variant;
            w->data.label.flags &= ~0x400;
            w->data.label.scroll_offset = 0;
        }
    } else {
        w->data.label.fade_timer += 3;
    }

    g = w->data.label.flags;
    if (g & 4) {
        if (w->data.label.text_id == 0) {
            return 1;
        }
        text = TEXT(w->data.label.text_id);
    } else if (g & 0x1000) {
        if (w->data.label.text_id == 0xFFFF) {
            return 1;
        }
        text = TEXT(w->data.label.text_id);
    } else if ((g & 0x100) && content == -1) {
        text = str_empty_menu_label;
    } else if (w->data.label.text_id != 0) {
        unsigned int at = (unsigned int)(content * *(int *)&w->data.label.text_stride) >> 2 << 2;
        char *table = *(char **)&w->data.label.text_id;
        text = TEXT(*(int *)(at + (unsigned int)(table + variant * 4)));
    }
    if ((w->data.label.flags & 0x11E4) == 0 && ((unsigned char *)&item_available)[content] == 0) {
        text = str_empty_menu_label;
    }
    if (w->data.label.flags & 0x200) {
        unsigned int at = (unsigned int)(content * *(int *)&w->data.label.text_stride) >> 2 << 2;
        char *table = *(char **)&w->data.label.text_id;
        int id = *(int *)(at + (unsigned int)table);
        if (id != 0x4ED2 && id != 0x4ED9 && id != 0x4EDD) {
            sprintf(buf, fmt_two_strings, TEXT(0x4ECC), text);
            text = buf;
        }
    }
    f = w->data.label.flags;
    a23 = 4;
    a21 = 4;
    if ((f & 0x4004) == 0x4004 && w->data.label.text_id == 0x5295) {
        f |= 1;
        a21 = 0xC;
    }
    if ((w->data.label.flags & 0x800) && gSkillPoints[content] == 0) {
        text = TEXT(0x4F55);
        f |= 3;
    }
    if (text == 0) {
        text = str_null_text;
    }
    wflags = 8;
    if (f & 1) {
        wflags = 9;
        a23 = w->width / 2;
    }
    if (f & 2) {
        wflags |= 2;
        a21 = w->height / 2;
    }

    SetupGifPaging(0);
    tex = GetEffectTex(font);
    ((void *(*)(void *, int, int))libc_memset)(&tmp, 0, 0x18);
    tmp.s[0] = menu_label_text_inset;
    tmp.s[1] = *(unsigned short *)&w->height - menu_label_text_inset;
    tmp.s[2] = 1;
    tmp.s[3] = *(unsigned short *)&w->width - 4;
    tmp.s[4] = a23;
    tmp.s[5] = a21 - (w->data.label.scroll_offset >> 4);
    tmp.s[8] = menu_label_marquee_gap;
    tmp.s[9] = wflags;
    tmp.s[11] = -(*(unsigned short *)&w->data.label.scroll_offset & 0xF);
    win = tmp;
    if (w->data.label.flags & 0x10000) {
        win.s[1] = *(unsigned short *)&w->height - 1;
    }
    colour = fade_menu_color(w->data.label.fade_timer, FastTweenColor(*(int *)&panel_clear_color, 0x80FFA888, 0.5f),
                             0x80FFA888);
    win.s[9] |= 4;
    DRAW(colour, text);
    win.s[9] ^= 4;
    if (!(w->data.label.flags & 0x2000) && !(win.s[7] + 4 < win.s[1] - win.s[0])) {
        if (!(w->data.label.flags & 0x400)) {
            w->data.label.flags |= 0x400;
            w->data.label.scroll_offset = -(w->height << 3);
        }
    } else if (w->data.label.flags & 0x400) {
        w->data.label.scroll_offset = 0;
        w->data.label.flags ^= 0x400;
    }

    win.s[5] = a21 - (w->data.label.scroll_offset >> 4);
    win_shift(&win);
    disable_font_color_codes();
    DRAW(0x80000000L, text);
    enable_font_color_codes();
    win_unshift(&win);
    if (w->data.label.flags & 0x20000) {
        disable_font_color_codes();
    }
    DRAW(colour, text);
    if (w->data.label.flags & 0x20000) {
        enable_font_color_codes();
    }
    if (w->data.label.flags & 0x400) {
        win.s[5] = win.s[5] + (win.s[7] + menu_label_marquee_gap * 3);
        win_shift(&win);
        disable_font_color_codes();
        DRAW(0x80000000L, text);
        enable_font_color_codes();
        win_unshift(&win);
        DRAW(colour, text);
        if (w->data.label.flags & 0x400) {
            int span = (win.s[7] + menu_label_marquee_gap * 3) << 4;
            w->data.label.scroll_offset += (pad_held[0] & 1) ? 10 : 3;
            w->data.label.scroll_offset %= span;
        }
    }
    DoGifPaging();
    return 2;
}

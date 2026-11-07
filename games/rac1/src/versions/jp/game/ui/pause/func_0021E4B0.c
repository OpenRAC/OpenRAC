extern void func_J_001F81F8(void);

int draw_menu_text_list(Menu *menu) {
    int size;
    int kind;
    int focused;
    unsigned char *font;
    int n;
    MenuItem *p;
    int rowh;
    int glyphs;
    int i;
    int sel;
    int en;
    int color;
    char *text;
    int end;
    int y;

    size = 14;
    kind = 1;
    font = normal_font_metrics;
    focused = ((MenuPage *)current_menu_page)->focus == (MenuScreen *)menu;
    if (menu->flags & 4) {
        size = 14;
        kind = 1;
        font = normal_font_metrics;
    }
    if (menu->flags & 8) {
        size = 14;
        kind = 1;
        font = normal_font_metrics;
    }
    VU1_addGSregister(0x42, 0x44);
    VU1_addGSregister(0x47, 0x2004B);
    SetupGifPaging(0);

    n = 0;
    p = menu->items;
    while (p->text != 0) {
        p++;
        n++;
    }
    if (menu->flags & 0x10) {
        rowh = size + 3;
    } else {
        rowh = menu->height / (n + 1);
    }
    y = rowh - size / 2;
    {
        short box[12] = {4, menu->height - 4, 0, menu->width - 2, 0, y - menu->scroll, 0, 0, size + 2};

        glyphs = GetEffectTex(kind);
        for (i = 0; menu->items[i].text != 0; i++) {
            sel = 0;
            if (focused && menu->sel == i) {
                sel = 1;
            }
            en = menu->items[i].enabled != 0;
            if (menu->flags & 2) {
                color = 0x80FFA888;
            } else if (sel) {
                color = en ? 0x8020FFFF : 0x80006060;
            } else {
                color = en ? 0x80FFA888 : 0x80303030;
            }
            if (!(menu->flags & 0x10000) && sel && box[5] < 4) {
                menu->scroll -= 4;
            }
            text = msg_string(menu->items[i].text);
            box[4] = (menu->flags & 0xA00) ? 0x20 : 4;
            if (menu->flags & 0x400) {
                box[9] = 1;
                box[4] = menu->width >> 1;
            }
            if (sel) {
                disable_font_color_codes();
            }
            ((void (*)(void *, long, char *, int, int, void *))func_J_001F81F8)(box, color, text, -1, glyphs, font);
            if (sel) {
                enable_font_color_codes();
            }
            if (menu->flags & 0x200) {
                draw_menu_selection_marker(0xF, box[5] + 9, gSkillPoints[i] != 0);
            }
            if (menu->flags & 0x800) {
                draw_menu_selection_marker(0xF, box[5] + 9, g_game_language == menu->items[i].id);
            }
            box[5] += box[7];
            if (menu->items[i].subtext != 0) {
                text = msg_string(menu->items[i].subtext);
                box[4] = 0x14;
                ((void (*)(void *, long, char *, int, int, void *))func_J_001F81F8)(box, color, text, -1, glyphs, font);
                box[5] += rowh;
            }
            box[5] += 8;
            if (!(menu->flags & 0x10000) && sel) {
                end = box[5] + box[7];
                if (box[1] < end) {
                    if (menu->flags & 0x8000) {
                        menu->scroll += end - box[1];
                    } else {
                        menu->scroll += 4;
                    }
                }
            }
        }
    }
    DoGifPaging();
    if (menu->flags & 0x8000) {
        menu->flags ^= 0x8000;
        return 1;
    }
    return 2;
}

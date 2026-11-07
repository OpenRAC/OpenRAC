extern void func_J_001F6EE8(s32, s32, u64, char *, s32, s64, u8 *);
extern int func_J_001F6B60(char *, int, void *);

s32 render_localized_ui_entry_list(MenuDescriptor *menu) {
    s32 focused;
    u8 *glyphs;
    s32 font_texture_index;
    s32 row_height;
    s32 half_width;
    s32 maximum_text_width;
    s32 selection_index;
    s32 font_height;
    s32 entry_count;
    MenuItem_1C1B0 *entry;
    MenuItem_1C1B0 *scan_entry;
    s32 entry_index;
    s32 selected_entry;
    s32 enabled;
    s32 color;
    char *text_id;
    char *secondary_text;
    s32 x;
    s32 y;
    s32 text_width;
    s32 flags;
    s32 menu_width;
    MenuItem_1C1B0 *selected_item;
    s32 shadow_y;
    s32 shadow_x;
    font_height = 14;
    font_texture_index = 1;
    glyphs = normal_font_metrics;
    focused = (MenuDescriptor *)current_menu_page_r[0]->focus == menu;
    if (menu->flags & 4) {
        font_height = 14;
        font_texture_index = 1;
        glyphs = normal_font_metrics;
    }
    if (menu->flags & 8) {
        font_height = 14;
        font_texture_index = 1;
        glyphs = normal_font_metrics;
    }
    VU1_addGSregister(0x42, 0x44);
    VU1_addGSregister(0x47, 0x2004B);
    SetupGifPaging(0);
    entry_count = 0;
    entry = menu->items;
    while (entry->text_id != 0) {
        entry++;
        entry_count++;
    }

    if (menu->flags & 0x10) {
        row_height = font_height + 3;
    } else {
        row_height = menu->height / (entry_count + 1);
    }
    half_width = menu->width >> 1;
    maximum_text_width = 0;
    y = (row_height - (font_height / 2)) - 1;
    if (menu->flags & 0x4000) {
        scan_entry = menu->items;
        if (scan_entry[0].text_id != 0) {
            entry_index = 0;
            selection_index = 0;
            do {
                text_width = func_J_001F6B60(msg_string_r(scan_entry[entry_index].text_id), -1, glyphs);
                entry_index++;
                selection_index++;
                maximum_text_width = (maximum_text_width < text_width) ? (text_width) : (maximum_text_width);
                scan_entry = menu->items;
            } while (menu->items[selection_index].text_id != 0);
        }
    }
    flags = menu->flags;
    menu_width = menu->width;
    if (flags & 0x20000) {
        if (menu_width < (maximum_text_width + 6)) {
            if (!(flags & 8)) {
                menu->flags = flags | 8;
                return 1;
            }
        }
    }
    entry_index = (selection_index = 0);
    maximum_text_width = (menu_width < maximum_text_width) ? (menu_width) : (maximum_text_width);
    if (menu->items[0].text_id != 0) {
        do {
            selected_entry = 0;
            if (focused) {
                selected_entry = menu->selected_entry == selection_index;
            }
            selected_item = (MenuItem_1C1B0 *)((u32)(entry_index * sizeof(MenuItem_1C1B0)) + (u32)menu->items);
            enabled = selected_item->enabled != 0;
            if (menu->flags & 2) {
                color = 0x80FFA888;
            } else if (selected_entry) {
                if (enabled) {
                    goto fade_timer;
                }
                color = 0x80006060;
            } else if (enabled) {
            fade_timer:
                color = fade_menu_color(selected_item->fade_timer, -1, -1);

            } else {
                color = 0x80303030;
            }
            text_id = msg_string_r(menu->items[entry_index].text_id);
            if (menu->items[entry_index].enabled == 2) {
                text_id = msg_string_r(0x4F55);
            }
            x = half_width - (func_J_001F6B60(text_id, -1, glyphs) >> 1);
            if (menu->flags & 0x40) {
                x = 4;
            } else if (menu->flags & 0x4000) {
                x = half_width - (maximum_text_width >> 1);
            }
            disable_font_color_codes();
            func_J_001F6EE8(x + (*(s32 *)&text_shadow_x), y + (*(s32 *)&text_shadow_y), 0x80000000L, text_id, -1,
                            GetEffectTex(font_texture_index), glyphs);
            enable_font_color_codes();
            if (menu->flags & 0x80) {
                disable_font_color_codes();
            }
            func_J_001F6EE8(x, y, color, text_id, -1, GetEffectTex(font_texture_index), glyphs);
            y += row_height;
            if (menu->items[entry_index].secondary_text_id != 0) {
                disable_font_color_codes();
                shadow_x = x + (*(s32 *)&text_shadow_x);
                shadow_y = y + (*(s32 *)&text_shadow_y);
                secondary_text = msg_string_r(menu->items[entry_index].secondary_text_id);
                func_J_001F6EE8(shadow_x, shadow_y, 0x80000000L, secondary_text, -1, GetEffectTex(font_texture_index),
                                glyphs);
                if (!(menu->flags & 0x80)) {
                    enable_font_color_codes();
                }
                func_J_001F6EE8(x, y, color, msg_string_r(menu->items[entry_index].secondary_text_id), -1,
                                GetEffectTex(font_texture_index), glyphs);
                y += row_height;
            }
            if (menu->flags & 0x80) {
                enable_font_color_codes();
            }
            entry_index++;
            selection_index++;
        } while (menu->items[entry_index].text_id != 0);
    }
    DoGifPaging();
    return 2;
}

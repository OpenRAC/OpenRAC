void update_map_icons(s32 level, s32 flag) {
    char label_text[128];
    f32 map_x;
    f32 map_y;
    struct MapIcon *icon;
    s32 icon_index;

    if (level < 19 && level_visit_state_r[level] != 0) {
        level_map_selection_r.icons = map_icon_lists[level];
    } else {
        level_map_selection_r.icons = 0;
    }

    if (g_hero.motion.pos.f[2] != 0.0f && level == g_current_level_index && flag) {
        world_to_map_coords(&map_x, &map_y, map_use_alt_level_layout ? level + 100 : level, g_hero.motion.pos.f[0],
                            g_hero.motion.pos.f[1]);
        level_map_selection_r.pan_x[level] = (s32)(map_x * 4096.0f) << 16;
        level_map_selection_r.pan_y[level] = (s32)(map_y * 4096.0f) << 16;
    } else {
        /* D_001A02F4_0CDE0 is &level_map_selection_r.posx: a null-guarded reset that can
           never run, but retail still emits it with p folded to 0. */
        s32 *p = D_001A02F4_0CDE0;
        if (p == 0) {
            p[level] = 0x8000000;
            p[level + 20] = 0x8000000;
        }
    }

    if (level_map_selection_r.icons == 0) {
        return;
    }

    if (!(level_map_selection_r.icons->flags & 4)) {
        icon_index = 0;
        do {
            icon = &level_map_selection_r.icons[icon_index];
            if (icon->id == -1) {
                icon->x = 0.234375f;
                icon->y = 0.30078125f;
            } else if (icon->id == -2) {
                icon->x = 0.5390625f;
                icon->y = 0.365234375f;
            } else if (icon->id == -3) {
                icon->x = 0.58203125f;
                icon->y = 0.6875f;
            } else if (icon->id == -4) {
                icon->x = 0.720703125f;
                icon->y = 0.728515625f;
            } else if (icon->id == -5) {
                icon->x = 0.384765625f;
                icon->y = 0.396484375f;
            } else if (icon->id == -7) {
                icon->x = 0.8671875f;
                icon->y = 0.23046875f;
            } else if (icon->id == -8) {
                icon->x = 0.48046875f;
                icon->y = 0.5703125f;
            } else if (icon->id == -9) {
                icon->x = 0.625f;
                icon->y = 0.72265625f;
            } else {
                if (level == g_current_level_index) {
                    if (menu_action_messages[icon->id] != 0) {
                        world_to_map_coords(&icon->x, &icon->y, level, menu_action_messages[icon->id]->x,
                                            menu_action_messages[icon->id]->y);
                        level_map_selection_r.icons[icon_index].angle = menu_action_messages[icon->id]->z;
                    }
                } else {
                    world_to_map_coords(&icon->x, &icon->y, level, race_gates_r2[icon->id].x,
                                        race_gates_r2[icon->id].y);
                    level_map_selection_r.icons[icon_index].angle = race_gates_r2[icon->id].z;
                }
            }
            icon_index++;
        } while (!(level_map_selection_r.icons[icon_index].flags & 4));
    }

    for (icon_index = 0; !(level_map_selection_r.icons[icon_index].flags & 4); icon_index++) {
        icon = &level_map_selection_r.icons[icon_index];
        icon->flags &= ~0x10;
        icon->active = 0;
        if (icon->link == -1) {
            icon->active = 1;
        } else {
            icon->active = D_001A2F90_0CDE0.links[icon->link].active == 1;
        }
        if ((icon->flags & 0x1000) && (map_icon_flags_table[icon->id * 4] ^ 1) & 1) {
            icon->active = 0;
        }
        if (level_map_selection_r.icons[icon_index].label_text_id != 0) {
            s32 label_height_changed = 0;
            s16 previous_label_height;

            level_map_selection_r.icons[icon_index].flags |= 0x10;
            FormatMenuItemText(icon_index, label_text);
            {
                struct TextRegion text_window = {0, icon->label_height, 0, icon->label_width, 4, 4, 0, 0, 0xF, 4};
                font_print_window_small(&text_window, 0x80FFA888L, label_text, -1);
                previous_label_height = text_window.rendered_height;
                icon->label_height = text_window.rendered_height + 8;
                do {
                    text_window.right -= 4;
                    font_print_window_small(&text_window, 0x80FFA888L, label_text, -1);
                    if (text_window.rendered_height != previous_label_height) {
                        label_height_changed = 1;
                    }
                } while (!label_height_changed);
                icon->label_width = text_window.right + 4;
            }
        }
    }
}

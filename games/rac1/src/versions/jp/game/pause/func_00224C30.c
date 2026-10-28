s32 draw_save_slot_list(MenuScreen *menu) {
    char text[0x50];
    McSaveSlot *slot;
    s32 color;
    s32 width;
    s32 inner_right;
    s32 highlight_top;
    s32 highlight_bottom;
    s32 inner_top;
    s32 inner_bottom;
    s32 i;
    s32 y;
    s32 play_ticks;
    s32 hours;
    s32 bolts;
    s32 minutes;

    y = 4;
    SetupGifPaging_r3(0);
    for (i = 0; i < 5; i++) {
        width = menu->width;
        highlight_top = y - 4;
        highlight_bottom = y + 0x34;
        inner_right = width - 3;
        inner_top = y - 1;
        inner_bottom = y + 0x31;
        if (memory_card_state_r4.state < 3 && memory_card_state_r4.err < 0 && D_001D5F70_24C30.unk154 >= 0xB &&
            menu->data.save.slot == i) {
            draw_hud_rect_blended_r4(0, highlight_top, width, highlight_bottom, 0x8020FFFF, 0);
            draw_hud_rect_blended_r4(3, inner_top, inner_right, inner_bottom, panel_clear_color_r, 0);
        }
        draw_hud_rect_blended_r4(3, inner_top, inner_right, inner_bottom, 0x80303030, 0);
        slot = &memory_card_state_r4.card[0].slots[i];
        if (memory_card_state_r4.state >= 3 || memory_card_state_r4.err >= 0 || D_001D5F70_24C30.unk154 < 0xB ||
            memory_card_state_r4.card[0].type != 2) {
            y += 0x30;
        } else {
            if (slot->id == -1) {
                y += 0x10;
                FontPrintCenterSmall_r(menu->width / 2, y, 0x80FFA888,
                                        func_001FE540_24C30(0x526D), -1);
                y += 0x20;
            } else {
                play_ticks = slot->time;
                hours = play_ticks / 216000;
                minutes = play_ticks / 3600 - hours * 60;
                color = menu->data.save.slot == i ? (s32)0x8020FFFF : (s32)0x80FFA888;
                if (hours > 99) {
                    hours = 99;
                    minutes = 59;
                }
                sprintf_r2(text, fmt_save_play_time_r, hours, minutes);
                HudSprite_r(GetIconFrame_r2(0xE99E, 3), 4, y, 0x10, 0x10,
                                0x80);
                FontPrintSmall_r(0x16, y, color, text, -1);
                if (slot->gold_bolts != 0) {
                    HudSprite_r(GetIconFrame_r2(0xE99E, 4), 0x4E, y, 0x10,
                                    0x10, 0x80);
                    sprintf_r2(text, fmt_decimal_r, slot->gold_bolts < 100 ? slot->gold_bolts : 99);
                    FontPrintSmall_r(0x60, y, color, text, -1);
                }
                bolts = slot->bolts;
                if (bolts > 9999999) {
                    bolts = 9999999;
                }
                y += 0x10;
                if (bolts < 1000) {
                    sprintf_r2(text, fmt_decimal_r, bolts);
                } else if (bolts < 1000000) {
                    sprintf_r2(text, fmt_number_thousands_r, bolts / 1000, bolts % 1000);
                } else {
                    sprintf_r2(text, fmt_save_bolt_count_r, bolts / 1000000, bolts % 1000000 / 1000,
                            bolts % 1000);
                }
                HudSprite_r(GetIconFrame_r2(0x754F, 0xF), 4, y, 0x10, 0x10,
                                0x80);
                FontPrintSmall_r(0x16, y, color, text, -1);
                y += 0x10;
                sprintf_r2(text, fmt_save_date_r, slot->name.date.b16, slot->name.date.b15, slot->name.date.b17);
                HudSprite_r(GetIconFrame_r2(0xE99E, 2), 4, y, 0x10, 0x10,
                                0x80);
                FontPrintSmall_r(0x16, y, color, text, -1);
                y += 0x10;
            }
        }
        if (pal_mode_r != 0) {
            y += 0x24;
        } else {
            y += 0x1B;
        }
    }
    DoGifPaging_r3();
    return 2;
}

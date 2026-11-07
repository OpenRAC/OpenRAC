extern void func_J_001F6EE8(s32, s32, u64, char *, s32, s64, u8 *);

s32 DrawEndScreenMenuMaybe(struct ThreeOptionMenu *menu) {
    char text_buffer[0x50];
    u8 *glyphs;
    s32 line_spacing;
    s32 draw_y;

    SetupGifPaging(0);
    line_spacing = menu->height / 5;
    draw_y = menu->height / 5 - 8;
    sprintf_r(text_buffer, msg_string_r5(0x5285), count_gold_bolts(), 0x28);
    measure_text_width_regular_r(text_buffer, -1);
    sprintf_r(text_buffer, msg_string_r5(0x5286), count_gold_weapons(), 10);
    measure_text_width_regular_r(text_buffer, -1);
    sprintf_r(text_buffer, msg_string_r5(0x5287), count_skill_points(), 0x1E);
    measure_text_width_regular_r(text_buffer, -1);
    glyphs = normal_font_metrics_r;
    disable_font_color_codes();
    FontPrintCenter_r(menu->menu_width >> 1, draw_y, 0x80FFA888, msg_string_r5(0x5284), -1);
    draw_y += line_spacing;
    draw_menu_selection_marker_r(0xB, draw_y + 9, count_gold_bolts() == 0x28);
    sprintf_r(text_buffer, msg_string_r5(0x5285), count_gold_bolts(), 0x28);
    func_J_001F6EE8(0x14, draw_y, 0x80FFA888, text_buffer, -1, GetEffectTex_r(1), glyphs);
    draw_y += line_spacing;
    draw_menu_selection_marker_r(0xB, draw_y + 9, count_gold_weapons() == 10);
    sprintf_r(text_buffer, msg_string_r5(0x5286), count_gold_weapons(), 10);
    func_J_001F6EE8(0x14, draw_y, 0x80FFA888, text_buffer, -1, GetEffectTex_r(1), glyphs);
    draw_y += line_spacing;
    draw_menu_selection_marker_r(0xB, draw_y + 9, count_skill_points() == 0x1E);
    sprintf_r(text_buffer, msg_string_r5(0x5287), count_skill_points(), 0x1E);
    func_J_001F6EE8(0x14, draw_y, 0x80FFA888, text_buffer, -1, GetEffectTex_r(1), glyphs);
    enable_font_color_codes();
    DoGifPaging();
    return 2;
}

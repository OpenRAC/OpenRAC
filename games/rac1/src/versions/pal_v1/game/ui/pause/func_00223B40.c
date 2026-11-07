s32 DrawEndScreenMenuMaybe(struct ThreeOptionMenu *menu) {
    char text_buffer[0x50];
    s32 font_texture_index;
    u8 *glyphs;
    s32 maximum_text_width;
    s32 measured_width;
    s32 line_spacing;
    s32 draw_y;

    font_texture_index = 1;
    SetupGifPaging(0);
    line_spacing = menu->height / 5;
    draw_y = menu->height / 5 - 8;
    maximum_text_width = 0;
    sprintf_r(text_buffer, msg_string_r5(0x522F), count_gold_bolts(), 0x28);
    measured_width = measure_text_width_regular_r(text_buffer, -1);
    if (maximum_text_width < measured_width) {
        maximum_text_width = measured_width;
    }
    sprintf_r(text_buffer, msg_string_r5(0x5230), count_gold_weapons(), 10);
    measured_width = measure_text_width_regular_r(text_buffer, -1);
    if (maximum_text_width < measured_width) {
        maximum_text_width = measured_width;
    }
    sprintf_r(text_buffer, msg_string_r5(0x5231), count_skill_points(), 0x1E);
    measured_width = measure_text_width_regular_r(text_buffer, -1);
    if (maximum_text_width < measured_width) {
        maximum_text_width = measured_width;
    }
    glyphs = normal_font_metrics_r;
    if (menu->menu_width < maximum_text_width + 0x18) {
        font_texture_index = 2;
        glyphs = small_font_metrics_r;
    }
    disable_font_color_codes();
    FontPrintCenter_r(menu->menu_width >> 1, draw_y, 0x80FFA888, msg_string_r5(0x522E), -1);
    draw_y += line_spacing;
    draw_menu_selection_marker_r(0xB, draw_y + 9, count_gold_bolts() == 0x28);
    sprintf_r(text_buffer, msg_string_r5(0x522F), count_gold_bolts(), 0x28);
    FontPrint_r2(0x14, draw_y, 0x80FFA888, text_buffer, -1, GetEffectTex_r(font_texture_index), glyphs);
    draw_y += line_spacing;
    draw_menu_selection_marker_r(0xB, draw_y + 9, count_gold_weapons() == 10);
    sprintf_r(text_buffer, msg_string_r5(0x5230), count_gold_weapons(), 10);
    FontPrint_r2(0x14, draw_y, 0x80FFA888, text_buffer, -1, GetEffectTex_r(font_texture_index), glyphs);
    draw_y += line_spacing;
    draw_menu_selection_marker_r(0xB, draw_y + 9, count_skill_points() == 0x1E);
    sprintf_r(text_buffer, msg_string_r5(0x5231), count_skill_points(), 0x1E);
    FontPrint_r2(0x14, draw_y, 0x80FFA888, text_buffer, -1, GetEffectTex_r(font_texture_index), glyphs);
    enable_font_color_codes();
    DoGifPaging();
    return 2;
}

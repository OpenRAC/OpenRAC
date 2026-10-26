s32 DrawSoundMenu(SoundMenu_1DE08 *menu) {
    s32 center_x;
    s32 row_height;
    s32 label_right;
    s32 border_left;
    s32 slider_left;
    s32 slider_padding;
    s32 slider_length;
    s32 second_row;

    row_height = menu->height >> 2;
    center_x = menu->width >> 1;
    SetupGifPaging_r(0);
    label_right = center_x - 8;
    border_left = center_x + 7;
    slider_left = center_x + 9;
    slider_padding = center_x + 0x4A;

    font_print_right_r(label_right, row_height - 8,
                        menu->selected_option == 0 ? 0x8020FFFF : 0x80FFA888,
                        msg_string_r2(0x5212), -1);
    draw_hud_rect_blended_r(border_left, row_height - 8, menu->width - 0x3F, row_height + 8,
                        0x80696969, 0);
    draw_hud_rect_blended_r(slider_left, row_height - 6, menu->width - 0x41, row_height + 6,
                        0x80383838, 0);
    slider_length = (menu->width - slider_padding) * sound_volume_r / 1024;
    func_00201190_1DE08(GetIconFrame_r(0xE99E, 8), slider_left << 4,
                        (row_height - 6) << 4, (center_x + 8 + slider_length) << 4,
                        (row_height + 5) << 4, 0, 0xA0, 0x1F0, 0x150, 0x80);

    second_row = row_height + row_height;
    font_print_right_r(label_right, second_row - 8,
                        menu->selected_option == 1 ? 0x8020FFFF : 0x80FFA888,
                        msg_string_r2(0x5213), -1);
    draw_hud_rect_blended_r(border_left, second_row - 8, menu->width - 0x3F, second_row + 8,
                        0x80696969, 0);
    draw_hud_rect_blended_r(slider_left, second_row - 6, menu->width - 0x41, second_row + 6,
                        0x80383838, 0);
    slider_length = (menu->width - slider_padding) * music_volume_r / 1024;
    func_00201190_1DE08(GetIconFrame_r(0xE99E, 9), slider_left << 4,
                        (second_row - 6) << 4, (center_x + 8 + slider_length) << 4,
                        (second_row + 5) << 4, 0, 0xA0, 0x1F0, 0x150, 0x80);

    second_row += row_height;
    font_print_right_r(label_right, second_row - 8,
                        menu->selected_option == 2 ? 0x8020FFFF : 0x80FFA888,
                        msg_string_r2(0x5214), -1);
    FontPrintLarge_r(center_x + 8, second_row - 8, 0x80FFA888,
                        msg_string_r2(playback_mode_r != 0 ? 0x5216 : 0x5215), -1);
    DoGifPaging_r();
    return 2;
}

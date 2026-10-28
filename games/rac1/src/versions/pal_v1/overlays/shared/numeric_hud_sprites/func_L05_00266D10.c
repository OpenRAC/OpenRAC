int draw_hud_progress_and_rank(char *e) {
    char buf[0x50] __attribute__((aligned(16)));
    int x, y, y1, tx, px, py, w;
    int pal;
    char *hd;
    char *t, *f, *h2;

    if (*(int *)(e + 0x6C) <= 0)
        return 0;
    pal = pal_mode;
    x = hud_sub_66D10(g_screen_extent[1], pal, &rilgar_progress_frame_bottom);
    y = hud_sub_66D10(g_screen_extent[1], pal, &rilgar_progress_text_bottom);
    hd = hero_globals_base + 0xE1D;
    y1 = y + 1;
    draw_stretchable_ui_frame(*(int *)&(rilgar_progress_frame_x), x, *(int *)&(rilgar_progress_frame_width), *(int *)&(rilgar_progress_frame_height), *(int *)&(rilgar_progress_frame_alpha));
    tx = *(int *)&(rilgar_progress_label_x);
    sprintf(buf, rilgar_fmt_label, msg_string(0x523F));
    print_text_with_texture_3(tx - 1, y1, 0x80000000L, buf, -1);
    print_text_with_texture_3(tx, y, *(int *)&(rilgar_progress_label_color), buf, -1);
    w = measure_text_width_regular(buf, -1);
    tx = tx + w;
    {
        int n = *(short *)(hd + 0x89A) + 1;
        if (!(n < 4)) n = 3;
        sprintf(buf, rilgar_fmt_progress_number, n);
    }
    print_text_with_texture_3(tx - 1, y + 2, 0x80000000L, buf, -1);
    print_text_with_texture_3(tx, y, *(int *)&(rilgar_progress_value_color), buf, -1);
    {
        int n = *(int *)(hd + 0x8C0);
        tx = *(int *)&(rilgar_progress_rank_right_x);
        n = n - 1;
        if (!(n < 5)) n = 4;
        sprintf(buf, rilgar_fmt_string, msg_string(n + 0x5224));
    }
    w = measure_text_width_regular(buf, -1);
    tx = tx - w;
    print_text_with_texture_3(tx + 1, y1, 0x80000000L, buf, -1);
    print_text_with_texture_3(tx, y, *(int *)&(rilgar_progress_value_color), buf, -1);
    sprintf(buf, rilgar_fmt_rank_label, msg_string(0x5240));
    w = measure_text_width_regular(buf, -1);
    tx = tx - w;
    print_text_with_texture_3(tx + 1, y1, 0x80000000L, buf, -1);
    print_text_with_texture_3(tx, y, *(int *)&(rilgar_progress_label_color), buf, -1);
    if (g_current_level_index != 0x10)
        return *(int *)(e + 0x58);
    sprintf(buf + 0x40, rilgar_fmt_number, *(unsigned char *)(hd + 0x8CE));
    py = pal_mode ? 0xA : 0x12;
    px = *(int *)(e + 0x50);
    if (*(unsigned char *)(special_items_base + 0x13B) == 0) {
        py = pal_mode ? 0xA : 0x12;
    } else {
        px = *(int *)&(rilgar_counter_box_alt_x);
        py = *(int *)&(rilgar_counter_box_alt_y);
    }
    draw_stretchable_ui_frame(px - 0x18, py, *(int *)&(rilgar_counter_frame_width), 0x20, 0x60);
    w = GetIconFrame(0x7558, 2);
    HudSprite(w, px, py, 0x20, 0x20, 0x80);
    print_text_with_texture_3(px + 0x29, py + 8, 0x80000000L, buf + 0x40, -1);
    h2 = hero_globals_base + 0xE1D;
    if (*(unsigned char *)(h2 + 0x8CE) != 0)
        print_text_with_texture_3(px + 0x28, py + 7, 0x80E08060L, buf + 0x40, -1);
    else
        print_text_with_texture_3(px + 0x28, py + 7, 0x80202080L, buf + 0x40, -1);
    return *(int *)(e + 0x58);
}

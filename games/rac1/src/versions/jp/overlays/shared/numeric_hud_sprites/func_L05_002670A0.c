int draw_hud_timer(HudElem_002670A0 *e) {
    char buf[64];
    int y0, y1, y2;
    int frames, per_min, per_sec;
    int min, sec, hund;
    int x;

    if (e->unk6C <= 0) {
        return 0;
    }
    y0 = -(pal_mode_r ? D_L05_0015FAD0_002670A0 + 10 : D_L05_0015FAD0_002670A0 + 18) + g_screen_extent_r[1];
    y1 = -(pal_mode_r ? D_L05_0015FAEC_002670A0 + 10 : D_L05_0015FAEC_002670A0 + 18) + g_screen_extent_r[1];
    y2 = -(pal_mode_r ? D_L05_0015FAFC_002670A0 + 10 : D_L05_0015FAFC_002670A0 + 18) + g_screen_extent_r[1];
    per_min = pal_mode_r ? 3000 : 3600;
    per_sec = per_min / 60;
    frames = hero_002670A0.race_time_frames;
    min = frames / per_min;
    frames -= min * per_min;
    sec = frames / per_sec;
    frames -= sec * per_sec;
    hund = frames * 100 / per_sec;

    if (gSpecialItems_r[0]) {
        draw_stretchable_ui_frame_r(D_L05_0015FACC_002670A0, y0, D_L05_0015FAD4_002670A0, D_L05_0015FAD8_002670A0, D_L05_0015FADC_002670A0);
        x = D_L05_0015FAE8_002670A0;
        sprintf_r(buf, rilgar_fmt_race_score_width_r, msg_string_r(0x50A7), hero_002670A0.race_score);
        x -= measure_text_width_regular_r(buf, -1) >> 1;
        sprintf_r(buf, rilgar_fmt_label_r, msg_string_r(0x50A7));
        print_text_with_texture_3_r(x + 1, y1 + 1, 0x80000000, buf, -1);
        print_text_with_texture_3_r(x, y1, D_L05_0015FAE0_002670A0, buf, -1);
        x += measure_text_width_regular_r(buf, -1);
        sprintf_r(buf, rilgar_fmt_race_score_value_r, hero_002670A0.race_score);
        print_text_with_texture_3_r(x + 1, y1 + 1, 0x80000000, buf, -1);
        print_text_with_texture_3_r(x, y1, D_L05_0015FAE4_002670A0, buf, -1);
    }
    x = D_L05_0015FAE8_002670A0;
    sprintf_r(buf, rilgar_fmt_race_time_width_r, msg_string_r(0x5298));
    x -= measure_text_width_regular_r(buf, -1) >> 1;
    sprintf_r(buf, rilgar_fmt_race_time_label_r, msg_string_r(0x5298));
    print_text_with_texture_3_r(x + 1, y2 + 1, 0x80000000, buf, -1);
    print_text_with_texture_3_r(x, y2, D_L05_0015FAE0_002670A0, buf, -1);
    x += measure_text_width_regular_r(buf, -1);
    sprintf_r(buf, rilgar_fmt_race_time_value_r, min, sec, hund);
    print_text_with_texture_3_r(x + 1, y2 + 1, 0x80000000, buf, -1);
    print_text_with_texture_3_r(x, y2, D_L05_0015FAE4_002670A0, buf, -1);
    return e->w;
}

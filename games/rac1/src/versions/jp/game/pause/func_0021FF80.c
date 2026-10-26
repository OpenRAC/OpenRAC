int DrawGBsShipMenu__maybe(char *arg0) {
    char buf[0x100];
    char *t = gCamera;
    char *o = *(char **)(arg0 + 0x44);
    int count;

    *(float *)(o + 0x10) = *(float *)(t + 0x140) + 8.0f;
    *(float *)(o + 0x14) = *(float *)(t + 0x144) + gold_bolt_marker_y_offset_f;
    *(float *)(o + 0x18) = *(float *)(t + 0x148) - 0.1f;
    if (*(int *)(arg0 + 0x44) != 0) {
        DrawMobyList(*(int *)(arg0 + 0x44), 1);
    }
    SetupGifPaging(0);
    count = 0;
    {
        int k;
        for (k = 0; k < 4; k++) {
            if (gold_bolts_collected[level_map_selection[0x89] * 4 + k] != 0) {
                count = count + 1;
            }
        }
    }
    sprintf(buf, fmt_gold_bolt_count_line, msg_string_id(0x4F50), count,
                  msg_string_id(0x4F54), map_gold_bolt_totals[level_map_selection[0x89]]);
    font_print_right_c(*(int *)(arg0 + 0x20) - 0x10, (g_screen_extent[1] >> 1) - 8,
                    0x80000000L, buf, -1);
    font_print_right_c(*(int *)(arg0 + 0x20) - 0x11, (g_screen_extent[1] >> 1) - 9,
                    0x80FFA888L, buf, -1);
    DoGifPaging();
    if (0) {
        /* no code: registers the 2-byte view last (see above) */
        (void)gold_bolt_marker_y_offset_s;
    }
    return 8;
}

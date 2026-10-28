void update_level09_item_unlock_and_frame_record(char *moby) {
    int *data = *(int **)(moby + 0x78);
    unsigned char *q;
    unsigned char *b;
    unsigned short *tp;
    int n;
    if (gaspar_frame_count > 0) {
        if (airship_parts_alive_count == 0) {
            q = menu_item_enabled_115 + 1;
            if (q[0xE] == 0) {
                q[0xE] = 1;
                PlayBankSound(1, 0, 0);
                ShowBanner(0x53D6, -1);
                airship_parts_alive_count = 0;
            } else {
                airship_parts_alive_count = 0;
            }
        } else {
            airship_parts_alive_count = 0;
        }
    }
    b = help_stats_base + 0x34D;
    tp = (unsigned short *)(b + 0x2D8);
    if (*tp == 0) {
        if (gHaveHeliPack[9] != 0) {
            (*tp)++;
        }
        n = scale_ticks(gameplay_event_frame_count) / 600;
        if (*(unsigned short *)(b + 0x2DA) < n) {
            *(unsigned short *)(b + 0x2DA) = scale_ticks(gameplay_event_frame_count) / 600;
        }
        *(int *)(b + 0x2DC) = *(int *)(b + 0x2DC) | (1 << g_current_level_index_m) | 0x80000000;
        if (is_point_inside_clip_volume(hero_globals_base + 0xE9D, *data) != 0) {
            Help_Request(0x2329, 0x5B);
        }
    }
}

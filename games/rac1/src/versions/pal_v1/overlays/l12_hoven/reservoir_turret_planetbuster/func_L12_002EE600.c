void update_hoven_help_and_dialogue(char *m) {
    int *d = *(int **)(m + 0x78);
    if (count_active_moby_group_members(d[3], -1) == 0) {
        unsigned char *s = menu_item_enabled_115_e + 1;
        if (s[0x14] == 0) {
            s[0x14] = 1;
            PlayBankSound(1, 0, 0);
            ShowBanner(0x53D6, -1);
        }
    }
    {
        char *h = help_stats_base_e + 0x34D;
        if (*(unsigned short *)(h + 0x368) == 0) {
            char *q = hero_globals_base_e + 0xE1D;
            if (*(int *)(q + 0x2084) == 0x81) {
                *(unsigned short *)(h + 0x368) = *(unsigned short *)(h + 0x368) + 1;
            }
            if (scale_ticks(gameplay_event_frame_count_e) / 600 > *(unsigned short *)(h + 0x36A)) {
                *(unsigned short *)(h + 0x36A) = scale_ticks(gameplay_event_frame_count_e) / 600;
            }
            *(unsigned int *)(h + 0x36C) = *(unsigned int *)(h + 0x36C) | (1 << g_current_level_index_e) | 0x80000000;
            if (is_point_inside_clip_volume(q + 0x80, d[0])) {
                Help_Request(0x2EE3, 0x6D);
            }
        }
    }
    {
        char *h = help_stats_base_e + 0x34D;
        if (*(unsigned short *)(h + 0x370) == 0
            || (*(unsigned short *)(h + 0x370) < 2
                && scale_ticks(gameplay_event_frame_count_e) - *(unsigned short *)(h + 0x372) * 600 > scale_ticks(72000))) {
            char *q = hero_globals_base_e + 0xE1D;
            if (*(int *)(q + 0x2084) == 0x81) {
                d[1]++;
                if ((pad_globals_base_e[0x2600 / 4] & 3) && 0.3f < *(float *)(q + 0x229C)) {
                    d[2]++;
                } else {
                    d[2] = 0;
                }
                if (scale_ticks(0x28) < d[2]) {
                    char *g = help_stats_base_e + 0x34D;
                    d[1] = 0;
                    if (*(unsigned short *)(g + 0x370) <= 0xFFFE) {
                        *(unsigned short *)(g + 0x370) = *(unsigned short *)(g + 0x370) + 1;
                    }
                    if (scale_ticks(gameplay_event_frame_count_e) / 600 > *(unsigned short *)(g + 0x372)) {
                        *(unsigned short *)(g + 0x372) = scale_ticks(gameplay_event_frame_count_e) / 600;
                    }
                    *(unsigned int *)(g + 0x374) = *(unsigned int *)(g + 0x374) | (1 << g_current_level_index_e) | 0x80000000;
                }
                {
                    int a = scale_ticks(0x1C20);
                    int b = scale_ticks(0x708);
                    char *k = help_stats_base_e + 0x34D;
                    if (*(unsigned short *)(k + 0x370) * a + b < d[1]) {
                        Help_Request(0x2EE4, 0x6E);
                    }
                }
            }
        }
    }
    if (gHaveHeliPack_e[2] != 0) {
        char *k = help_stats_base_e + 0x34D;
        if (*(unsigned short *)(k + 0x1F0) == 0) {
            Help_Request(0x2EE0, 0x3E);
        }
    }
}

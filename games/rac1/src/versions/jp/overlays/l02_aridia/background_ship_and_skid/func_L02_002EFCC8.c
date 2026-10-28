void update_tutorial_hint_triggers(unsigned char *moby) {
    int *data = *(int **)(moby + 0x78);
    moby[0x30] = 0xFF;

    if (is_point_inside_clip_volume(hero_globals_base + 0xE9D, data[0x17]) != 0
        && *(unsigned int *)(hero_globals_base + 0xE9D + 0x200C) < 2) {
        char *b = help_stats_base + 0x34D;
        if (*(unsigned short *)(b + 0x278) == 0 && usage_stats_entry20_count == 0) {
            int count = 0;
            int i;
            for (i = 0; i < 37; i++) {
                if (aridia_item_defs[i].active == 0 && i != 8 && i != 24) {
                    if ((menu_item_enabled_115 + 0xB9)[i] != 0) {
                        count++;
                    }
                }
            }
            if (count >= 2) {
                Help_Request(0x4E25, 0x4F);
            }
        }
    }

    switch (data[0x15]) {
    case 0:
        if (is_point_inside_clip_volume(hero_globals_base + 0xE9D, data[0x12]) != 0) {
            if (*(int *)(hero_globals_base + 0xE9D + 0x2004) == 0x2C) {
                data[0x15] = 1;
            }
        }
        break;
    case 1: {
        char *h = hero_globals_base + 0xE1D;
        if (*(int *)(h + 0x300) != 0) {
            data[0x15] = 0;
        } else if (is_point_inside_clip_volume(h + 0x80, data[0x13]) != 0) {
            unsigned char *q = menu_item_enabled_115 + 1;
            if (q[1] == 0) {
                q[1] = 1;
                PlayBankSound(1, 0, 0);
                ShowBanner(0x542D, -1);
            }
        }
        break;
    }
    }

    {
        char *b = help_stats_base + 0x34D;
        if (*(unsigned short *)(b + 0x60) == 0) {
            if (is_point_inside_clip_volume(hero_globals_base + 0xE9D, data[4]) != 0) {
                int lvl = data[0];
                if (saved_mission_state[g_current_level_index][lvl] != 0xFF) {
                    unsigned char *f = menu_item_enabled_115 + 0xB9;
                    if (f[0x10] != 0 && ((int *)(help_stats_base + 0xD875))[lvl] >= 6) {
                        int t = scale_ticks(gameplay_event_frame_count) - item_usage_entry36_last_time * 600;
                        if (scale_ticks(0x4650) < t) {
                            Help_Request(0x7D2, 0xC);
                        }
                    }
                }
            }
        }
    }

    if (is_point_inside_clip_volume(hero_globals_base + 0xE9D, data[0x10]) != 0) {
        data[0x16] = 1;
    }
    if (data[0x16] != 0) {
        if (usage_stats_entry8_count < 3) {
            char *b = help_stats_base + 0x34D;
            if (!(*(unsigned int *)(b + 0x264) & (1 << g_current_level_index))) {
                Help_Request(0x4E22, 0x4C);
            }
        }
        if (invert52[0] != 0 && opt_cam_invert[0] != 0) {
            char *b = help_stats_base + 0x34D;
            if (!(*(unsigned int *)(b + 0x3B4) & (1 << g_current_level_index))) {
                Help_Request(0x4E2C, 0x76);
            }
        } else {
            char *b = help_stats_base + 0x34D;
            *(unsigned short *)(b + 0x3B0) = 0xFFFF;
        }
    }

    {
        char *b = help_stats_base + 0x34D;
        if (*(unsigned short *)(b + 0x68) == 0) {
            if (is_point_inside_clip_volume(hero_globals_base + 0xE9D, data[4]) != 0) {
                int lvl = data[0];
                if (saved_mission_state[g_current_level_index][lvl] != 0xFF) {
                    unsigned char *f = menu_item_enabled_115 + 0xB9;
                    if (f[0x10] == 0 && (float)((int *)(help_stats_base + 0xD875))[lvl] >= 3.0f) {
                        Help_Request(0x7D3, 0xD);
                    }
                }
            }
        }
    }

    {
        char *b = help_stats_base + 0x34D;
        if (*(unsigned short *)(b + 0x80) == 0) {
            char *h;
            if (is_point_inside_clip_volume(hero_globals_base + 0xE9D, data[6]) != 0) {
                *(unsigned short *)(b + 0x80) = 0xFFFF;
            }
            h = hero_globals_base + 0xE1D;
            if (*(int *)(h + 0x2084) != 0x2C && *(int *)(h + 0x2090) == 0x2C && *(int *)(h + 0x198) == 1) {
                data[0xC]++;
            }
            if (data[0xC] >= 2) {
                if (is_point_inside_clip_volume(hero_globals_base + 0xE9D, data[7]) != 0) {
                    Help_Request(0x7D6, 0x10);
                }
            }
        }
    }

    if (((Flags_0013D5C8 *)(menu_item_enabled_115 + 0xB9))->b[0xC] == 0) {
        if (is_point_inside_clip_volume(hero_globals_base + 0xE9D, data[5]) != 0) {
            char *b = help_stats_base + 0x34D;
            int t = scale_ticks(gameplay_event_frame_count) - *(unsigned short *)(b + 0x8A) * 600;
            if ((int)(scale_ticks(0x12) * 60.0f) < t || *(unsigned short *)(b + 0x8A) * 600 == 0) {
                Help_Request(0x7D7, 0x11);
            } else {
                int n = scale_ticks(gameplay_event_frame_count) / 600;
                if (*(unsigned short *)(b + 0x8A) < n) {
                    *(unsigned short *)(b + 0x8A) = scale_ticks(gameplay_event_frame_count) / 600;
                }
            }
        }
    }
    if (((Flags_0013D5C8 *)(menu_item_enabled_115 + 0xB9))->b[0xC] != 0 && is_point_inside_clip_volume(hero_globals_base + 0xE9D, data[5]) != 0) {
        char *b = help_stats_base + 0x34D;
        if (*(unsigned short *)(b + 0x90) != 0) {
            int t = scale_ticks(gameplay_event_frame_count) - *(unsigned short *)(b + 0x92) * 600;
            if ((int)(scale_ticks(0x12) * 60.0f) < t || *(unsigned short *)(b + 0x92) * 600 == 0) {
                Help_Request(0x7D8, 0x12);
            } else {
                int n = scale_ticks(gameplay_event_frame_count) / 600;
                if (*(unsigned short *)(b + 0x92) < n) {
                    *(unsigned short *)(b + 0x92) = scale_ticks(gameplay_event_frame_count) / 600;
                }
            }
        } else {
            int n;
            (*(unsigned short *)(b + 0x90))++;
            n = scale_ticks(gameplay_event_frame_count) / 600;
            if (*(unsigned short *)(b + 0x92) < n) {
                *(unsigned short *)(b + 0x92) = scale_ticks(gameplay_event_frame_count) / 600;
            }
            *(int *)(b + 0x94) = *(int *)(b + 0x94) | (1 << g_current_level_index) | 0x80000000;
        }
    } else if (is_point_inside_clip_volume(hero_globals_base + 0xE9D, data[7]) != 0) {
        char *b = help_stats_base + 0x34D;
        *(unsigned short *)(b + 0x90) = 0xFFFF;
    }

    {
        char *b = help_stats_base + 0x34D;
        unsigned short v = *(unsigned short *)(b + 0x230);
        if (v == 0 || *(unsigned short *)(b + 0x238) == 0) {
            char *h = hero_globals_base + 0xE1D;
            if (*(int *)(h + 0x2084) == 0x72) {
                char *p = *(char **)(h + 0x2FC);
                if (p != 0 && *(short *)(p + 0xA6) == 0x267) {
                    if (v == 0) {
                        Help_Request(0x7DB, 0x46);
                    } else if (*(unsigned short *)(b + 0x238) == 0) {
                        data[0x14]++;
                        if (scale_ticks(600) < data[0x14]) {
                            Help_Request(0x7DC, 0x47);
                        }
                    }
                }
            }
        }
    }

    {
        unsigned char *g = special_items_base + 0x13B;
        if (g[0x14] == 0) {
            if (is_point_inside_clip_volume(hero_globals_base + 0xE9D, data[0x11]) != 0) {
                g[0x14] = 1;
            }
        }
    }
    qcopy(data + 8, hero_globals_base + 0xE9D);
}

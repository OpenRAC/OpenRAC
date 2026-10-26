void update_moby_hit_stats_and_state(char *m) {
    char *d = *(char **)(m + 0x78);
    char *info = MobyGetHitMessage(m, 0x330000, 0);
    char *g;
    ((unsigned char *)m)[0xA4] = 0xFF;
    g = hero_globals_base + 0xE1D;
    if (*(int *)(g + 0x2084) != 0x32) {
        *(unsigned short *)(m + 0x34) &= 0xEFFF;
    } else {
        *(unsigned short *)(m + 0x34) |= 0x1000;
    }
    if (info != 0) {
        char *st = (char *)help_stats_base + 0x34D;
        int st2;
        int cur;
        if (*(unsigned short *)(st + 0x157E8) < 0xFFFF) {
            *(unsigned short *)(st + 0x157E8) += 1;
        }
        if (scale_ticks(gameplay_event_frame_count) / 600 > *(unsigned short *)(st + 0x157EA)) {
            *(unsigned short *)(st + 0x157EA) = scale_ticks(gameplay_event_frame_count) / 600;
        }
        *(int *)(st + 0x157EC) = *(int *)(st + 0x157EC) | (1 << g_current_level_index_r) | 0x80000000;
        if (g_current_level_index_r == 0xB && *(char **)(info + 0x20) != 0 && *(short *)(*(char **)(info + 0x20) + 0xA6) == 0xAC) {
            if (++skill_point_18_kill_count >= 3 && gSkillPoints[0x12] == 0) {
                gSkillPoints[0x12] = 1;
                PlayBankSound(1, 0, 0);
                ShowBanner(0x53D6, -1);
            }
        }
        if (*(int *)(d + 0x88) != 0) {
            st2 = 6;
            cur = ((unsigned char *)m)[0x20];
        } else {
            cur = ((unsigned char *)m)[0x20];
            st2 = 2;
        }
        if (cur != st2) {
            m[0x20] = st2;
            if (((unsigned char *)m)[0x53] != 1) {
                MobyBlendAnimation(m, 1, 0, scale_ticks(10));
            }
        }
    } else if (((unsigned char *)m)[0x20] == 5) {
        char *g2 = hero_globals_base + 0xE1D;
        if (*(int *)(g2 + 0x2084) != 0x32) {
            m[0x20] = 4;
            *(int *)(m + 0x94) = 0;
            *(unsigned short *)(m + 0x34) |= 1;
        }
        if (coll_sphere(1.0f, m + 0x10, 0, m)) {
            m[0x20] = 6;
            if (((unsigned char *)m)[0x53] != 1) {
                MobyBlendAnimation(m, 1, 0, scale_ticks(10));
            }
        }
    }
}

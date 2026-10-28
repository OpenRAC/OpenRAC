void update_hero_relocation_trigger(char *m) {
    char *d = *(char **)(m + 0x78);
    unsigned short id;
    ((unsigned char *)m)[0x30] = 0xFF;
    if (*(int *)&orxon_relocation_reset_flag == 1) {
        *(short *)(help_stats_base + 0x32D) = 0;
        *(int *)&orxon_relocation_reset_flag = 0;
    }
    if (*(int *)(d + 0x18) != -1) {
        int b = *(int *)(d + 0x1C);
        if (b != -1) {
            if (*(short *)((*(int *)(d + 0x18) << 8) + (int)orxon_mobys_r + 0xA6) == 0x325 &&
                *(short *)((b << 8) + (int)orxon_mobys_r + 0xA6) == 0x325) {
                char *f1 = *(char **)((*(int *)(d + 0x18) << 8) + (int)orxon_mobys_r + 0x78);
                char *f2 = *(char **)((b << 8) + (int)orxon_mobys_r + 0x78);
                if (saved_mission_state[((unsigned char *)m)[0xB0] + (g_current_level_index << 4)] == 0xFF) {
                    *(int *)(f1 + 8) |= 8;
                    *(int *)(f2 + 8) &= ~8;
                } else {
                    *(int *)(f1 + 8) &= ~8;
                    *(int *)(f2 + 8) |= 8;
                }
            }
        }
    }
    id = *(unsigned short *)(m + 0xB2);
    if (orxon_collected_flags_base.collected[(short)id] != 0 ||
        ((*(int *)(help_stats_base + 0xAB75 + ((((short)id >> 5) * 4) + (g_current_level_index << 8))) >> (id & 0x1F)) & 1) != 0) {
        if (gHaveHeliPack[4] != 0) {
            char *bs = orxon_mobys_r;
            *(unsigned short *)((*(int *)d << 8) + (int)bs + 0x34) |= 1;
            *(unsigned short *)((*(int *)d << 8) + (int)bs + 0x34) |= 2;
            *(int *)((*(int *)d << 8) + (int)bs + 0x94) = 0;
        } else {
            if (*(int *)&orxon_cinematic_mode == 0) {
                unsigned char *T = help_stats_base + 0x22D;
                unsigned short t0 = *(unsigned short *)(T + 0x100);
                int r = scale_ticks(0x12C);
                if (t0 < r) {
                    unsigned short t1 = *(unsigned short *)(T + 0x100);
                    int r2;
                    if (t1 <= 0xFFFE)
                        *(short *)(T + 0x100) = t1 + 1;
                    r2 = scale_ticks(gameplay_event_frame_count);
                    if (r2 / 600 > *(unsigned short *)(T + 0x102)) {
                        *(short *)(T + 0x102) = scale_ticks(gameplay_event_frame_count) / 600;
                    }
                    {
                        int w = *(int *)(T + 0x104);
                        w |= 1 << g_current_level_index;
                        w |= 0x80000000;
                        *(int *)(T + 0x104) = w;
                    }
                    try_set_help_message(1, 0x5448);
                    if (((unsigned char *)hero_globals_base)[0x2EC1] == 1) return;
                } else {
                    if (((unsigned char *)hero_globals_base)[0x2EC1] == 1) return;
                }
            } else {
                if (((unsigned char *)hero_globals_base)[0x2EC1] == 1) return;
            }
        }
    }
    *(int *)(help_stats_base + 0xAB75 + ((((short)*(unsigned short *)(m + 0xB2) >> 5) * 4) + (g_current_level_index << 8)))
        |= 1 << (*(unsigned short *)(m + 0xB2) & 0x1F);
    orxon_moby_killed[(short)*(unsigned short *)(m + 0xB2) >> 5] |= 1 << (*(unsigned short *)(m + 0xB2) & 0x1F);
    if (*(int *)(d + 8) == -1 || *(int *)d == -1) return;
    if (gHaveHeliPack[4] == 0) {
        char *e = (char *)((*(int *)d << 8) + (int)orxon_mobys_r);
        int eo = g_current_level_index << 4;
        unsigned char byte;
        int k;
        *(int *)(e + 0x94) = *(int *)(*(char **)(e + 0x24) + 0x10);
        byte = saved_mission_state[((unsigned char *)m)[0xB0] + eo];
        if (byte == 0xFF || *(int *)(d + 0x14) == -1) {
            k = *(int *)(d + 0x14);
            Hero_ResetToPosition(orxon_scene_points + (k << 7) + 0x30, orxon_scene_points + (k << 7) + 0x70, 0, 1);
        } else {
            k = *(int *)(d + 8);
            Hero_ResetToPosition(orxon_scene_points + (k << 7) + 0x30, orxon_scene_points + (k << 7) + 0x70, 0, 1);
        }
        enter_hero_vehicle_mode(1, 0x43, orxon_mobys_r + (*(int *)d << 8));
        MusicRequestTrack(2, 4);
    } else {
        unsigned char *h = (unsigned char *)hero_globals_base + 0xE1D;
        if (h[0x20A4] == 1) {
        exit_hero_vehicle_mode();
        {
            Hero_ResetToPosition(orxon_scene_points + (*(int *)(d + 4) << 7) + 0x30, orxon_scene_points + (*(int *)(d + 4) << 7) + 0x70, 0, 1);
        }
        MusicRequestTrack(0, 5);
        {
            char *bs = orxon_mobys_r;
            *(unsigned short *)((*(int *)d << 8) + (int)bs + 0x34) |= 1;
            *(unsigned short *)((*(int *)d << 8) + (int)bs + 0x34) |= 2;
            *(int *)((*(int *)d << 8) + (int)bs + 0x94) = 0;
        }
            h[0x22CB] = 1;
            DeleteMoby(m);
        } else {
            h[0x22CB] = 1;
            DeleteMoby(m);
        }
    }
}

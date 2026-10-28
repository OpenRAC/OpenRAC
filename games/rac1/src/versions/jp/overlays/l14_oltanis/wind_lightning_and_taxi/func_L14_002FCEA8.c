void qwark_oltanis_update(char *m) {
    char *d = *(char **)(m + 0x78);
    attach_big_head_cheat_manipulator_to_matching_moby(m);
    if (*(unsigned char *)(m + 0x31) != 0) {
        if (FastVecDist(m + 0x10, oltanis_camera_pos) < 28.0f) {
            set_ground_height_band(m);
            m[0x7F] = 0x16;
        }
    }
    switch (*(unsigned char *)(m + 0x20)) {
    case 0: {
        unsigned short id;
        if (oltanis_collected_flags_base_r2.collected[(short)(id = *(unsigned short *)(m + 0xB2))] != 0
            || (*(int *)(help_stats_base_r + 0xAB75 + (((short)id >> 5) * 4 + (g_current_level_index_m << 8))) >> (id & 0x1F)) & 1) {
            DeleteMoby(m);
            return;
        }
        if (*(int *)(d + 0x44) == -1 || *(int *)(d + 0x48) == -1) {
            DeleteMoby(m);
            return;
        }
        *(char **)(d + 0x20) = qwark_oltanis_dialogue_name;
        DialogRequest_Init(m, d);
        d[8] = 1;
        m[0x20] = 1;
        break;
    }
    case 1:
        if (*(unsigned char *)(d + 8) == 1 && is_point_inside_clip_volume(hero_globals_base + 0xE9D, *(int *)(d + 0x40)) != 0) {
            if ((help_stats_base_r + 0xAA35)[*(unsigned char *)(m + 0xB0) + (g_current_level_index_m << 4)] != 0xFF) {
                char *t;
                jet_clear_persistent((unsigned char)m[0xB0]);
                t = oltanis_clip_volumes_r + (*(int *)(d + 0x48) << 7);
                jet_reset_camera(t + 0x30, t + 0x70);
            }
            enable_elevator_alternate_route_r(oltanis_mobys_r + (*(int *)(d + 0x44) << 8));
            *(float *)(d + 0xC) = 255.0f;
        } else {
            *(float *)(d + 0xC) = 1.7f;
        }
        if (Npc_TryStartDialog(m, d) != 0) {
            PlaceMarkerAheadOfMoby(3.0f, m);
            m[0x20] = 2;
        }
        break;
    case 2:
        if (oltanis_game_mode == 2) break;
        m[0x20] = 1;
        if (*(short *)(d + 4) != 3 && *(short *)(d + 4) != 4) break;
        if (*(short *)(d + 4) == 4) {
            ShowBanner(0x5434, -1);
        } else {
            ShowBanner(0x36B3, -1);
        }
        grant_inventory_item(0x20, 1);
        mark_elevator_collected(oltanis_mobys_r + (*(int *)(d + 0x44) << 8));
        *(int *)(help_stats_base_r + 0xAB75 + (((short)*(unsigned short *)(m + 0xB2) >> 5) * 4 + (g_current_level_index_m << 8))) |= 1 << (*(unsigned short *)(m + 0xB2) & 0x1F);
        oltanis_moby_killed_bits[(short)*(unsigned short *)(m + 0xB2) >> 5] |= 1 << (*(unsigned short *)(m + 0xB2) & 0x1F);
        memcard_Save(0, -1);
        DeleteMoby(m);
        return;
    case 3:
        DeleteMoby(m);
        return;
    }
    {
        float b[4];
        float sx = 0.02f;
        float sy = 0.3f;
        int doit = 0;
        if (*(unsigned char *)(m + 0x53) == 0) {
            char *g = hero_globals_base + 0xE9D;
            char *a_;
            doit = 1;
            if (vec_distance_xy(m + 0x10, g) < 8.0f
                && FastDiffRots(*(float *)(m + 0x48),
                       fast_atan2(*(float *)((a_ = g - 0x80) + 0xD0) - *(float *)(m + 0x10),
                                         *(float *)(a_ + 0xD4) - *(float *)(m + 0x14))) < 1.5707964f) {
                if (0.01f < FastVecLength(g + 0x80)) {
                    *(int *)(d + 0x194) = scale_ticks(0x78);
                } else {
                    fast_dec_timer((int *)(d + 0x194));
                }
            } else if (*(int *)(d + 0x194) != 0) {
                *(int *)(d + 0x194) = 0;
                qcopy(d + 0x180, hero_globals_base + 0xEED);
            }
            if (fast_dec_timer((int *)(d + 0x198)) != 0) {
                float y;
                float z;
                float x = random_float_between(180.0f, 300.0f);
                *(int *)(d + 0x198) = truncate_float_to_s32_r(multiply_global_factor(x));
                y = FastAddRots(*(float *)(m + 0x48), random_float_between(-90.0f, 90.0f) * 0.017453292f);
                z = random_float_between(0.0f, 30.0f) * 0.017453292f;
                BuildSphericalOffset(d + 0x180, 6.0f, y, z);
                FastVecAdd(d + 0x180, d + 0x180, m + 0x10);
            }
            if (*(int *)(d + 0x194) != 0) {
                qcopy(b, hero_globals_base + 0xEED);
                sx = 0.04f;
                sy = 0.3f;
            } else {
                qcopy(b, d + 0x180);
            }
        }
        if (doit != 0) {
            float w[4];
            float u[4];
            float f20;
            float f2;
            qcopy(w, m + 0x10);
            w[2] = w[2] + 1.0f;
            FastVecSub(u, b, w);
            f20 = FastSubRots(fast_atan2(u[0], u[1]), *(float *)(m + 0x48));
            f2 = -fast_atan2(vec_length_xy(u), u[2]);
            if (f20 > 1.5707964f) f20 = 1.5707964f;
            else if (f20 < -1.5707964f) f20 = -1.5707964f;
            if (f2 > 0.5235988f) f2 = 0.5235988f;
            else if (f2 < -0.5235988f) f2 = -0.5235988f;
            *(float *)(d + 0xE4) = f2;
            *(float *)(d + 0xE8) = f20 * 0.6f;
            *(float *)(d + 0x168) = f20 * 0.4f;
        }
        if (gCheats[0] != 0) *(float *)(d + 0xF0) = 2.75f;
        EaseManipulatorOffsets(m, d + 0x80, 0, sx * g_frame_scale_sq, sy * g_frame_scale_sq);
        EaseManipulatorOffsets(m, d + 0x100, 1, sx * g_frame_scale_sq, sy * g_frame_scale_sq);
    }
}

void update_moby_proximity_interaction(unsigned char *m) {
    char *d = *(char **)(m + 0x78);
    float *p;
    float tmp[4];
    int v;
    int one;
    unsigned char *q;

    if (m[0x20] != 0x2A) {
        if (m[0x31] != 0) {
            m[0xBC] += 2;
            v = (int)(FastCos(IntToFloat((unsigned char)m[0xBC] - 0x80) * 0.024543693f) * 50.0f) + 0x96;
            if (*(short *)(m + 0xA6) == 0x215) {
                *(int *)(m + 0x90) = (v >> 1) | (v << 8) | (v << 16);
            } else {
                *(int *)(m + 0x90) = v | (v << 8) | (v << 16);
            }
            AddDrawCallback(render_environment_mapped_object, m);
            if (vec_distance_xy(m + 0x10, g_debug_camera) < 32.0f) {
                AddDrawCallback(draw_light_flare, m);
            }
        }
        p = (float *)(m + 0x10);
        if (*(short *)(d + 0xC) != 0) {
            matrix_mul_vec3(tmp, ship_entry_point_offsets + ship_index[0] * 16, m + 0xC0);
            FastVecAdd(tmp, tmp, p);
            if (FastVecDist(tmp, hero_globals_base + 0xE9D) > 4.1f) {
                *(short *)(d + 0xC) = 0;
            }
        } else {
            q = (unsigned char *)hero_globals_base + 0xE9D;
            if (vec_distance_xy(p, q) < 6.0f) {
                matrix_mul_vec3(tmp, ship_entry_point_offsets + ship_index[0] * 16, m + 0xC0);
                FastVecAdd(tmp, tmp, p);
                if (FastVecDist(tmp, q) < 4.0f) {
                    *(short *)(d + 0xC) = 1;
                }
            }
        }
        if (*(unsigned short *)(m + 0x34) & 1) {
            *(short *)(d + 0xC) = 0;
        }
        if (*(short *)(d + 0xC) != 0) {
            int r = try_set_help_message(2, 0x543B) != 0;
            if ((*(int *)(pad_globals_base + 0x2604) & 0x10) && r) {
                if (g_current_level_index_m == 10 && *(unsigned char *)(hero_globals_base + 0x2EC1) == 1) {
                    m[0x20] = 0x2A;
                    *(unsigned short *)(m + 0x34) |= 0x41;
                    StartCutscene(9);
                } else {
                    remove_help_message_for_selector(2);
                    ship_level_exit_requested = 1;
                    ship_index[1] = 0;
                }
            }
        }
    } else {
        if (cinematic_mode != 2) {
            q = special_items_base + 0x13B;
            if (q[0x48] == 0 && gHaveMorphORay[7] != 0) {
                StartCutscene(4);
                q[0x48] = 1;
            } else if ((q = special_items_base + 0x13B)[0x49] == 0 && level_available_base[8] != 0) {
                StartCutscene(5);
                q[0x49] = 1;
            } else if ((q = special_items_base + 0x13B)[0x4A] == 0 && level_available_base[8] != 0) {
                PlayMovieEntryIfIdle(0xB);
                q[0x4A] = 1;
            } else if ((q = special_items_base + 0x13B)[0x4B] == 0 && level_available_base[8] != 0) {
                StartCutscene(6);
                q[0x4B] = 1;
            } else {
                m[0x20] = 0;
                *(unsigned short *)(m + 0x34) &= 0xFFBE;
                one = 1;
                ship_index[1] = one;
                ship_level_exit_requested = one;
            }
        }
    }
}

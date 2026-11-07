void background_ship_2_update(char *moby) {
    char *d;
    char *t;
    int r;
    int flag = 0;
    float tv[4];
    float f70;
    float p2;
    float x;
    int r1;
    int r2;
    int r3;
    d = *(char **)(moby + 0x78);
    t = aridia_paths[*(int *)(d + 0x60)];
    r = (int)MobyGetHitMessage(moby, 0x10000, 0);
    switch (*(unsigned char *)(moby + 0x20)) {
        case 0: {
            int m1;
            *(unsigned char *)(moby + 0x30) = 0xFF;
            *(short *)(moby + 0x32) = 0xFF;
            if (g_current_level_index_r == LEVEL_ORXON) {
                *(float *)(moby + 0x2C) = *(float *)(moby + 0x2C) * 0.5f;
            }
            m1 = -1;
            if (*(int *)(d + 0x60) == m1) {
                STUB_printf(background_ship_2_no_path_msg, *(short *)(moby + 0xB2));
                DeleteMoby(moby);
                return;
            }
            if (*(int *)aridia_paths[*(int *)(d + 0x60)] == 0) {
                STUB_printf(background_ship_2_empty_path_msg, *(short *)(moby + 0xB2));
                DeleteMoby(moby);
                return;
            }
            f70 = fast_vec_dist(aridia_paths[*(int *)(d + 0x60)] + 0x10, aridia_paths[*(int *)(d + 0x60)] + 0x20);
            *(unsigned char *)(d + 0x2C) = 0x28;
            *(float *)(d + 0x70) = f70;
            *(unsigned char *)(d + 0x2B) = 1;
            qcopy(moby + 0x10, aridia_paths[*(int *)(d + 0x60)] + 0x10);
            *(float *)(d + 0x68) = random_angle_radians();
            *(int *)(d + 0x6C) = m1;
            *(unsigned char *)(moby + 0x20) = 1;
            *(unsigned short *)(moby + 0x34) |= 0x10;
            *(float *)(d + 0x64) = *(float *)(d + 0x74) * (float)*(int *)t;
            break;
        }
        case 1:
            if (*(unsigned char *)hero_vehicle_mode == 1 && g_current_level_index_r == LEVEL_ORXON) {
                DeleteMoby(moby);
                return;
            }
            qcopy(tv, moby + 0x10);
            if (voice_is_playing(moby, *(int *)(d + 0x6C)) == 0) {
                *(int *)(d + 0x6C) = Moby_PlaySound(1, 4, (int)moby);
            }
            p2 = background_ship_2_speed * g_frame_time;
            if (g_current_level_index_r == LEVEL_ORXON) {
                p2 = p2 * 0.25f;
            }
            *(float *)(d + 0x64) = *(float *)(d + 0x64) + p2 / *(float *)(d + 0x70);
            if ((float)*(int *)t < *(float *)(d + 0x64)) {
                *(float *)(d + 0x64) = *(float *)(d + 0x64) - (float)*(int *)t;
            }
            if ((unsigned int)(g_current_level_index_r - 9) < 2) {
                sample_camera_path((int *)t, 1, moby + 0x10, (float *)(moby + 0x40), 0, *(float *)(d + 0x64));
            } else {
                sample_camera_path((int *)t, 0, moby + 0x10, (float *)(moby + 0x40), 0, *(float *)(d + 0x64));
            }
            if (*(float *)(moby + 0x10) < 8.0f) {
                *(int *)(moby + 0x94) = 0;
            } else if (*(float *)(moby + 0x14) < 8.0f) {
                *(int *)(moby + 0x94) = 0;
            } else {
                *(int *)(moby + 0x94) = *(int *)(*(char **)(moby + 0x24) + 0x10);
            }
            if (*(unsigned char *)(moby + 0x31) != 0) {
                if (g_current_level_index_r != LEVEL_ORXON) {
                    spawn_background_ship_2_joint_sparks(moby, tv);
                } else {
                    spawn_background_ship_2_level_10_joint_sparks(moby, tv);
                }
            }
            if (r != 0 && 0.0f < *(float *)(r + 0x2C)) {
                flag = 1;
            }
            if (flag) {
                *(unsigned char *)(moby + 0x20) = 2;
            }
            break;
        case 2:
            if (g_current_level_index_r == LEVEL_ARIDIA) {
                aridia_skill_ship_kills = aridia_skill_ship_kills + 1;
                if (aridia_skill_ship_kills >= 3) {
                    unsigned char *p = gSkillPoints;
                    if (p[2] == 0) {
                        p[2] = 1;
                        allocate_voice_for_bank_entry(1, 0, 0);
                        queue_dialogue_message(0x53D6, -1);
                    }
                }
            }
            if (g_current_level_index_r == LEVEL_GASPAR) {
                if (*(int *)hero_current_state == 0x32) {
                    gaspar_skill_ship_kills = gaspar_skill_ship_kills + 1;
                    if (gaspar_skill_ship_kills >= 5) {
                        unsigned char *p = gSkillPoints;
                        if (p[0xF] == 0) {
                            p[0xF] = 1;
                            allocate_voice_for_bank_entry(1, 0, 0);
                            queue_dialogue_message(0x53D6, -1);
                        }
                    }
                }
            }
            Moby_PlaySound(0, 0, (int)moby);
            FastVecScale(tv, moby + 0xC0, background_ship_2_debris_speed * frame_scale);
            tv[2] = tv[2] + background_ship_2_debris_lift * frame_scale;
            r1 = scale_ticks(90);
            SpawnBreakFragment(moby, 0x5F6, (float *)(moby + 0x10), (void *)(moby + 0x40), r1, 0,
                               g_frame_time_sq * 12.0f, tv, aridia_zero_vec, aridia_zero_vec);
            r2 = scale_ticks(90);
            SpawnBreakFragment(moby, 0x5F8, (float *)(moby + 0x10), (void *)(moby + 0x40), r2, 0,
                               g_frame_time_sq * 12.0f, tv, aridia_zero_vec, aridia_zero_vec);
            r3 = scale_ticks(90);
            SpawnBreakFragment(moby, 0x5F9, (float *)(moby + 0x10), (void *)(moby + 0x40), r3, 0,
                               g_frame_time_sq * 12.0f, tv, aridia_zero_vec, aridia_zero_vec);
            DeleteMoby(moby);
            return;
        default:
            break;
    }
    if (*(unsigned char *)(moby + 0x31) != 0) {
        x = FastSin(*(float *)(d + 0x68)) * 4.0f - 3.0f;
        if (1.0f < x) {
            x = 1.0f;
        } else if (x < 0.0f) {
            x = 0.0f;
        }
        {
            int res;
            float k;
            res = FastTweenColor(background_ship_2_light_color_off, background_ship_2_light_color_on, x);
            k = background_ship_2_light_spin_rate * 0.017453292f * g_frame_time;
            *(int *)(moby + 0x90) = res;
            *(float *)(d + 0x68) = FastAddRots(*(float *)(d + 0x68), k);
        }
        prepare_background_ship_2_joint_draw_data(moby);
    }
}

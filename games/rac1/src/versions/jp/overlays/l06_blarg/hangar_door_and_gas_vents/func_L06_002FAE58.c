void alien_queen_update(unsigned char *moby)
{
    unsigned char *data = *(unsigned char **)(moby + 0x78);
    char *target;
    float v[4];

    update_alien_queen_hit_response_and_recovery(moby);
    target = *(char **)(data + 0xB0);
    Manipulator_ApplyScale_q(moby, *(int *)&alien_queen_scale_manip_flag, 1.9f, data + 0x130);
    if ((unsigned char)moby[0x31] != 0 && FastVecDist_q(moby + 0x10, blarg_camera_pos_q) < 45.0f) {
        set_ground_height_band_q(moby);
        moby[0x7F] = 0x24;
    }
    if ((unsigned char)moby[0x20] >= 3 && (unsigned char)moby[0x20] <= 13) {
        *(int *)(data + 0x10C) = queue_animation_update_q(0x16, 0xFFFF, reset_numeric_hud_element_q, Hud_UpdateCounter_q,
                                                 draw_segmented_hud_meter_base_q + 0x20, data + 0x110, 0x1EF);
    }
    switch ((unsigned char)moby[0x20]) {
    case 0: {
        int *p;
        *(float *)(data + 0x20) = 30.0f;
        *(short *)(data + 0x24) = 0x1E;
        data[0x29] = 1;
        data[0x28] = 3;
        data[0x2E] = 1;
        data[0x58] = truncate_float_to_s32_q(14.0f);
        data[0x5A] = truncate_float_to_s32_q(18.4f);
        *(int *)(data + 0x120) = scale_ticks_q(0x4B0);
        *(int *)(data + 0x178) = 0;
        qcopy(data + 0xC0, moby + 0x10);
        *(int *)(data + 0x10C) = -1;
        *(int *)(data + 0x114) = *blarg_paths_q[*(int *)(data + 0xEC)];
        p = blarg_paths_q[*(int *)(data + 0xEC)];
        lerp_vector_q(data + 0xD0, p + 4, (char *)p + ((*p / 2) * 16 + 16), 0.5f);
        *(float *)(data + 0xDC) = vec_distance_xy_q(data + 0xC0, data + 0xD0);
        *(float *)(data + 0x128) = random_angle_radians_q();
        *(int *)(data + 0x12C) = scale_ticks_q(0x1E);
        *(int *)(moby + 0x94) = 0;
        *(unsigned short *)(moby + 0x34) = (*(unsigned short *)(moby + 0x34) | 0x4001) & 0xEFFF;
        *(float *)(moby + 0x18) = *(float *)(moby + 0x18) + 17.5f;
        set_moby_list_state_18_q(*(int *)(data + 0xF4));
        if (menu_item_enabled_19_q[4] != 0 && race_gates_base_q[8] == 0) {
            DeleteMoby_q(moby);
            return;
        }
        moby[0x20] = 1;
        break;
    }
    case 1:
        if (*(int *)(data + 0x170) == 0) {
            if (*(int *)(data + 0xB4) == 2) return;
            if (*(short *)(target + 0xA6) != 0) return;
            *(int *)(data + 0x170) = 1;
            *(int *)(data + 0x12C) = scale_ticks_q(10);
            break;
        }
        if (*(int *)(data + 0x12C) != 0) return;
        set_moby_and_links_to_states_8_and_9_q(level_mobys_q_q + (*(int *)(data + 0xFC) << 8));
        set_moby_and_links_to_states_8_and_9_q(level_mobys_q_q + (*(int *)(data + 0x100) << 8));
        set_listed_class_16f_state_2_and_data_28_to_one_q(*(int *)(data + 0x104));
        *(int *)(data + 0x170) = 0;
        moby[0x20] = 2;
        *(int *)(moby + 0x94) = *(int *)(*(char **)(moby + 0x24) + 0x10);
        *(unsigned short *)(moby + 0x34) = (*(unsigned short *)(moby + 0x34) | 0x1000) & 0xFFFE;
        if ((unsigned char)moby[0x53] != 8) {
            MobyBlendAnimation_q(moby, 8, 0, scale_ticks_q(1));
        }
        start_alien_queen_intro_q(moby);
        return;
    case 2: {
        float g = g_frame_time_sq_q * 30.0f;
        *(float *)(moby + 0x18) = *(float *)(moby + 0x18) + *(float *)(data + 0x48);
        *(float *)(data + 0x48) = *(float *)(data + 0x48) - g;
        if ((unsigned char)moby[0x53] == 8) {
            if (FastAbsF_q((*(float *)(moby + 0x18) - *(float *)(data + 0xC8)) / *(float *)(data + 0x48)) <= 6.0f
                && (unsigned char)moby[0x53] != 9) {
                MobyBlendAnimation_q(moby, 9, 0, scale_ticks_q(5));
            }
        } else {
            if (*(float *)(moby + 0x18) < *(float *)(data + 0xC8)) {
                *(float *)(moby + 0x18) = *(float *)(data + 0xC8);
            }
            if (*(float *)(moby + 0x54) == 0.0f && (moby[0x70] & 2)) {
                if ((unsigned char)moby[0x53] == 9) {
                    MobyBlendAnimation_q(moby, 6, 0, scale_ticks_q(0x1E));
                } else if ((unsigned char)moby[0x53] != 1) {
                    MobyBlendAnimation_q(moby, 1, 0, scale_ticks_q(0x1E));
                }
            }
        }
        if (update_alien_queen_intro_camera_q(moby) != 0) {
            moby[0x20] = 7;
            finish_alien_queen_intro_q(moby);
        }
        break;
    }
    case 3:
        if ((unsigned char)moby[0x53] == 6 && *(float *)(moby + 0x54) == 0.0f && (moby[0x70] & 2)) {
            MobyBlendAnimation_q(moby, 1, 0, scale_ticks_q(0x14));
            activate_listed_state_15_mobys_at_raised_positions_q(moby);
        }
        if (update_alien_queen_first_recovery_camera_q(moby) != 0) {
            moby[0x20] = 4;
            finish_alien_queen_first_recovery(moby);
            return;
        }
        break;
    case 5:
        if ((unsigned char)moby[0x53] == 6 && *(float *)(moby + 0x54) == 0.0f && (moby[0x70] & 2)) {
            MobyBlendAnimation_q(moby, 1, 0, scale_ticks_q(0x14));
            clear_moby_list_state_18_q(*(int *)(data + 0xF4));
        }
        if (update_alien_queen_second_recovery_camera(moby) != 0) {
            moby[0x20] = 6;
            finish_alien_queen_second_recovery(moby);
        }
        break;
    case 4:
        activate_listed_state_15_mobys_at_raised_positions_q(moby);
    case 6:
        update_alien_queen_orbit_movement_q(moby);
        if (((unsigned char)moby[0x20] == 4 && check_all_listed_class_33b_states_fe_fd_or_10_q(*(int *)(data + 0xF0)) != 0)
            || ((unsigned char)moby[0x20] == 6 && count_active_moby_group_members_q(*(int *)(data + 0xF4), -1) == 0)) {
            moby[0x20] = 8;
            *(int *)(data + 0x120) = scale_ticks_q(0x4B0);
            if ((unsigned char)moby[0x53] != 3) {
                MobyBlendAnimation_q(moby, 3, 0, scale_ticks_q(0x14));
            }
        }
        return;
    case 7:
        if (*(int *)(data + 0xB4) == 2) break;
        moby[0x20] = 8;
        if ((unsigned char)moby[0x53] == 3) break;
        MobyBlendAnimation_q(moby, 3, 0, scale_ticks_q(0x14));
        return;
    case 8: {
        float dist = vec_distance_xy_q(moby + 0x10, target + 0x10);
        float turn = FastDiffRots_q(*(float *)(moby + 0x48),
                                     fast_atan2_q(*(float *)(target + 0x10) - *(float *)(moby + 0x10),
                                                         *(float *)(target + 0x14) - *(float *)(moby + 0x14)));
        SteerAngleWithVelocity_q((float *)(moby + 0x48),
                            fast_atan2_q(*(float *)(target + 0x10) - *(float *)(moby + 0x10),
                                                *(float *)(target + 0x14) - *(float *)(moby + 0x14)),
                            (float *)(data + 0xE0), g_frame_time_sq_q * 6.2831855f, g_frame_time_sq_q * 6.2831855f,
                            g_frame_time_q * 3.1415927f);
        if (turn < 0.7853982f && *(float *)&alien_queen_attack_range < dist) {
            ApproachValue_q((float *)(data + 0xE8), *(float *)&alien_queen_walk_speed * g_frame_time_q,
                            *(float *)&alien_queen_walk_accel * g_frame_time_sq_q);
        } else if (dist < *(float *)&alien_queen_back_off_range) {
            ApproachValue_q((float *)(data + 0xE8), -(*(float *)&alien_queen_walk_speed * g_frame_time_q),
                            *(float *)&alien_queen_walk_accel * g_frame_time_sq_q);
        } else {
            ApproachValue_q((float *)(data + 0xE8), 0.0f, g_frame_time_sq_q * 8.0f);
        }
        FastVecSub_q(v, target + 0x10, moby + 0x10);
        v[2] = 0.0f;
        vec_scale_to_length_q(v, v, *(float *)(data + 0xE8));
        FastVecAdd_q(moby + 0x10, moby + 0x10, v);
        resolve_point_path_segment_xy_clearance_q(*(int *)(data + 0xEC), moby + 0x10, 1.0f, moby + 0x10);
        if (*(int *)(data + 0xB4) == 2) {
            moby[0x20] = 0xD;
            break;
        }
        if (fast_dec_timer_q(data + 0x120) != 0) {
            *(int *)(data + 0x120) = scale_ticks_q(0x168);
            moby[0x20] = 0xC;
            break;
        }
        if (dist < *(float *)&alien_queen_attack_range && turn < *(float *)&alien_queen_attack_arc * 0.017453292f) {
            moby[0x20] = 9;
            moby[0xBC] = 7;
            if ((unsigned char)moby[0x53] != 4) {
                MobyBlendAnimation_q(moby, 4, 0, scale_ticks_q(0x14));
                return;
            }
        }
        break;
    }
    case 12:
        update_alien_queen_orbit_movement_q(moby);
        if (fast_dec_timer_q(data + 0x120) != 0) {
            *(int *)(data + 0x120) = scale_ticks_q(0x4B0);
            moby[0x20] = 0xB;
            moby[0xBC] = 7;
            if ((unsigned char)moby[0x53] != 6) {
                MobyBlendAnimation_q(moby, 6, 0, scale_ticks_q(0x14));
            }
        }
        return;
    case 9:
        fast_dec_timer_q(data + 0x120);
        SteerAngleWithVelocity_q((float *)(moby + 0x48), *(float *)(moby + 0x48), (float *)(data + 0xE0),
                            g_frame_time_sq_q * 6.2831855f, g_frame_time_sq_q * 6.2831855f, g_frame_time_q * 3.1415927f);
        if (moby[0x70] & 2) {
            moby[0x20] = 0xA;
            if ((unsigned char)moby[0x53] != 5) {
                MobyBlendAnimation_q(moby, 5, 0, scale_ticks_q(0x14));
                return;
            }
        }
        break;
    case 10: {
        QueenProbe probe;
        float buf[4];
        fast_dec_timer_q(data + 0x120);
        if (1.0f < MobyGetFrameValue_q(moby) && MobyGetFrameValue_q(moby) < 8.0f) {
            v[0] = FastCos_q(*(float *)(moby + 0x48)) * 0.45f;
            v[1] = FastSin_q(*(float *)(moby + 0x48)) * 0.45f;
            v[2] = 0.0f;
            probe.owner = (char *)moby;
            probe.flags = 1;
            probe.scale = 1.0f;
            probe.count = 1;
            probe.dir[0] = FastCos_q(*(float *)(moby + 0x48));
            probe.dir[1] = FastSin_q(*(float *)(moby + 0x48));
            probe.dir[2] = 0.0f;
            Moby_GetJointWorldPos_q(moby, 0, buf);
            coll_sphere_mobys_q(buf, 0.5f, 0, moby, &probe);
            Moby_GetJointWorldPos_q(moby, 1, buf);
            coll_sphere_mobys_q(buf, 0.5f, 0, moby, &probe);
            Moby_GetJointWorldPos_q(moby, 2, buf);
            coll_sphere_mobys_q(buf, 0.5f, 0, moby, &probe);
            Moby_GetJointWorldPos_q(moby, 3, buf);
            coll_sphere_mobys_q(buf, 0.5f, 0, moby, &probe);
            Moby_GetJointWorldPos_q(moby, 4, buf);
            coll_sphere_mobys_q(buf, 0.5f, 0, moby, &probe);
            Moby_GetJointWorldPos_q(moby, 5, buf);
            coll_sphere_mobys_q(buf, 1.1f, 0, moby, &probe);
            Moby_GetJointWorldPos_q(moby, 6, buf);
            coll_sphere_mobys_q(buf, 1.1f, 0, moby, &probe);
            return;
        }
        if (moby[0x70] & 2) {
            moby[0x20] = moby[0xBC];
            if ((unsigned char)moby[0x53] != 1) {
                MobyBlendAnimation_q(moby, 1, 0, scale_ticks_q(0x14));
            }
        }
        return;
    }
    case 11:
        if (moby[0x70] & 2) {
            moby[0x20] = moby[0xBC];
            if ((unsigned char)moby[0x53] != 1) {
                MobyBlendAnimation_q(moby, 1, 0, scale_ticks_q(0x14));
            }
        }
        return;
    case 13: {
        char *pos = (char *)moby + 0x10;
        char *home = (char *)data + 0xC0;
        float dist = vec_distance_xy_q(pos, home);
        float turn = FastDiffRots_q(*(float *)(moby + 0x48),
                                     fast_atan2_q(*(float *)(data + 0xC0) - *(float *)(moby + 0x10),
                                                         *(float *)(data + 0xC4) - *(float *)(moby + 0x14)));
        SteerAngleWithVelocity_q((float *)(moby + 0x48),
                            fast_atan2_q(*(float *)(data + 0xC0) - *(float *)(moby + 0x10),
                                                *(float *)(data + 0xC4) - *(float *)(moby + 0x14)),
                            (float *)(data + 0xE0), g_frame_time_sq_q * 6.2831855f, g_frame_time_sq_q * 6.2831855f,
                            g_frame_time_q * 3.1415927f);
        if (turn < 0.7853982f) {
            FastVecSub_q(v, home, pos);
            v[2] = 0.0f;
            vec_scale_to_length_q(v, v, *(float *)&alien_queen_walk_speed * g_frame_time_q);
            FastVecAdd_q(pos, pos, v);
            resolve_point_path_segment_xy_clearance_q(*(int *)(data + 0xEC), pos, 1.0f, pos);
        }
        if (*(int *)(data + 0xB4) != 2) {
            moby[0x20] = 7;
            break;
        }
        if (dist < 1.0f) {
            moby[0x20] = 7;
            if ((unsigned char)moby[0x53] != 1) {
                MobyBlendAnimation_q(moby, 1, 0, scale_ticks_q(0x14));
                return;
            }
        }
        break;
    }
    case 14: {
        unsigned char *flag;
        menu_item_enabled_19_q[4] = 1;
        *(int *)(data + 0x110) = 0;
        moby[0x30] = 0xFF;
        if (data[0x2E] == 2) {
            set_moby_and_links_to_states_4_and_5_q(level_mobys_q_q + (*(int *)(data + 0x100) << 8));
            set_listed_class_16f_state_2_and_clear_data_28_q(*(int *)(data + 0x104));
            *(unsigned short *)(moby + 0x34) |= 1;
            moby[0x20] = 0xF;
            moby[0x31] = 0;
            *(int *)(moby + 0x94) = 0;
            if ((unsigned char)moby[0x53] != 1) {
                MobyBlendAnimation_q(moby, 1, 0, scale_ticks_q(0x14));
                return;
            }
            break;
        }
        if (!(moby[0x70] & 2)) break;
        qcopy(v, moby + 0x10);
        v[2] = v[2] + 1.0f;
        spawn_color_swapped_explosion_burst_q(moby, v, -1, 2.0f, 20.0f);
        spawn_moby_break_fragments_q(moby, 0x6A2, 1, 0x6A3, 3, 6, 2);
        set_moby_and_links_to_states_4_and_5_q(level_mobys_q_q + (*(int *)(data + 0x100) << 8));
        set_listed_class_16f_state_2_and_clear_data_28_q(*(int *)(data + 0x104));
        moby[0x20] = 0xF;
        if ((unsigned char)moby[0x53] != 1) {
            MobyBlendAnimation_q(moby, 1, 0, scale_ticks_q(0x14));
        }
        *(int *)(moby + 0x94) = 0;
        moby[0x31] = 0;
        *(unsigned short *)(moby + 0x34) |= 1;
        if (*(int *)(data + 0x178) != 0) return;
        flag = menu_item_enabled_115_q + 1;
        if (flag[0xA] != 0) return;
        flag[0xA] = 1;
        PlayBankSound_q(1, 0, 0);
        ShowBanner_q(0x542D, -1);
        return;
    }
    case 15: {
        int i;
        char *m = 0;
        for (i = 0; i < 0x30; i++) {
            if (blarg_camera_slot_used[i] != 0) {
                m = blarg_camera_slots + i * 0xA0;
                if (*(short *)(m + 0x86) == 0x12) {
                    SetTableEntryFlagIfKind18_q(m);
                    break;
                }
            }
        }
        if (level_game_mode_q_q == 2) {
            set_moby_and_links_to_states_4_and_5_q(level_mobys_q_q + (*(int *)(data + 0xFC) << 8));
            DeleteMoby_q(moby);
        }
        break;
    }
    }
}

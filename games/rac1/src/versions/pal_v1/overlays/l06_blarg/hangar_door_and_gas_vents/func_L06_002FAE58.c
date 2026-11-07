void alien_queen_update(unsigned char *moby) {
    struct AlienQueenVars *data = (struct AlienQueenVars *)((Moby *)moby)->pvars;
    struct Moby *target;
    float v[4];

    update_alien_queen_hit_response_and_recovery(moby);
    target = (struct Moby *)(*(char **)&data->target);
    Manipulator_ApplyScale_q(moby, alien_queen_scale_manip_flag, 1.9f, &data->manip);
    if ((unsigned char)((Moby *)moby)->visible != 0 &&
        fast_vec_dist(&((struct Moby *)moby)->pos, blarg_camera_pos_q) < 45.0f) {
        set_ground_height_band(moby);
        ((Moby *)moby)->shadow_range = 0x24;
    }
    if ((unsigned char)((Moby *)moby)->state >= 3 && (unsigned char)((Moby *)moby)->state <= 13) {
        data->hud_handle = queue_animation_update_q(0x16, 0xFFFF, reset_numeric_hud_element_q, Hud_UpdateCounter,
                                                    draw_segmented_hud_meter_base_q + 0x20, &data->hud_value, 0x1EF);
    }
    switch ((unsigned char)((Moby *)moby)->state) {
        case 0: {
            int *p;
            data->health = 30.0f;
            data->unk_24 = 0x1E;
            data->unk_29 = 1;
            data->unk_28 = 3;
            data->hit_state = 1;
            data->unk_58 = truncate_float_to_s32_q(14.0f);
            data->unk_5A = truncate_float_to_s32_q(18.4f);
            data->timer = scale_ticks(0x4B0);
            data->unk_178 = 0;
            qcopy(&data->home_x, &((struct Moby *)moby)->pos);
            data->hud_handle = -1;
            data->path_len = *blarg_paths_q[data->path];
            p = blarg_paths_q[data->path];
            lerp_vector(data->path_mid, p + 4, (char *)p + ((*p / 2) * 16 + 16), 0.5f);
            data->path_mid_dist = vec_distance_xy(&data->home_x, data->path_mid);
            data->angle = random_angle_radians();
            data->timer2 = scale_ticks(0x1E);
            *(int *)&((Moby *)moby)->collision = 0;
            ((Moby *)moby)->flags = (((Moby *)moby)->flags | 0x4001) & 0xEFFF;
            ((Moby *)moby)->pos.z = ((Moby *)moby)->pos.z + 17.5f;
            set_moby_list_state_18_q(data->list_f4);
            if (menu_item_enabled_19_q[4] != 0 && race_gates_base_q[8] == 0) {
                DeleteMoby(moby);
                return;
            }
            ((Moby *)moby)->state = 1;
            break;
        }
        case 1:
            if (data->phase == 2)
                return;
            if (target->oclass != MOBY_CLASS_RATCHET)
                return;
            if (data->unk_170 == 0) {
                data->unk_170 = 1;
                data->timer2 = scale_ticks(10);
                break;
            }
            if (data->timer2 != 0)
                return;
            set_moby_and_links_to_states_8_and_9_q(level_mobys_q_q + (data->door_a << 8));
            set_moby_and_links_to_states_8_and_9_q(level_mobys_q_q + (data->door_b << 8));
            set_listed_class_16f_state_2_and_data_28_to_one_q(data->list_16f);
            data->unk_170 = 0;
            ((Moby *)moby)->state = 2;
            *(int *)&((Moby *)moby)->collision = *(int *)(*(char **)&((struct Moby *)moby)->pclass + 0x10);
            ((Moby *)moby)->flags = (((Moby *)moby)->flags | 0x1000) & 0xFFFE;
            if ((unsigned char)((Moby *)moby)->prev_seq != 8) {
                MobyBlendAnimation(moby, 8, 0, scale_ticks(1));
            }
            start_alien_queen_intro_q(moby);
            return;
        case 2: {
            float g = g_frame_time_sq_q * 30.0f;
            ((Moby *)moby)->pos.z = ((Moby *)moby)->pos.z + data->vel_z;
            data->vel_z = data->vel_z - g;
            if ((unsigned char)((Moby *)moby)->prev_seq == 8) {
                if (absolute_float((((Moby *)moby)->pos.z - data->home_z) / data->vel_z) <= 6.0f &&
                    (unsigned char)((Moby *)moby)->prev_seq != 9) {
                    MobyBlendAnimation(moby, 9, 0, scale_ticks(5));
                }
            } else {
                if (((Moby *)moby)->pos.z < data->home_z) {
                    ((Moby *)moby)->pos.z = data->home_z;
                }
                if (((Moby *)moby)->anim_time == 0.0f && (((Moby *)moby)->anim_flags & 2)) {
                    if ((unsigned char)((Moby *)moby)->prev_seq == 9) {
                        MobyBlendAnimation(moby, 6, 0, scale_ticks(0x1E));
                    } else if ((unsigned char)((Moby *)moby)->prev_seq != 1) {
                        MobyBlendAnimation(moby, 1, 0, scale_ticks(0x1E));
                    }
                }
            }
            if (update_alien_queen_intro_camera_q(moby) != 0) {
                ((Moby *)moby)->state = 7;
                finish_alien_queen_intro_q(moby);
            }
            break;
        }
        case 3:
            if ((unsigned char)((Moby *)moby)->prev_seq == 6 && ((Moby *)moby)->anim_time == 0.0f &&
                (((Moby *)moby)->anim_flags & 2)) {
                MobyBlendAnimation(moby, 1, 0, scale_ticks(0x14));
                activate_listed_state_15_mobys_at_raised_positions_q(moby);
            }
            if (update_alien_queen_first_recovery_camera_q(moby) != 0) {
                ((Moby *)moby)->state = 4;
                finish_alien_queen_first_recovery(moby);
                return;
            }
            break;
        case 5:
            if ((unsigned char)((Moby *)moby)->prev_seq == 6 && ((Moby *)moby)->anim_time == 0.0f &&
                (((Moby *)moby)->anim_flags & 2)) {
                MobyBlendAnimation(moby, 1, 0, scale_ticks(0x14));
                clear_moby_list_state_18_q(data->list_f4);
            }
            if (update_alien_queen_second_recovery_camera(moby) != 0) {
                ((Moby *)moby)->state = 6;
                finish_alien_queen_second_recovery(moby);
            }
            break;
        case 4:
            activate_listed_state_15_mobys_at_raised_positions_q(moby);
            /* fall through */
        case 6:
            update_alien_queen_orbit_movement_q(moby);
            if (((unsigned char)((Moby *)moby)->state == 4 &&
                 check_all_listed_frog_states_fe_fd_or_10_q(data->unk_F0) != 0) ||
                ((unsigned char)((Moby *)moby)->state == 6 &&
                 count_active_moby_group_members(data->list_f4, -1) == 0)) {
                ((Moby *)moby)->state = 8;
                data->timer = scale_ticks(0x4B0);
                if ((unsigned char)((Moby *)moby)->prev_seq != 3) {
                    MobyBlendAnimation(moby, 3, 0, scale_ticks(0x14));
                }
            }
            return;
        case 7:
            if (data->phase == 2)
                break;
            ((Moby *)moby)->state = 8;
            if ((unsigned char)((Moby *)moby)->prev_seq == 3)
                break;
            MobyBlendAnimation(moby, 3, 0, scale_ticks(0x14));
            return;
        case 8: {
            float dist = vec_distance_xy(&((struct Moby *)moby)->pos, &target->pos);
            float turn = FastDiffRots(((Moby *)moby)->rot.z, fast_atan2(target->pos.x - ((Moby *)moby)->pos.x,
                                                                        target->pos.y - ((Moby *)moby)->pos.y));
            SteerAngleWithVelocity_q(
                &((struct Moby *)moby)->rot.z,
                fast_atan2(target->pos.x - ((Moby *)moby)->pos.x, target->pos.y - ((Moby *)moby)->pos.y), &data->unk_E0,
                g_frame_time_sq_q * 6.2831855f, g_frame_time_sq_q * 6.2831855f, g_frame_time_q * 3.1415927f);
            if (turn < 0.7853982f && alien_queen_attack_range < dist) {
                ApproachValue_q(&data->unk_E8, alien_queen_walk_speed * g_frame_time_q,
                                alien_queen_movement_acceleration_gp * g_frame_time_sq_q);
            } else if (dist < alien_queen_back_off_range) {
                ApproachValue_q(&data->unk_E8, -(alien_queen_walk_speed * g_frame_time_q),
                                alien_queen_movement_acceleration_gp * g_frame_time_sq_q);
            } else {
                ApproachValue_q(&data->unk_E8, 0.0f, g_frame_time_sq_q * 8.0f);
            }
            FastVecSub(v, &target->pos, &((struct Moby *)moby)->pos);
            v[2] = 0.0f;
            vec_scale_to_length(v, v, data->unk_E8);
            FastVecAdd(&((struct Moby *)moby)->pos, &((struct Moby *)moby)->pos, v);
            resolve_point_path_segment_xy_clearance_q(data->path, &((struct Moby *)moby)->pos, 1.0f,
                                                      &((struct Moby *)moby)->pos);
            if (data->phase == 2) {
                ((Moby *)moby)->state = 0xD;
                break;
            }
            if (fast_dec_timer(&data->timer) != 0) {
                data->timer = scale_ticks(0x168);
                ((Moby *)moby)->state = 0xC;
                break;
            }
            if (dist < alien_queen_attack_range && turn < alien_queen_attack_arc * 0.017453292f) {
                ((Moby *)moby)->state = 9;
                ((Moby *)moby)->command = 7;
                if ((unsigned char)((Moby *)moby)->prev_seq != 4) {
                    MobyBlendAnimation(moby, 4, 0, scale_ticks(0x14));
                    return;
                }
            }
            break;
        }
        case 12:
            update_alien_queen_orbit_movement_q(moby);
            if (fast_dec_timer(&data->timer) != 0) {
                data->timer = scale_ticks(0x4B0);
                ((Moby *)moby)->state = 0xB;
                ((Moby *)moby)->command = 7;
                if ((unsigned char)((Moby *)moby)->prev_seq != 6) {
                    MobyBlendAnimation(moby, 6, 0, scale_ticks(0x14));
                }
            }
            return;
        case 9:
            fast_dec_timer(&data->timer);
            SteerAngleWithVelocity_q(&((struct Moby *)moby)->rot.z, ((Moby *)moby)->rot.z, &data->unk_E0,
                                     g_frame_time_sq_q * 6.2831855f, g_frame_time_sq_q * 6.2831855f,
                                     g_frame_time_q * 3.1415927f);
            if (((Moby *)moby)->anim_flags & 2) {
                ((Moby *)moby)->state = 0xA;
                if ((unsigned char)((Moby *)moby)->prev_seq != 5) {
                    MobyBlendAnimation(moby, 5, 0, scale_ticks(0x14));
                    return;
                }
            }
            break;
        case 10: {
            QueenProbe probe;
            float buf[4];
            fast_dec_timer(&data->timer);
            if (1.0f < compute_interpolated_record_value(moby) && compute_interpolated_record_value(moby) < 8.0f) {
                v[0] = FastCos(((Moby *)moby)->rot.z) * 0.45f;
                v[1] = FastSin(((Moby *)moby)->rot.z) * 0.45f;
                v[2] = 0.0f;
                probe.owner = (char *)moby;
                probe.flags = 1;
                probe.scale = 1.0f;
                probe.count = 1;
                probe.dir[0] = FastCos(((Moby *)moby)->rot.z);
                probe.dir[1] = FastSin(((Moby *)moby)->rot.z);
                probe.dir[2] = 0.0f;
                moby_point_position(moby, 0, buf);
                coll_sphere_mobys_q(buf, 0.5f, 0, moby, &probe);
                moby_point_position(moby, 1, buf);
                coll_sphere_mobys_q(buf, 0.5f, 0, moby, &probe);
                moby_point_position(moby, 2, buf);
                coll_sphere_mobys_q(buf, 0.5f, 0, moby, &probe);
                moby_point_position(moby, 3, buf);
                coll_sphere_mobys_q(buf, 0.5f, 0, moby, &probe);
                moby_point_position(moby, 4, buf);
                coll_sphere_mobys_q(buf, 0.5f, 0, moby, &probe);
                moby_point_position(moby, 5, buf);
                coll_sphere_mobys_q(buf, 1.1f, 0, moby, &probe);
                moby_point_position(moby, 6, buf);
                coll_sphere_mobys_q(buf, 1.1f, 0, moby, &probe);
                return;
            }
            if (((Moby *)moby)->anim_flags & 2) {
                ((Moby *)moby)->state = ((Moby *)moby)->command;
                if ((unsigned char)((Moby *)moby)->prev_seq != 1) {
                    MobyBlendAnimation(moby, 1, 0, scale_ticks(0x14));
                }
            }
            return;
        }
        case 11:
            if (((Moby *)moby)->anim_flags & 2) {
                ((Moby *)moby)->state = ((Moby *)moby)->command;
                if ((unsigned char)((Moby *)moby)->prev_seq != 1) {
                    MobyBlendAnimation(moby, 1, 0, scale_ticks(0x14));
                }
            }
            return;
        case 13: {
            char *pos = (unsigned char *)&((struct Moby *)moby)->pos;
            char *home = (unsigned char *)&data->home_x;
            float dist = vec_distance_xy(pos, home);
            float turn = FastDiffRots(((Moby *)moby)->rot.z, fast_atan2(data->home_x - ((Moby *)moby)->pos.x,
                                                                        data->home_y - ((Moby *)moby)->pos.y));
            SteerAngleWithVelocity_q(
                &((struct Moby *)moby)->rot.z,
                fast_atan2(data->home_x - ((Moby *)moby)->pos.x, data->home_y - ((Moby *)moby)->pos.y), &data->unk_E0,
                g_frame_time_sq_q * 6.2831855f, g_frame_time_sq_q * 6.2831855f, g_frame_time_q * 3.1415927f);
            if (turn < 0.7853982f) {
                FastVecSub(v, home, pos);
                v[2] = 0.0f;
                vec_scale_to_length(v, v, alien_queen_walk_speed * g_frame_time_q);
                FastVecAdd(pos, pos, v);
                resolve_point_path_segment_xy_clearance_q(data->path, pos, 1.0f, pos);
            }
            if (data->phase != 2) {
                ((Moby *)moby)->state = 7;
                break;
            }
            if (dist < 1.0f) {
                ((Moby *)moby)->state = 7;
                if ((unsigned char)((Moby *)moby)->prev_seq != 1) {
                    MobyBlendAnimation(moby, 1, 0, scale_ticks(0x14));
                    return;
                }
            }
            break;
        }
        case 14: {
            unsigned char *flag;
            menu_item_enabled_19_q[4] = 1;
            data->hud_value = 0;
            ((Moby *)moby)->update_dist = 0xFF;
            if (data->hit_state == 2) {
                set_moby_and_links_to_states_4_and_5_q(level_mobys_q_q + (data->door_b << 8));
                set_listed_class_16f_state_2_and_clear_data_28_q(data->list_16f);
                ((Moby *)moby)->flags |= 1;
                ((Moby *)moby)->state = 0xF;
                ((Moby *)moby)->visible = 0;
                *(int *)&((Moby *)moby)->collision = 0;
                if ((unsigned char)((Moby *)moby)->prev_seq != 1) {
                    MobyBlendAnimation(moby, 1, 0, scale_ticks(0x14));
                    return;
                }
                break;
            }
            if (!(((Moby *)moby)->anim_flags & 2))
                break;
            qcopy(v, &((struct Moby *)moby)->pos);
            v[2] = v[2] + 1.0f;
            spawn_color_swapped_explosion_burst(moby, v, -1, 2.0f, 20.0f);
            spawn_moby_break_fragments(moby, 0x6A2, 1, 0x6A3, 3, 6, 2);
            set_moby_and_links_to_states_4_and_5_q(level_mobys_q_q + (data->door_b << 8));
            set_listed_class_16f_state_2_and_clear_data_28_q(data->list_16f);
            ((Moby *)moby)->state = 0xF;
            if ((unsigned char)((Moby *)moby)->prev_seq != 1) {
                MobyBlendAnimation(moby, 1, 0, scale_ticks(0x14));
            }
            *(int *)&((Moby *)moby)->collision = 0;
            ((Moby *)moby)->visible = 0;
            ((Moby *)moby)->flags |= 1;
            if (data->unk_178 != 0)
                return;
            flag = gSkillPoints;
            if (flag[0xA] != 0)
                return;
            flag[0xA] = 1;
            allocate_voice_for_bank_entry(1, 0, 0);
            queue_dialogue_message(0x53D6, -1);
            return;
        }
        case 15: {
            int i;
            char *m = 0;
            for (i = 0; i < 0x30; i++) {
                if (l06_post_death_scan_record_gates[i] != 0) {
                    m = l06_post_death_scan_records + i * 0xA0;
                    if (*(short *)(m + 0x86) == 0x12) {
                        release_moby_camera_control(m);
                        break;
                    }
                }
            }
            if (level_game_mode_q_q == 2) {
                set_moby_and_links_to_states_4_and_5_q(level_mobys_q_q + (data->door_a << 8));
                DeleteMoby(moby);
            }
            break;
        }
    }
}

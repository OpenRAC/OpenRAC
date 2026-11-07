typedef struct BossShared46 {
    char p0[0x10];
    float f10[4];
    float f20[4];
    int f30;
    int f34;
    char p38[0xE];
    short f46;
} BossShared46;
void drek_final_boss_robot_update(Moby *moby) {
    BossData *d = (BossData *)(*(char **)&moby->pvars);
    Moby *tgt;
    float v0[4];
    float v1[4];
    float v2[4];
    float v3[4];
    float v4[4];
    float slow;
    float ang;
    float *aim;
    float fast;

    count_drek_boss_platform_event_once((char *)moby);
    update_drek_boss_damage_and_phase_transitions((char *)moby);
    tgt = d->fB0;
    /* The file defines this one without parameters; retail passes the moby. */
    attach_drek_boss_big_head_manipulator((char *)moby);
    {
        char *o = d->f3A4;
        if (o != 0 && o[0x20] >= 0) {
            Manipulator_ApplyScale(drek_boss_big_head_manipulator_scale_gp, (int)o, 0, d->f1C0);
        }
    }
    if (moby->visible != 0 && fast_vec_dist(((f32 *)&moby->pos), veldin2_camera_pos) < 32.0f) {
        set_ground_height_band(moby);
        moby->shadow_range = 0x1A;
    }
    switch (moby->state) {
        case 0x0: {
            int i;

            qcopy(d->f3C0, ((f32 *)&moby->pos));
            moby->state = 1;
            moby->update_dist = 0xFF;
            moby->draw_dist = 0x200;
            moby->flags |= 0x41;
            moby->collision = 0;
            d->f28 = 3;
            d->f20 = 500.0f;
            d->f30 = 4.0f;
            d->f24 = 500;
            d->f340 = veldin2_paths[d->f260[3]];
            d->f398 = d->f204[0];
            d->f348 = 1;
            d->f368 = -1;
            d->f38C = 350.0f;
            d->f334 = veldin2_moby_table + d->f220 * 0x100;
            d->f3D0 = 0.75f;
            d->f374 = 0.0f;
            for (i = 0; i < 3; i++) {
                hide_drek_boss_moby_group((int)moby, d->f204[i]);
            }
            break;
        }
        case 0x7:
            if (veldin2_cinematic_mode != 2) {
                moby->flags &= 0xFFBE;
                moby->collision = (*(int **)&moby->pclass)[4];
                moby->state = *(unsigned char *)&d->f39C;
                if (moby->state == 5) {
                    veldin2_screen_fade = 1.0f;
                }
                if (moby->state == 0x19) {
                    hero_respawn();
                }
                break;
            }
            if (((BossShared *)&veldin2_cutscene_state)->f30 == 2) {
                if (scale_ticks(0xA28) <= ((BossShared *)&veldin2_cutscene_state)->f34 &&
                    ((BossShared *)&veldin2_cutscene_state)->f34 <= scale_ticks(0xB36)) {
                    moby_init_action_mode(d->f330);
                }
            } else if (((BossShared *)&veldin2_cutscene_state)->f30 == 3) {
                if (scale_ticks(0x3B6) < ((BossShared *)&veldin2_cutscene_state)->f34 &&
                    ((BossShared *)&veldin2_cutscene_state)->f34 < scale_ticks(0x3C0)) {
                    set_thruster_pack_lock_green_list_data_fields(d->f200);
                }
                if (drek_boss_camera_reset_done == 0) {
                    char *b = veldin2_arena_nodes + d->f294 * 0x80;
                    jet_reset_camera(b + 0x30, b + 0x70);
                    drek_boss_camera_reset_done = 1;
                }
            }
            break;
        case 0x1:
            if (is_point_inside_clip_volume(g_hero.motion.pos.f, d->f274) != 0 && g_hero.vehicle_mode == 2) {
                moby->state = 7;
                moby->flags |= 0x41;
                start_scene(0);
                d->f39C = 2;
                d->f358 = scale_ticks(600);
                d->f344 = d->f250[3];
                if (moby->prev_seq != 10) {
                    MobyBlendAnimation(moby, 10, 0, scale_ticks(10));
                }
            } else {
                int *t = veldin2_paths[d->f270];
                if (point_in_path_xy(g_hero.motion.pos.f, t + 4, *t) != 0) {
                    moby->flags &= 0xFFBE;
                    moby->collision = (*(int **)&moby->pclass)[4];
                    moby->state = 10;
                    {
                        int *b = veldin2_paths[d->f260[0]];
                        d->f340 = b;
                        qcopy(d->f3C0, b + 4);
                    }
                    moby->rot.z = fast_atan2(g_hero.motion.pos.f[0] - d->f3C0[0], g_hero.motion.pos.f[1] - d->f3C0[1]);
                    show_drek_boss_moby_group((int)moby, d->f204[0]);
                    d->f38C = d->f38C * 0.85714287f;
                } else if (fast_vec_dist(g_hero.motion.pos.f, veldin2_arena_nodes + (d->f250[0] << 7) + 0x30) < 50.0f) {
                    moby->flags &= 0xFFBE;
                    moby->collision = (*(int **)&moby->pclass)[4];
                    moby->state = 0xC;
                    {
                        int *p = veldin2_paths[d->f260[0]];
                        d->f340 = p;
                        qcopy(d->f3C0, (char *)p + *p * 16);
                    }
                    d->f344 = d->f250[0];
                    d->f38C = d->f38C * 0.85714287f;
                } else if (fast_vec_dist(g_hero.motion.pos.f, veldin2_arena_nodes + (d->f250[1] << 7) + 0x30) < 50.0f) {
                    moby->flags &= 0xFFBE;
                    moby->collision = (*(int **)&moby->pclass)[4];
                    d->f34C = 1;
                    d->f38C = d->f38C * 0.71428573f;
                    moby->state = 0xC;
                    {
                        int *p = veldin2_paths[d->f260[1]];
                        d->f340 = p;
                        qcopy(d->f3C0, (char *)p + *p * 16);
                    }
                    d->f344 = d->f250[1];
                } else if (fast_vec_dist(g_hero.motion.pos.f, veldin2_arena_nodes + (d->f250[2] << 7) + 0x30) < 50.0f) {
                    if (drek_boss_camera_reset_done != 0) {
                        set_thruster_pack_lock_green_list_data_fields(d->f200);
                        if (voice_is_playing(moby, d->f3B0) != 0) {
                            int i = d->f3B0;
                            if (i != -1) {
                                char *e = voice_pool + i * 0x70;
                                if (*(Moby **)(e + 0x88) == moby && *(unsigned char *)(e + 0x74) != 0) {
                                    Sound_StopSlot(i);
                                }
                            }
                            d->f3B0 = -1;
                        }
                        moby->state = 0x1B;
                        moby->collision = 0;
                        moby->flags |= 0x41;
                        d->f34C = 7;
                    } else {
                        moby->flags &= 0xFFBE;
                        moby->collision = (*(int **)&moby->pclass)[4];
                        d->f34C = 2;
                        d->f38C = d->f38C * 0.5714286f;
                        moby->state = 0xC;
                        {
                            int *p = veldin2_paths[d->f260[2]];
                            d->f340 = p;
                            qcopy(d->f3C0, (char *)p + *p * 16);
                        }
                        d->f344 = d->f250[2];
                        d->f358 = scale_ticks(0x168);
                    }
                }
            }
            break;
        case 0x2:
            aim = &moby->rot.z;
            ang = fast_atan2(g_hero.motion.pos.f[0] - moby->pos.x, g_hero.motion.pos.f[1] - moby->pos.y);
            SteerAngleWithVelocity(aim, &d->f370, ang, g_frame_time_sq * 6.2831855f, g_frame_time_sq * 6.2831855f,
                                   g_frame_time * 6.2831855f);
            update_drek_boss_arena_circling_movement((char *)moby);
            if (d->f38C <= 301.0f) {
                if (g_hero.vehicle_mode != 0) {
                    g_hero.moby->collision = 0;
                    g_hero.moby->flags |= 0x41;
                }
                restore_saved_hero_moby_and_clear_vehicle_mode();
                {
                    char *b = veldin2_arena_nodes + d->f2A0 * 0x80;
                    set_race_camera(b + 0x30, b + 0x70, 0, 1);
                }
                jet_clear_persistent(d->f2B0);
                jet_reset_camera(g_hero.motion.pos.f, g_hero.motion.rot.f);
                show_drek_boss_moby_group((int)moby, d->f204[0]);
                start_scene(1);
                moby->state = 7;
                moby->flags |= 0x41;
                if (moby->prev_seq != 10) {
                    MobyBlendAnimation(moby, 10, 0, scale_ticks(0x14));
                }
                d->f39C = 10;
                d->f348 = 0;
                d->f374 = 0.0f;
                {
                    int *b = veldin2_paths[d->f260[0]];
                    d->f340 = b;
                    qcopy(d->f3C0, b + 4);
                }
                moby->rot.z = fast_atan2(g_hero.motion.pos.f[0] - d->f3C0[0], g_hero.motion.pos.f[1] - d->f3C0[1]);
                d->f368 = -1;
                break;
            }
            if (moby->prev_seq == 10 && (is_value_within_interpolated_window((char *)moby, 1.0f) != 0 ||
                                         is_value_within_interpolated_window((char *)moby, 16.0f) != 0)) {
                qcopy(v0, g_hero.motion.near_ground_point.f);
                if (absolute_float(v0[2] - d->f3C0[2]) < 5.0f) {
                    if ((float)moby->frame < 15.0f) {
                        moby_point_position(moby, 2, v1);
                    } else {
                        moby_point_position(moby, 3, v1);
                    }
                    vec_scale_to_length(v2, ((f32 *)&moby->rot_row0), g_frame_time * 20.0f);
                    {
                        float a = randf_sym(0.08726646f, 0.5235988f);
                        float c = -random_float_between(0.08726646f, 0.17453292f);
                        float e = vec_distance_xy(((f32 *)&moby->pos), g_hero.motion.pos.f) * 3.0f;
                        if (e > 120.0f) {
                            e = 120.0f;
                        } else if (e < 60.0f) {
                            e = 60.0f;
                        }
                        create_rocket_moby(v1, v2, v0, a, c, truncate_float_to_s32(multiply_global_factor(e)), 0);
                    }
                }
            }
            if ((moby->anim_flags & 2) != 0 && moby->prev_seq != 10) {
                MobyBlendAnimation(moby, 10, 0, scale_ticks(0x14));
            }
            break;
        case 0x9: {
            float lvl;
            int hit;

            aim = &moby->rot.z;
            ang = fast_atan2(g_hero.motion.pos.f[0] - moby->pos.x, g_hero.motion.pos.f[1] - moby->pos.y);
            SteerAngleWithVelocity(aim, &d->f370, ang, g_frame_time_sq * 6.2831855f, g_frame_time_sq * 6.2831855f,
                                   g_frame_time * 6.2831855f);
            qcopy(v0, d->f340 + 4);
            hit = advance_drek_boss_planar_position_toward_target((char *)moby, 20.0f, v0);
            lvl = get_camera_transition_progress(d->f334);
            if (moby->prev_seq == 7) {
                if ((moby->anim_flags & 2) != 0) {
                    MobyBlendAnimation(moby, 8, 0, scale_ticks(10));
                }
            } else {
                FastVecSub(v1, d->f340 + 4, d->f3C0);
                vec_scale_to_length(v1, v1, g_frame_time * 5.0f);
                FastVecAdd(d->f3C0, d->f3C0, v1);
            }
            if (lvl > 0.5f && lvl < 0.7f) {
                if (moby->prev_seq != 9) {
                    MobyBlendAnimation(moby, 9, 0, 0);
                }
                if (d->f33C == 0) {
                    break;
                }
                moby_point_position(moby, 1, v1);
                qcopy(v2, veldin2_arena_nodes + d->f344 * 0x80 + 0x30);
                v2[2] = v2[2] + 10.0f;
                v2[2] = probe_ground_height(v2, 0, 0.5f);
                refresh_moby_position_and_activate(d->f33C, v1, veldin2_arena_nodes + d->f344 * 0x80 + 0x30, 1, 1.0f);
                d->f33C = 0;
            } else if (lvl >= 0.729f) {
                int i;

                if (moby->prev_seq != 0) {
                    MobyBlendAnimation(moby, 0, 0, scale_ticks(0x14));
                }
                if (hit != 0) {
                    moby->state = 10;
                }
                for (i = 0; i < 4; i++) {
                    int k = d->f34C == 1 ? d->f230[i] : d->f240[i];

                    activate_drek_boss_class_0565_moby(veldin2_moby_table + k * 0x100);
                }
            }
            if (d->f33C != 0) {
                float f;

                moby_point_position(moby, 1, v1);
                f = lvl + lvl;
                if (f > 1.0f) {
                    f = 1.0f;
                }
                refresh_moby_position_and_activate(d->f33C, v1, veldin2_zero_vec, 0, f);
            }
            break;
        }
        case 0xA: {
            unsigned char *h;

            aim = &moby->rot.z;
            ang = fast_atan2(g_hero.motion.pos.f[0] - moby->pos.x, g_hero.motion.pos.f[1] - moby->pos.y);
            SteerAngleWithVelocity(aim, &d->f370, ang, g_frame_time_sq * 6.2831855f, g_frame_time_sq * 6.2831855f,
                                   g_frame_time * 6.2831855f);
            ApproachValue(&d->f374, g_frame_time * 12.0f, g_frame_time_sq * 20.0f);
            if (d->f374 != 0.0f) {
                FastVecSub(v0, (char *)d->f340 + (d->f348 * 0x10 + 0x10), d->f3C0);
                if (FastVecLength(v0) <= d->f374) {
                    d->f348 = d->f348 + 1;
                } else {
                    vec_scale_to_length(v0, v0, d->f374);
                }
                FastVecAdd(d->f3C0, d->f3C0, v0);
            }
            h = select_drek_boss_platform_firing_target((char *)moby);
            if (h != 0 && (is_value_within_interpolated_window((char *)moby, 1.0f) != 0 ||
                           is_value_within_interpolated_window((char *)moby, 16.0f) != 0)) {
                qcopy(v2, h + 0x10);
                qcopy(v0, d->f3C0);
                FastVecAdd(v0, v0, ((f32 *)&moby->rot_row0));
                v0[2] = v0[2] + 6.0f;
                vec_scale_to_length(v1, ((f32 *)&moby->rot_row0), g_frame_time * 10.0f);
                {
                    float a = randf_sym(0.5235988f, 0.7853982f);
                    float c = -random_float_between(0.34906584f, 0.5235988f);
                    create_rocket_moby(v0, v1, v2, a, c, scale_ticks(0x3C), 5);
                }
            }
            if (d->f348 == *d->f340 - 2) {
                if (moby->prev_seq != 0) {
                    MobyBlendAnimation(moby, 0, 0, scale_ticks(0x14));
                }
                moby->state = 0xB;
            }
            break;
        }
        case 0xB:
            if (d->f348 != *d->f340) {
                ApproachValue(&d->f374, g_frame_time * 10.0f, g_frame_time_sq * 20.0f);
                FastVecSub(v0, (char *)d->f340 + (d->f348 * 0x10 + 0x10), d->f3C0);
                if (FastVecLength(v0) <= d->f374) {
                    d->f348 = d->f348 + 1;
                } else {
                    vec_scale_to_length(v0, v0, d->f374);
                }
                FastVecAdd(d->f3C0, d->f3C0, v0);
                break;
            }
            if (vec_distance_xy(((f32 *)&moby->pos), g_hero.motion.pos.f) < 50.0f && g_hero.air_frames.s == 0 &&
                (g_hero.ground_moby == 0 || g_hero.ground_moby->oclass != MOBY_CLASS_FLOATING_PLATFORM_587)) {
                int *p = d->f340;

                d->f374 = 0.0f;
                qcopy(d->f3C0, (char *)p + *p * 16);
                d->f344 = d->f250[d->f34C];
                if (d->f34C == 2) {
                    if (drek_boss_intro_started == 0) {
                        moby->state = 4;
                    } else {
                        moby->state = 0x15;
                        d->f330 = find_thruster_pack_lock_green_moby_with_largest_xy_angle_in_list(
                            d->f3C0, g_hero.motion.pos.f, d->f200);
                    }
                    break;
                }
                moby->state = 0xC;
            }
            break;
        case 0x4:
            if (g_hero.air_frames.s == 0 &&
                (g_hero.ground_moby == 0 || g_hero.ground_moby->oclass != MOBY_CLASS_FLOATING_PLATFORM_587)) {
                int i;

                drek_boss_intro_started = 1;
                moby->state = 7;
                d->f39C = 5;
                d->f330 = (Moby *)find_nearest_thruster_pack_lock_green_moby_in_list(g_hero.motion.pos.f, d->f200);
                moby->flags |= 0x41;
                start_scene(2);
                for (i = 0; i < 3; i++) {
                    hide_drek_boss_moby_group((int)moby, d->f204[i]);
                }
            }
            break;
        case 0x5: {
            unsigned char *b = veldin2_moby_table + d->f21C * 0x100;

            b += 0x10;
            *(int *)&drek_boss_camera_move_request = 0;
            v0[0] = FastCos(0.0f) * 6.0f;
            v0[1] = FastSin(0.0f) * 6.0f;
            v0[2] = 0.0f;
            v1[0] = FastCos(0.7853982f) * 6.0f;
            v1[1] = FastSin(0.7853982f) * 6.0f;
            v1[2] = 0.0f;
            FastVecAdd(v0, v0, b);
            FastVecAdd(v1, v1, b);
            v0[2] = v0[2] + 5.0f;
            v1[2] = v1[2] + 7.0f;
            clear_u64_value(v2);
            v2[2] = 3.1415927f;
            clear_u64_value(v3);
            v3[2] = 3.9269907f;
            v2[1] = 0.17453292f;
            v3[1] = 0.5235988f;
            {
                /* In locals: retail does not load them again after the copy. */
                char *recs = veldin2_arena_nodes;
                int idx = d->f298;

                qcopy(v4, recs + idx * 0x80 + 0x30);
                v4[3] = *(float *)(recs + idx * 0x80 + 0x78);
            }
            start_camera_transition((char *)d->f334, v0, v1, v2, v3, v4, drek_boss_camera_move_ticks,
                                    drek_boss_camera_move_hold_ticks, 1);
            g_hero.pending_item[3] = 3;
            moby->state = 8;
            request_thruster_pack_lock_red_wait(veldin2_moby_table + d->f21C * 0x100, scale_ticks(0x708));
            break;
        }
        case 0x3:
            if (classify_moby_state_two_or_three(d->f334) == 2) {
                qcopy(d->f3C0, d->f300);
                qcopy(((f32 *)&moby->rot), d->f310);
                moby->state = 9;
                if (moby->prev_seq != 7) {
                    MobyBlendAnimation(moby, 7, 0, scale_ticks(0x14));
                }
                show_drek_boss_moby_group((int)moby, d->f398);
                hide_drek_boss_moby_group((int)moby, d->f204[d->f34C - 1]);
                moby_point_position(moby, 1, v0);
                d->f33C = (unsigned char *)spawn_drek_boss_electric_ball(moby, v0, 300);
            }
            break;
        case 0x8:
            moby->flags |= 0x41;
            if (get_camera_transition_progress(d->f334) >= 1.0f) {
                moby->state = 0xC;
                moby->flags &= 0xFFBE;
                if (moby->prev_seq != 0) {
                    MobyBlendAnimation(moby, 0, 0, scale_ticks(10));
                }
            }
            break;
        case 0x14:
            if (fast_dec_timer(&d->f354) != 0) {
                d->f354 = scale_ticks(0x14);
                qcopy(v0, d->f3C0);
                v0[2] = v0[2] + 4.0f;
                if (activate_drek_fight_missiles_in_group((char *)moby, d->f214, v0) != 0) {
                    moby->state = 0xC;
                }
            }
            break;
        case 0xC:
            aim = &moby->rot.z;
            ang = fast_atan2(tgt->pos.x - moby->pos.x, tgt->pos.y - moby->pos.y);
            SteerAngleWithVelocity(aim, &d->f370, ang, g_frame_time_sq * 6.2831855f, g_frame_time_sq * 6.2831855f,
                                   g_frame_time * 6.2831855f);
            update_drek_boss_arena_circling_movement((char *)moby);
            if ((moby->anim_flags & 2) == 0) {
                break;
            }
            switch (select_drek_boss_next_action((char *)moby)) {
                case 0xF:
                    moby->state = 0xF;
                    if (moby->prev_seq != 10) {
                        MobyBlendAnimation(moby, 10, 0, scale_ticks(0x14));
                    }
                    d->f354 = 0;
                    d->f358 = scale_ticks(600);
                    d->f35C = scale_ticks(0x168);
                    break;
                case 0xD:
                    moby->state = 0xD;
                    if (moby->prev_seq != 2) {
                        MobyBlendAnimation(moby, 2, 0, scale_ticks(0x14));
                    }
                    d->f354 = 0;
                    d->f358 = scale_ticks(random_integer_below(3) * 0x5A + 0x14A);
                    d->f35C = scale_ticks((6 - d->f34C) * 0x1E);
                    d->f3BC = random_float_between(-0.333f, 0.333f);
                    break;
                case 0x11:
                    moby->state = 0x11;
                    if (moby->prev_seq != 11) {
                        MobyBlendAnimation(moby, 0xB, 0, scale_ticks(0x14));
                    }
                    d->f354 = scale_ticks(10);
                    d->f358 = scale_ticks(600);
                    break;
                default:
                    moby->state = 0x10;
                    if (moby->prev_seq != 6) {
                        MobyBlendAnimation(moby, 6, 0, scale_ticks(0x14));
                    }
                    d->f354 = scale_ticks(0xF);
                    d->f358 = scale_ticks(0x168);
                    break;
                case 0xE:
                    moby->state = 0x12;
                    if (moby->prev_seq != 0) {
                        MobyBlendAnimation(moby, 0, 0, scale_ticks(0x14));
                    }
                    {
                        float best = 0.0f;
                        int i;

                        for (i = 0; i < 3; i++) {
                            float w;

                            qcopy(v0, veldin2_arena_nodes + d->f280[i] * 0x80 + 0x30);
                            w = FastDiffRots(
                                fast_atan2(d->f3C0[0] - g_hero.motion.pos.f[0], d->f3C0[1] - g_hero.motion.pos.f[1]),
                                fast_atan2(d->f3C0[0] - v0[0], d->f3C0[1] - v0[1]));
                            if (best < w) {
                                d->f3A8 = d->f280[i];
                                best = w;
                            }
                        }
                    }
                    d->f358 = scale_ticks(900);
                    d->f354 = scale_ticks(0xB4);
                    break;
                case 0x13:
                    moby->state = 0x13;
                    if (moby->prev_seq != 1) {
                        MobyBlendAnimation(moby, 1, 0, scale_ticks(0x14));
                    }
                    if (vec_distance_xy(((f32 *)&moby->pos), g_hero.motion.pos.f) < 16.0f) {
                        d->f354 = scale_ticks(0x5A);
                    } else {
                        d->f354 = 0;
                    }
                    d->f358 = scale_ticks(600);
                    d->f35C = scale_ticks(600);
                    d->f338 = create_class_274_ring_effect(moby, d->f3C0, 5.8f);
                    break;
            }
            break;
        case 0xE:
            aim = &moby->rot.z;
            ang = fast_atan2(d->f320[0] - moby->pos.x, d->f320[1] - moby->pos.y);
            SteerAngleWithVelocity(aim, &d->f370, ang, g_frame_time_sq * 6.2831855f, g_frame_time_sq * 6.2831855f,
                                   g_frame_time * 6.2831855f);
            moby_point_position(moby, 0x13, v0);
            *(BossQ *)v1 = *(BossQ *)d->f320;
            set_drek_boss_relative_aim_yaw((char *)moby, d->f140, v0, v1);
            d->f3D8 = -3.0f;
            if ((moby->anim_flags & 2) != 0 && moby->prev_seq != 8) {
                MobyBlendAnimation(moby, 8, 0, scale_ticks(0x14));
            }
            if (d->f354 != 0) {
                if (d->f33C == 0) {
                    moby_point_position(moby, 1, v1);
                    d->f33C = (unsigned char *)spawn_drek_boss_electric_ball(moby, v1,
                                                                             scale_ticks(drek_boss_class_3d7_ticks));
                } else if (fast_dec_timer(&d->f354) != 0) {
                    if (moby->prev_seq != 9) {
                        MobyBlendAnimation(moby, 9, 0, 0);
                    }
                    d->f354 = scale_ticks(0x78);
                    moby_point_position(moby, 1, v1);
                    FastVecSub(v2, d->f320, v1);
                    v2[2] = 2.0f;
                    vec_set_length_xy(v2, v2, 180.0f);
                    FastVecAdd(v2, v2, v1);
                    refresh_moby_position_and_activate(d->f33C, v1, v2, 1, 1.0f);
                    d->f33C = 0;
                    if (d->f358 == 0) {
                        d->f350 = 0xE;
                        moby->state = 0xC;
                    }
                } else {
                    float f = 1.0f - IntToFloat(d->f354) / multiply_global_factor(90.0f);

                    if (f > 1.0f) {
                        f = 1.0f;
                    } else if (f < 0.0f) {
                        f = 0.0f;
                    }
                    moby_point_position(moby, 1, v1);
                    refresh_moby_position_and_activate(d->f33C, v1, d->f320, 0, f);
                    if (scale_ticks(0xF) < d->f354) {
                        qcopy(d->f320, ((f32 *)&tgt->pos));
                    }
                }
            }
            fast_dec_timer(&d->f358);
            break;
        case 0xF:
            aim = &moby->rot.z;
            ang = fast_atan2(tgt->pos.x - moby->pos.x, tgt->pos.y - moby->pos.y);
            SteerAngleWithVelocity(aim, &d->f370, ang, g_frame_time_sq * 6.2831855f, g_frame_time_sq * 6.2831855f,
                                   g_frame_time * 6.2831855f);
            update_drek_boss_arena_circling_movement((char *)moby);
            if (is_value_within_interpolated_window((char *)moby, 1.0f) != 0 ||
                is_value_within_interpolated_window((char *)moby, 16.0f) != 0) {
                if (random_integer_below(5) != 0 || d->fB4 == 2) {
                    float a;
                    float r;

                    if (d->fB4 != 2) {
                        qcopy(v1, d->f70);
                    } else {
                        qcopy(v1, g_hero.motion.pos.f);
                    }
                    a = random_angle_radians();
                    r = random_float_between(1.0f, 10.0f);
                    v0[0] = FastCos(a) * r;
                    v0[1] = FastSin(a) * r;
                    v0[2] = 0.0f;
                    FastVecAdd(v0, v0, v1);
                } else {
                    qcopy(v0, d->f70);
                }
                if (g_hero.state.control_mode == HERO_MODE_GRIND) {
                    qcopy(v1, veldin2_arena_nodes + d->f344 * 0x80 + 0x30);
                    FastVecSub(v0, v0, v1);
                    if (vector_length_xy(v0) > 29.0f) {
                        vec_set_length_xy(v0, v0, 29.0f);
                    }
                    FastVecAdd(v0, v0, v1);
                }
                v0[2] = probe_ground_height(v0, 0, 0.5f);
                if (absolute_float(v0[2] - d->f3C0[2]) < 5.0f) {
                    float a;
                    float c;
                    float e;

                    if (is_value_within_interpolated_window((char *)moby, 1.0f) != 0) {
                        moby_point_position(moby, 2, v1);
                    } else {
                        moby_point_position(moby, 3, v1);
                    }
                    vec_scale_to_length(v2, ((f32 *)&moby->rot_row0), g_frame_time * 20.0f);
                    a = randf_sym(0.08726646f, 0.5235988f);
                    c = -random_float_between(0.08726646f, 0.17453292f);
                    e = vec_distance_xy(((f32 *)&moby->pos), g_hero.motion.pos.f) * 3.0f;
                    if (e > 120.0f) {
                        e = 120.0f;
                    } else if (e < 60.0f) {
                        e = 60.0f;
                    }
                    create_rocket_moby(v1, v2, v0, a, c, truncate_float_to_s32(multiply_global_factor(e)), 2);
                    spawn_drek_boss_firing_effects((char *)moby, v1);
                }
            }
            if (fast_dec_timer(&d->f358) != 0 || (d->f390 > 25.0f && (float)d->f34C < 4.0f)) {
                d->f350 = 0xF;
                moby->state = 0xC;
            }
            break;
        case 0xD:
            aim = &moby->rot.z;
            ang = fast_atan2(tgt->pos.x - moby->pos.x, tgt->pos.y - moby->pos.y);
            SteerAngleWithVelocity(aim, &d->f370, ang, g_frame_time_sq * 6.2831855f, g_frame_time_sq * 6.2831855f,
                                   g_frame_time * 6.2831855f);
            update_drek_boss_arena_circling_movement((char *)moby);
            if (d->f354 < d->f358 + d->f35C) {
                d->f128 = fast_atan2(1.0f, d->f3BC);
            }
            if (moby->prev_seq == 2) {
                if ((moby->anim_flags & 2) != 0) {
                    MobyBlendAnimation(moby, 4, 0, scale_ticks(0x14));
                }
            } else if (fast_dec_timer(&d->f354) != 0 && d->f358 != 0 && moby->prev_seq != 4) {
                MobyBlendAnimation(moby, 4, 0, scale_ticks(0x14));
            }
            if (moby->prev_seq == 4) {
                if (is_value_within_interpolated_window((char *)moby, 1.0f) != 0) {
                    float f;

                    d->f354 = scale_ticks(0x5A);
                    d->f360 = scale_ticks(300);
                    moby_point_position(moby, 0, v0);
                    qcopy(v1, ((f32 *)&moby->rot_row0));
                    vec_scale_to_length(v2, ((f32 *)&moby->rot_row1), d->f3BC);
                    FastVecAdd(v1, v1, v2);
                    f = vec_distance_xy(v0, ((f32 *)&tgt->pos)) * 0.5f;
                    if (f > 30.0f) {
                        f = 30.0f;
                    } else if (f < 0.0f) {
                        f = 0.0f;
                    }
                    vec_scale_to_length(v1, v1, f * g_frame_time);
                    spawn_drek_boss_ring_projectile((char *)v0, v1, (int)moby, 64.0f, g_frame_time * 18.0f);
                    spawn_drek_boss_firing_effects((char *)moby, v0);
                    d->f3BC = randf_sym(0.133f, 0.333f);
                } else if ((moby->anim_flags & 2) != 0 && moby->prev_seq != 3) {
                    MobyBlendAnimation(moby, 3, 0, scale_ticks(0x14));
                }
            }
            if (fast_dec_timer(&d->f358) != 0 && fast_dec_timer(&d->f35C) != 0) {
                d->f350 = 0xD;
                moby->state = 0xC;
                if (moby->prev_seq != 5) {
                    MobyBlendAnimation(moby, 5, 0, scale_ticks(0x14));
                }
            }
            break;
        case 0x10:
            aim = &moby->rot.z;
            ang = fast_atan2(g_hero.motion.pos.f[0] - moby->pos.x, g_hero.motion.pos.f[1] - moby->pos.y);
            SteerAngleWithVelocity(aim, &d->f370, ang, g_frame_time_sq * 6.2831855f, g_frame_time_sq * 6.2831855f,
                                   g_frame_time * 6.2831855f);
            update_drek_boss_arena_circling_movement((char *)moby);
            if (fast_dec_timer(&d->f354) != 0) {
                float a;
                float r;

                qcopy(v0, veldin2_arena_nodes + d->f344 * 0x80 + 0x30);
                vec_scale_to_length(v1, ((f32 *)&moby->rot_row0), 3.0f);
                FastVecAdd(v1, v1, ((f32 *)&moby->pos));
                v1[2] = v1[2] + 3.0f;
                a = random_angle_radians();
                r = random_float_between(3.0f, 29.0f);
                v2[0] = FastCos(a) * r;
                v2[1] = FastSin(a) * r;
                v2[2] = 0.0f;
                FastVecAdd(v2, v2, v0);
                v2[2] = v2[2] + 5.0f;
                v2[2] = probe_ground_height(v2, 0, 0.5f);
                if (absolute_float(v2[2] - d->f3C0[2]) < 5.0f) {
                    float speed = vec_distance_xy(v2, v1) / (float)scale_ticks(0x3C);
                    float w = random_float_between(18.0f, 20.0f);

                    launch_mine_568_moby_from_list((int)moby, d->f29C, v1, v2, speed,
                                                   truncate_float_to_s32(multiply_global_factor(w * 60.0f)));
                }
                d->f354 = scale_ticks(0xF);
            }
            if (fast_dec_timer(&d->f358) != 0) {
                d->f350 = 0x10;
                moby->state = 0xC;
            }
            break;
        case 0x11: {
            int n;

            aim = &moby->rot.z;
            ang = fast_atan2(g_hero.motion.pos.f[0] - moby->pos.x, g_hero.motion.pos.f[1] - moby->pos.y);
            SteerAngleWithVelocity(aim, &d->f370, ang, g_frame_time_sq * 6.2831855f, g_frame_time_sq * 6.2831855f,
                                   g_frame_time * 6.2831855f);
            update_drek_boss_arena_circling_movement((char *)moby);
            if ((moby->anim_flags & 2) != 0 && moby->prev_seq != 12) {
                MobyBlendAnimation(moby, 0xC, 0, scale_ticks(0x14));
            }
            if ((moby->prev_seq == 12 && fast_dec_timer(&d->f354) != 0) || d->f354 == scale_ticks(0x1E)) {
                float f = d->f354 != 0 ? 4.3f : -4.3f;

                vec_scale_to_length(v0, ((f32 *)&moby->rot_row1), f);
                FastVecAdd(v0, v0, d->f3C0);
                v0[2] = probe_ground_height(v0, 0, 0.5f);
                if (absolute_float(v0[2] - d->f3C0[2]) < 1.0f) {
                    /* v1 is not written in this state; retail passes it all the same. */
                    float speed = vec_distance_xy(v0, v1) / (float)scale_ticks(0x3C);

                    if (launch_first_idle_group_gadgebot_1906_moby(d->f210, v0, v0, speed) != 0 && d->f354 == 0) {
                        d->f354 = scale_ticks(0x3C);
                    }
                }
            }
            n = count_group_gadgebot_1906_mobys_not_in_state_8(d->f210);
            if (n >= 11 && scale_ticks(0x3C) > d->f354) {
                d->f354 = scale_ticks(1000);
            } else if (n < 3 && scale_ticks(0x3C) < d->f354) {
                d->f358 = 0;
            }
            if (fast_dec_timer(&d->f358) != 0) {
                d->f350 = 0x11;
                moby->state = 0xC;
                if (moby->prev_seq != 13) {
                    MobyBlendAnimation(moby, 0xD, 0, scale_ticks(0x14));
                }
            }
            break;
        }
        case 0x13: {
            int fire;

            aim = &moby->rot.z;
            ang = fast_atan2(g_hero.motion.pos.f[0] - moby->pos.x, g_hero.motion.pos.f[1] - moby->pos.y);
            SteerAngleWithVelocity(aim, &d->f370, ang, g_frame_time_sq * 6.2831855f, g_frame_time_sq * 6.2831855f,
                                   g_frame_time * 6.2831855f);
            if (fast_dec_timer(&d->f354) != 0) {
                update_drek_boss_hero_follow_movement((char *)moby);
            }
            refresh_ring_effect(d->f338, d->f3C0, 5.8f);
            if (voice_is_playing(moby, d->f3B0) == 0) {
                d->f3B0 = Moby_PlaySound(0xE, 4, (int)moby);
            }
            fire = 0;
            if (fast_dec_timer(&d->f358) != 0 &&
                (g_hero.state.control_mode != HERO_MODE_GRIND || fast_dec_timer(&d->f35C) != 0)) {
                fire = 1;
            }
            if (g_hero.state.control_mode == HERO_MODE_GRIND) {
                int *list = (int *)g_hero.rail_spline;

                if (vec_distance_xy(g_hero.motion.pos.f, (char *)list + *list * 16) < 3.0f) {
                    fire = 1;
                }
            }
            if (g_hero.state.current == HERO_STATE_HURT &&
                vec_distance_xy(((f32 *)&moby->pos), g_hero.motion.pos.f) < 12.0f) {
                fire = 1;
            }
            if (vec_distance_xy(g_hero.motion.pos.f, veldin2_arena_nodes + d->f250[2] * 0x80 + 0x30) >
                l18_drek_boss_hero_follow_default_radius + 5.8f) {
                fire = 1;
            }
            if (fire) {
                int i = d->f3B0;

                if (i != -1) {
                    char *e = voice_pool + i * 0x70;
                    if (*(Moby **)(e + 0x88) == moby && *(unsigned char *)(e + 0x74) != 0) {
                        Sound_StopSlot(i);
                    }
                }
                d->f3B0 = -1;
                moby->state = 0xC;
                if (moby->prev_seq != 0) {
                    MobyBlendAnimation(moby, 0, 0, scale_ticks(0x14));
                }
                d->f350 = 0x13;
            }
            break;
        }
        case 0x12:
            aim = &moby->rot.z;
            ang = fast_atan2(d->f70[0] - moby->pos.x, d->f70[1] - moby->pos.y);
            SteerAngleWithVelocity(aim, &d->f370, ang, g_frame_time_sq * 6.2831855f, g_frame_time_sq * 6.2831855f,
                                   g_frame_time * 6.2831855f);
            {
                /* Retail scales the index before it sets up the call. */
                int off = d->f3A8 * 0x80;

                if (advance_drek_boss_planar_position_toward_target((char *)moby, g_frame_time * 12.0f,
                                                                    veldin2_arena_nodes + off + 0x30) != 0) {
                    moby->state = 0xE;
                    if (moby->prev_seq != 7) {
                        MobyBlendAnimation(moby, 7, 0, scale_ticks(0x14));
                    }
                    qcopy(d->f320, ((f32 *)&tgt->pos));
                }
            }
            break;
        case 0x15:
            aim = &moby->rot.z;
            ang = fast_atan2(d->f330->pos.x - moby->pos.x, d->f330->pos.y - moby->pos.y);
            SteerAngleWithVelocity(aim, &d->f370, ang, g_frame_time_sq * 6.2831855f, g_frame_time_sq * 6.2831855f,
                                   g_frame_time * 6.2831855f);
            moby_init_action_mode(d->f330);
            if (advance_drek_boss_planar_position_toward_target((char *)moby, g_frame_time * 12.0f,
                                                                ((f32 *)&d->f330->pos)) != 0) {
                int ticks;

                switch (d->f34C) {
                    case 0:
                        ticks = scale_ticks(0xE10);
                        break;
                    case 1:
                        ticks = scale_ticks(0xA8C);
                        break;
                    case 2:
                        ticks = scale_ticks(0x708);
                        break;
                    case 3:
                        ticks = scale_ticks(0x4B0);
                        break;
                    case 4:
                        ticks = scale_ticks(900);
                        break;
                    case 5:
                    default:
                        ticks = scale_ticks(600);
                        break;
                }
                request_thruster_pack_lock_red_wait(veldin2_moby_table + d->f21C * 0x100, scale_ticks(ticks));
                d->f354 = 0;
                if (d->f34C == 4) {
                    moby->state = 0x14;
                } else {
                    moby->state = 0xC;
                }
                d->f364 = 1;
                d->f34C = d->f34C + 1;
            }
            if (*(int *)&drek_boss_camera_move_request != 0) {
                moby->state = 5;
            }
            break;
        case 0x1A: {
            int n;

            aim = &moby->rot.z;
            ang = fast_atan2(g_hero.motion.pos.f[0] - moby->pos.x, g_hero.motion.pos.f[1] - moby->pos.y);
            SteerAngleWithVelocity(aim, &d->f370, ang, g_frame_time_sq * 6.2831855f, g_frame_time_sq * 6.2831855f,
                                   g_frame_time * 6.2831855f);
            update_drek_boss_arena_circling_movement((char *)moby);
            d->f358 = d->f358 + 1;
            n = scale_ticks(0x14);
            if (d->f358 % n == 0) {
                float a = FastAddRots(moby->rot.z, random_float_between(-30.0f, 30.0f) * 0.017453292f);

                v0[0] = FastCos(a) * 6.0f;
                v0[1] = FastSin(a) * 6.0f;
                v0[2] = 0.0f;
                v0[2] = random_float_between(3.0f, 8.0f);
                FastVecAdd(v0, v0, d->f3C0);
                spawn_explosion(moby, veldin2_zero_vec, v0, 0.0f, 0.0f, 0x14, 6, 0x20, 1.0f, 2.5f, 9.0f, 1.0f, -1, 0.0f,
                                0, 0, -1, 0);
            }
            if (d->f358 == scale_ticks(0x78)) {
                if (moby->prev_seq != 14) {
                    MobyBlendAnimation(moby, 0xE, 5, 1);
                }
                v0[0] = FastCos(moby->rot.z) * 3.0f;
                v0[1] = FastSin(moby->rot.z) * 3.0f;
                v0[2] = 6.0f;
                FastVecAdd(v0, v0, d->f3C0);
                spawn_explosion(moby, veldin2_zero_vec, v0, 0.0f, 0.0f, 0x28, 0xA, 0x20, 2.5f, 5.0f, 9.0f, 2.0f, -1,
                                10.0f, 0, 0, -1, 0);
            }
            if (moby->seq == moby->prev_seq && moby->seq == 14 &&
                is_value_within_interpolated_window((char *)moby, 30.0f) != 0) {
                moby->state = 0x1B;
                end_thruster_pack_lock_red_wait(veldin2_moby_table + d->f21C * 0x100);
            }
            break;
        }
        case 0x1B:
            moby->collision = 0;
            moby->flags |= 0x41;
            moby->flags &= 0xEFFF;
            if (d->f34C < 7) {
                start_scene(3);
                d->f34C = 7;
                ((BossShared46 *)&veldin2_cutscene_state)->f46 = 1;
                {
                    /* In locals: retail does not load them again after the first copy. */
                    char *recs = veldin2_arena_nodes;
                    int idx = d->f294;

                    qcopy(((BossShared *)&veldin2_cutscene_state)->f10, recs + idx * 0x80 + 0x30);
                    qcopy(((BossShared *)&veldin2_cutscene_state)->f20, recs + idx * 0x80 + 0x70);
                }
                moby->state = 7;
                d->f39C = 0x1B;
            } else if (d->f34C == 7) {
                if (g_hero.ground_moby != 0 && g_hero.air_frames.s == 0 &&
                    g_hero.ground_moby->oclass == MOBY_CLASS_THRUSTER_PACK_LOCK_GREEN &&
                    g_hero.state.current == HERO_STATE_RISE_THEN_FALL_WITH_AIRBORNE_HIT_QUERY) {
                    if (d->f3AC == 0) {
                        d->f3AC = 1;
                    }
                } else if (d->f3AC == 0) {
                    set_thruster_pack_lock_green_list_data_fields(d->f200);
                }
                if (d->f3AC != 0) {
                    int n = d->f3AC++;

                    if (scale_ticks(0x28) < n) {
                        start_scene(4);
                        d->f34C = 8;
                        moby->state = 7;
                        d->f39C = 0x1B;
                    }
                }
            } else if (d->f34C == 8) {
                *(BossU128 *)v0 = 0;
                *(BossU128 *)v1 = 0;
                v0[0] = 660.6f;
                v0[1] = 481.4f;
                v0[2] = 112.6f;
                v1[1] = -0.12f;
                v1[2] = -2.76f;
                jet_mount(v0, v1, 1, 0, 0);
                start_scene(5);
                d->f34C = 9;
            } else if (d->f34C == 9 && veldin2_cinematic_mode == 0) {
                start_slideshow();
                d->f34C = 10;
            } else {
                int k = d->f34C;

                if (k == 10 && veldin2_cinematic_mode == 0) {
                    queue_movie_with_game_language(0xB);
                    d->f34C = 0xB;
                } else if (k == 0xB && veldin2_cinematic_mode == 0) {
                    drek_boss_intro_started = 0;
                    drek_boss_camera_reset_done = 0;
                    PauseAllSounds(0x21);
                    DeleteMoby(d->f3A0);
                    if (d->f3A4 != 0) {
                        DeleteMoby(d->f3A4);
                    }
                    DeleteMoby(moby);
                    return;
                }
            }
            break;
    }
    update_drek_boss_state_dependent_scalars((char *)moby);
    if (moby->visible != 0) {
        moby_point_position(moby, 8, v0);
        moby_point_position(moby, 9, v1);
        FastVecSub(v2, v0, v1);
        vec_scale_to_length(v2, v2, 0.1f);
        draw_trail(v0, v0, v2, 600000.0f, 0.0f);
    }
    if (d->f34C > 1) {
        g_hero.grind_look_moby = moby;
    }
    qcopy(((f32 *)&moby->pos), d->f3C0);
    slow = 0.3f;
    fast = 0.03f;
    d->f3D4 = FastAddRots(d->f3D4, g_frame_time * 3.1415927f);
    ApproachWithVelocity_v(d->f3D0 * FastSin(d->f3D4) + d->f3D8, g_frame_time_sq * 4.0f, g_frame_time_sq * 4.0f,
                           g_frame_time * 4.0f, &d->f3DC, &d->f3E0);
    moby->pos.z = moby->pos.z + d->f3DC;
    d->f3D8 = 0.0f;
    update_joint_animation((char *)moby, d->fC0, 7, g_frame_scale_sq * fast, g_frame_scale_sq * slow);
    update_joint_animation((char *)moby, d->f140, 0x13, g_frame_scale_sq * fast, g_frame_scale_sq * slow);
    d->f3B4 = FastAddRots(d->f3B4, g_frame_time * 3.4906585f);
    {
        int c = FastTweenColor(0x30C8C8C8, 0x1C393939, FastSin(d->f3B4) * 0.5f + 0.5f);
        int r = c & 0xFF;
        int g = (c >> 8) & 0xFF;
        int b = (c >> 16) & 0xFF;
        int a = (unsigned int)c >> 24;

        moby->glow_color = c;
        d->f3B8 = (a << 24) | ((b / 3) << 16) | (g << 8) | (r / 3);
    }
    if (moby->visible != 0) {
        AddDrawCallback((void (*)(void))render_drek_boss_joint_glows, moby);
    }
}

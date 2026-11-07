void BlargSpaceCommandoUpdate(struct Moby *moby) {
    Vars *vars = (Vars *)moby->pvars;
    struct Moby *target;
    Vec4 a;
    Vec4 b;
    Vec4 c;
    Shot shot;
    s32 near;
    s32 close;
    s32 hit;

    update_blarg_space_commando_damage_targeting_and_ground_motion(moby);
    target = vars->target;
    Manipulator_ApplyScale(2.5f, moby, 1, vars->unk400);
    if (moby->visible != 0 && fast_vec_dist(&moby->pos, gemlik_camera_pos) < 28.0f) {
        set_ground_height_band(moby);
        moby->shadow_range = 0x16;
    }
    switch (moby->state) {
        case 0:
            vars->unk20 = 2.0f;
            vars->unk28 = 1;
            vars->unk58 = 12;
            vars->unk5A = 8;
            vars->unk24 = 2;
            vars->unk30 = 1.5f;
            MoveCtl_Init(&vars->ctl);
            vars->ctl.unk8 = 2.0f;
            vars->ctl.unkC = 0.5f;
            vars->ctl.unk0 = truncate_float_to_s32_r_r(D_L13_00161EE8_309330 * 716.8f);
            vars->ctl.unk4 = D_L13_00161EE8_309330 * 0.7f;
            vars->ctl.unk24 = D_L13_00161F08_309330 * g_frame_time * random_float_between(0.95f, 1.05f);
            qcopy(&vars->home, &moby->pos);
            moby_point_position(moby, 0, &vars->unk1E0);
            moby->state = 1;
            break;
        case 1:
            SteerAngleWithVelocity_v(&moby->rot.z, fast_atan2(target->pos.x - moby->pos.x, target->pos.y - moby->pos.y),
                                     &vars->turn, g_frame_time_sq * 6.2831855f, g_frame_time_sq * 6.2831855f,
                                     g_frame_time * 3.1415927f);
            if (vars->unk164 != 2) {
                f32 dist = vec_distance_xy(&moby->pos, &target->pos);
                if (D_L13_00161F04_309330 < dist) {
                    if (FastDiffRots(moby->rot.z, fast_atan2(target->pos.x - moby->pos.x,
                                                             target->pos.y - moby->pos.y)) < 0.2617994f) {
                        moby->state = 2;
                        if (moby->prev_seq != 3) {
                            MobyBlendAnimation(moby, 3, 0, scale_ticks(10));
                        }
                    } else {
                        moby->state = 7;
                        if (moby->prev_seq != 1) {
                            MobyBlendAnimation(moby, 1, 0, scale_ticks(20));
                        }
                    }
                } else {
                    moby->state = 5;
                    if (moby->prev_seq != 6) {
                        MobyBlendAnimation(moby, 6, 0, scale_ticks(10));
                    }
                }
            } else if (vars->unk184 != 0) {
                moby->state = 11;
                if (moby->prev_seq != 1) {
                    MobyBlendAnimation(moby, 1, 0, scale_ticks(20));
                }
            }
            break;
        case 2:
            if (moby->anim_flags & 2) {
                moby->state = 3;
                initialize_gemlik_trail((char *)moby);
                if (moby->prev_seq != 5) {
                    MobyBlendAnimation(moby, 5, 0, scale_ticks(10));
                }
            }
            break;
        case 3:
            if (moby->anim_flags & 2) {
                f32 dist = vec_distance_xy(&moby->pos, &target->pos);
                if (vars->unk164 != 2 && dist < 15.0f) {
                    moby->state = 4;
                    if (moby->prev_seq != 4) {
                        MobyBlendAnimation(moby, 4, 0, scale_ticks(20));
                    }
                    vars->timer = scale_ticks(D_L13_00161F00_309330);
                } else {
                    moby->state = 1;
                    if (moby->prev_seq != 0) {
                        MobyBlendAnimation(moby, 0, 0, scale_ticks(20));
                    }
                }
            }
            break;
        case 4:
            SteerAngleWithVelocity_v(&moby->rot.z, fast_atan2(target->pos.x - moby->pos.x, target->pos.y - moby->pos.y),
                                     &vars->turn, g_frame_time_sq * 6.2831855f, g_frame_time_sq * 6.2831855f,
                                     g_frame_time * 3.1415927f);
            if (fast_dec_timer(&vars->timer) != 0) {
                moby->state = 3;
                initialize_gemlik_trail((char *)moby);
                if (moby->prev_seq != 5) {
                    MobyBlendAnimation(moby, 5, 0, scale_ticks(5));
                }
            } else if (vec_distance_xy(&moby->pos, &target->pos) < D_L13_00161F04_309330) {
                moby_point_position(moby, 0, &vars->unk1E0);
                moby->state = 5;
                if (moby->prev_seq != 6) {
                    MobyBlendAnimation(moby, 6, 0, scale_ticks(10));
                }
            } else if (vars->unk184 != 0) {
                moby->state = 11;
                if (moby->prev_seq != 1) {
                    MobyBlendAnimation(moby, 1, 0, scale_ticks(20));
                }
            }
            break;
        case 5:
            SteerAngleWithVelocity_v(&moby->rot.z, fast_atan2(target->pos.x - moby->pos.x, target->pos.y - moby->pos.y),
                                     &vars->turn, g_frame_time_sq * 6.2831855f, g_frame_time_sq * 6.2831855f,
                                     g_frame_time * 3.1415927f);
            moby_point_position(moby, 0, &a);
            qcopy(&b, &moby->pos);
            b.f[2] += 0.3f;
            {
                f32 frame = compute_interpolated_record_value(moby);
                if (frame >= 9.0f && frame <= 15.0f) {
                    c.f[0] = FastCos(moby->rot.z);
                    c.f[1] = FastSin(moby->rot.z);
                    c.f[2] = 1.0f;
                    c.f[3] = 5627.925f;
                    InitDirectionalHitQuery_r(&shot, moby, 1, &c, 1.0f);
                    shot.unk18 = 0;
                    shot.unk19 = 1;
                    shot.oclass = moby->oclass;
                    sweep_hit_segment(&a, &b, &vars->unk1E0, &b, moby, &shot, 5);
                }
            }
            qcopy(&vars->unk1E0, &a);
            if (moby->anim_flags & 2) {
                moby->state = 6;
                moby->command = 1;
                if (moby->prev_seq != 2) {
                    MobyBlendAnimation(moby, 2, 0, scale_ticks(10));
                }
            }
            break;
        case 6:
            if (moby->anim_flags & 2) {
                moby->state = moby->command;
                if (moby->prev_seq != 0) {
                    MobyBlendAnimation(moby, 0, 0, scale_ticks(20));
                }
            }
            break;
        case 7:
            MobySpringTurn_r(
                moby, FastAddRots(fast_atan2(target->pos.x - moby->pos.x, target->pos.y - moby->pos.y), vars->unk1A8),
                &vars->turn, 0.05f, 0.3f, g_frame_time * 6.2831855f);
            a.f[0] = 2.0f * FastCos(moby->rot.z);
            a.f[1] = 2.0f * FastSin(moby->rot.z);
            a.f[2] = 0.0f;
            if (MobySteerToward(moby, &vars->ctl, &a, &b, 1.0f) & 2) {
                func_001160D8();
                vars->unk1A8 = IntToFloat(-1) * 1.0995574f;
            } else {
                vars->unk1A8 = 0.0f;
            }
            resolve_point_path_segment_xy_clearance_v(vars->path, &moby->pos, &moby->pos, 0.75f);
            close = 0;
            if (path_8808(vars->path, &moby->pos, &target->pos, &c) != 0 && fast_vec_dist(&moby->pos, &c) < 0.8f) {
                close = 1;
            }
            {
                f32 dist = vec_distance_xy(&moby->pos, &target->pos);
                if (dist < 15.0f) {
                    moby->state = 2;
                    if (moby->prev_seq != 3) {
                        MobyBlendAnimation(moby, 3, 0, scale_ticks(10));
                    }
                } else if (close != 0) {
                    moby->state = 2;
                    if (moby->prev_seq != 3) {
                        MobyBlendAnimation(moby, 3, 0, scale_ticks(10));
                    }
                } else if (dist < D_L13_00161F04_309330) {
                    moby->state = 5;
                    if (moby->prev_seq != 6) {
                        MobyBlendAnimation(moby, 6, 0, scale_ticks(10));
                    }
                }
            }
            break;
        case 11:
            near = 0;
            MobySpringTurn_r(moby, fast_atan2(target->pos.x - moby->pos.x, target->pos.y - moby->pos.y), &vars->turn,
                             0.05f, 0.3f, g_frame_time * 6.2831855f);
            a.f[0] = 2.0f * FastCos(moby->rot.z);
            a.f[1] = 2.0f * FastSin(moby->rot.z);
            a.f[2] = 0.0f;
            MobySteerToward(moby, &vars->ctl, &a, &b, 1.0f);
            resolve_point_path_segment_xy_clearance_v(vars->path, &moby->pos, &moby->pos, 0.75f);
            if (path_8808(vars->path, &moby->pos, &target->pos, &c) != 0 && fast_vec_dist(&moby->pos, &c) < 0.8f) {
                near = 1;
            }
            if (vars->unk184 == 0) {
                moby->state = 13;
            } else if (near != 0) {
                moby->state = 12;
                if (moby->prev_seq != 6) {
                    MobyBlendAnimation(moby, 6, 0, scale_ticks(10));
                }
            } else if (vec_distance_xy(&moby->pos, &target->pos) < D_L13_00161F04_309330) {
                moby_point_position(moby, 0, &vars->unk1E0);
                moby->state = 12;
                if (moby->prev_seq != 6) {
                    MobyBlendAnimation(moby, 6, 0, scale_ticks(10));
                }
            }
            break;
        case 12:
            moby_point_position(moby, 0, &a);
            qcopy(&b, &moby->pos);
            b.f[2] += 1.5f;
            {
                f32 frame = compute_interpolated_record_value(moby);
                if (frame >= 9.0f && frame <= 15.0f) {
                    c.f[0] = FastCos(moby->rot.z);
                    c.f[1] = FastSin(moby->rot.z);
                    c.f[2] = 1.0f;
                    c.f[3] = 5627.925f;
                    InitDirectionalHitQuery_r(&shot, moby, 1, &c, 1.0f);
                    shot.unk18 = 0;
                    shot.unk19 = 1;
                    shot.oclass = moby->oclass;
                    sweep_hit_segment(&a, &b, &vars->unk1E0, &b, moby, &shot, 5);
                }
            }
            qcopy(&vars->unk1E0, &a);
            if (moby->anim_flags & 2) {
                moby->state = 11;
                if (moby->prev_seq != 1) {
                    MobyBlendAnimation(moby, 1, 0, scale_ticks(20));
                }
            }
            break;
        case 13:
            MobySpringTurn_r(moby, fast_atan2(vars->home.f[0] - moby->pos.x, vars->home.f[1] - moby->pos.y),
                             &vars->turn, 0.05f, 0.3f, g_frame_time * 6.2831855f);
            a.f[0] = 2.0f * FastCos(moby->rot.z);
            a.f[1] = 2.0f * FastSin(moby->rot.z);
            a.f[2] = 0.0f;
            if (FastDiffRots(moby->rot.z, fast_atan2(vars->home.f[0] - moby->pos.x, vars->home.f[1] - moby->pos.y)) <
                0.7853982f) {
                MobySteerToward(moby, &vars->ctl, &a, &b, 1.0f);
                resolve_point_path_segment_xy_clearance_v(vars->path, &moby->pos, &moby->pos, 0.75f);
            }
            if (fast_vec_dist(&vars->home, &moby->pos) < 1.0f) {
                moby->state = 1;
                if (moby->prev_seq != 0) {
                    MobyBlendAnimation(moby, 0, 0, scale_ticks(20));
                }
            }
            break;
        case 15:
            hit = step_moby_motion(moby, &vars->vel);
            if (hit & 0x40) {
                qzero(&vars->vel);
                moby->state = 1;
                if (moby->prev_seq != 0) {
                    MobyBlendAnimation(moby, 0, 0, scale_ticks(10));
                }
            } else if (hit & 0x120) {
                DeleteMoby(moby);
            }
            break;
        case 16:
            if (step_moby_motion(moby, &vars->vel) & 0x160) {
                spawn_explosion_v(moby, &vars->vel, 0, 0.0f, 0.0f, 10, 3, 16, 4.0f, 2.0f, 9.0f, -1, 1.0f, 15.0f, 1, 1,
                                  -1, 0);
                DeleteMoby(moby);
            }
            break;
    }
}

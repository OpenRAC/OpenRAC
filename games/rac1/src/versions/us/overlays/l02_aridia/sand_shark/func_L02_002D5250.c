void sand_shark_update(unsigned char *moby) {
    float v0[4];
    float v1[4];
    float v2[4];
    float v3[4];
    float v4[4];
    Vars *d;
    char *m;
    float a;
    float g;
    int hit;

    if (sand_shark_updates_disabled != 0) {
        return;
    }
    *(volatile int *)0x10000010 = 0x81;
    *(volatile int *)0x10000000 = 0;
    d = ((Vars *)((Moby *)moby)->pvars);
    Manipulator_ApplyScale(2.1f, (int)moby, 0, d->blink);
    if (((Moby *)moby)->visible != 0 && fast_vec_dist(moby + 0x10, aridia_camera_pos) < 28.0f) {
        set_ground_height_band(moby);
        ((Moby *)moby)->shadow_range = 0x16;
    }
    handle_sand_shark_damage_and_targeting(moby);
    *(volatile int *)0x10000000;
    switch (((Moby *)moby)->state) {
        case 0x0:
            if (d->i290 == -1) {
                STUB_printf(sand_shark_no_path_fmt, *(short *)&((Moby *)moby)->save_id);
                DeleteMoby(moby);
                return;
            }
            if (*(int *)aridia_paths[d->i290] == 0) {
                STUB_printf(sand_shark_empty_path_fmt, *(short *)&((Moby *)moby)->save_id);
                DeleteMoby(moby);
                return;
            }
            if (d->i288 >= 0) {
                *(int *)(*(char **)((d->i288 << 8) + aridia_mobys_m + 0x78) + 0x14C) += 1;
            }
            qcopy(d->home, moby + 0x10);
            qcopy(d->start, moby + 0x10);
            d->s24 = 1;
            d->f20 = 1.0f;
            d->b58 = sand_shark_vars_b58_init;
            d->b5C = 1;
            d->b2E = 1;
            d->b29 = 1;
            d->t25E = 0;
            MoveCtl_Init((char *)&d->i180);
            d->f188 = d->f18C = 2.0f;
            d->i180 = 0x200;
            d->f184 = 0.5f;
            d->f1A4 = d->f254 * g_frame_time;
            if (func_001160D8() & 1) {
                ((Moby *)moby)->flags |= 0x8000;
            }
            d->pD0 = &sand_shark_spawn_d0_record;
            if (d->i26C >= 0) {
                if (d->i28C == -1) {
                    DeleteMoby(moby);
                    return;
                }
                ((Moby *)moby)->pos.z += 10.0f;
                ((Moby *)moby)->state = 0xC;
                *(int *)&((Moby *)moby)->collision = 0;
                ((Moby *)moby)->flags |= 1;
                ((Moby *)moby)->flags &= 0xEFFF;
                if (((Moby *)moby)->prev_seq != 0) {
                    MobyBlendAnimation(moby, 0, 0, scale_ticks(3));
                }
            } else if (d->i278 >= 0) {
                d->f18C = 0.2f;
                ((Moby *)moby)->state = 3;
                if (((Moby *)moby)->prev_seq != 2) {
                    MobyBlendAnimation(moby, 2, 0, scale_ticks(3));
                }
            } else if (d->i258 >= 0) {
                ((Moby *)moby)->state = 0x1D;
                ((Moby *)moby)->flags |= 0x41;
                *(int *)&((Moby *)moby)->collision = 0;
                ((Moby *)moby)->flags &= 0xEFFF;
            } else {
                ((Moby *)moby)->state = 1;
                if (((Moby *)moby)->prev_seq != 2) {
                    MobyBlendAnimation(moby, 2, 0, scale_ticks(3));
                }
            }
            break;
        case 0x1D: {
            unsigned char *w = (unsigned char *)(aridia_mobys_m + (d->i258 << 8));
            if (w[0x20] < 5) {
                break;
            }
            if (g_hero.state.current == 0x72) {
                break;
            }
            ((Moby *)moby)->flags &= 0xFFBE;
            ((Moby *)moby)->flags |= 0x1000;
            *(int *)&((Moby *)moby)->collision = *(int *)(*(char **)(moby + 0x24) + 0x10);
            d->i258 = -1;
            d->t25C = scale_ticks(0x1518);
            ((Moby *)moby)->state = 0;
            ((Moby *)moby)->pos.z -= sand_shark_sink_depth;
            break;
        }
        case 0x1:
            FastVecSub(v0, &g_hero.motion.pos, moby + 0x10);
            ((Moby *)moby)->state = 0x12;
            if (((Moby *)moby)->prev_seq != 0) {
                MobyBlendAnimation(moby, 0, 0, scale_ticks(3));
            }
            break;
        case 0x2:
            ((Moby *)moby)->pos.z += sand_shark_sink_depth;
            qcopy(v0, d->home);
            a = fast_atan2(v0[0] - ((Moby *)moby)->pos.x, v0[1] - ((Moby *)moby)->pos.y);
            a = FastAddRots(a, d->f244);
            MobySpringTurn_v((char *)moby, a, &d->f240, 0.05f, 0.3f, 0.04f);
            ((Moby *)moby)->pos.x += FastCos(((Moby *)moby)->rot.z) * g_frame_time;
            ((Moby *)moby)->pos.y += FastSin(((Moby *)moby)->rot.z) * g_frame_time;
            if (fast_dec_timer(&d->i260)) {
                d->i260 = scale_ticks(0x3C);
                a = d->f248 * 0.017453292f;
                d->f244 = random_float_between(-a, a);
            }
            if (((Moby *)moby)->visible != 0 && (random_integer_below(0xFF) & 1)) {
                *(u128b *)v1 = *(u128b *)(moby + 0x10);
                emit_sand_shark_single_particle(v1);
            }
            ((Moby *)moby)->pos.z -= sand_shark_sink_depth;
            if (d->i214 == 2) {
                if (d->t25E == 0) {
                    break;
                }
                if (random_integer_below(0x13) != 0) {
                    break;
                }
                ((Moby *)moby)->state = 0x15;
                Moby_PlaySound(4, 0, (int)moby);
                if (((Moby *)moby)->prev_seq != 0) {
                    MobyBlendAnimation(moby, 0, 0, 0);
                }
                d->i274 = -1;
                break;
            }
            if (d->i274 >= 0) {
                if (is_point_inside_clip_volume(d->tgt, d->i274) == 0 || d->t25E != 0) {
                    if (random_integer_below(9) == 0) {
                        d->i274 = -1;
                    }
                }
                break;
            }
            if (random_integer_below(4) != 0) {
                break;
            }
            d->f244 = d->f248 * 0.017453292f;
            if (d->i284 != 0) {
                d->f244 = d->f244 * (float)d->i284;
            } else if (random_integer_below(0x100) & 1) {
                d->f244 = -d->f244;
            }
            d->i260 = scale_ticks((int)random_float_between(30.0f, 90.0f));
            d->f1A4 = g_frame_time * 4.0f;
            ((Moby *)moby)->state = 0x11;
            if (((Moby *)moby)->prev_seq != 0xC) {
                MobyBlendAnimation(moby, 0xC, 0, scale_ticks(3));
            }
            break;
        case 0x3:
            if (d->i214 == 2) {
                break;
            }
            if (is_point_inside_clip_volume(d->tgt, d->i278) == 0) {
                break;
            }
            ((Moby *)moby)->state = 5;
            if (((Moby *)moby)->prev_seq != 0) {
                MobyBlendAnimation(moby, 0, 0, scale_ticks(3));
            }
            break;
        case 0xE:
            qcopy(v1, d->tgt);
            hit = 0;
            a = fast_atan2(v1[0] - ((Moby *)moby)->pos.x, v1[1] - ((Moby *)moby)->pos.y);
            MobySpringTurn_v((char *)moby, a + d->f244, &d->f240, 0.05f, 0.3f, 0.2f);
            v0[0] = FastCos(((Moby *)moby)->rot.z) * 2.0f;
            v0[1] = FastSin(((Moby *)moby)->rot.z) * 2.0f;
            v0[2] = 0.0f;
            FastVecAdd(v0, v0, moby + 0x10);
            MobySteerToward(moby, &d->i180, v0, v2, 1.0f);
            begin_sand_shark_return_home_if_outside_path(moby);
            if (path_8808(d->i290, moby + 0x10, v1, v3) && fast_vec_dist(moby + 0x10, v3) < d->f294) {
                hit = 1;
            }
            if (d->t25E == 0) {
                ((Moby *)moby)->state = 0x12;
                break;
            }
            if (hit) {
                ((Moby *)moby)->state = 0xF;
                if (((Moby *)moby)->prev_seq != 5) {
                    MobyBlendAnimation(moby, 5, 0, scale_ticks(0xA));
                }
                break;
            }
            if (vec_distance_xy(moby + 0x10, v1) < 1.5f) {
                ((Moby *)moby)->state = 6;
                if (((Moby *)moby)->prev_seq != 1) {
                    MobyBlendAnimation(moby, 1, 0, scale_ticks(0xA));
                }
                break;
            }
            if (d->i214 == 2) {
                break;
            }
            ((Moby *)moby)->state = 5;
            if (((Moby *)moby)->prev_seq != 0) {
                MobyBlendAnimation(moby, 0, 0, scale_ticks(0xF));
            }
            break;
        case 0xF: {
            float *ang = (float *)(moby + 0x48);
            a = fast_atan2(d->tgt[0] - ((Moby *)moby)->pos.x, d->tgt[1] - ((Moby *)moby)->pos.y);
            SteerAngleWithVelocity(ang, &d->f240, a, g_frame_time_sq * 6.2831855f, g_frame_time_sq * 6.2831855f,
                                   g_frame_time * 6.2831855f);
            if (d->t25E == 0) {
                ((Moby *)moby)->state = 0x12;
                break;
            }
            if (vec_distance_xy(moby + 0x10, d->tgt) < 1.5f) {
                ((Moby *)moby)->state = 6;
                if (((Moby *)moby)->prev_seq != 1) {
                    MobyBlendAnimation(moby, 1, 0, scale_ticks(0xA));
                }
                break;
            }
            if (d->i214 == 2) {
                break;
            }
            ((Moby *)moby)->state = 5;
            if (((Moby *)moby)->prev_seq != 0) {
                MobyBlendAnimation(moby, 0, 0, scale_ticks(0xF));
            }
            break;
        }
        case 0x11:
        case 0x12:
            ((Moby *)moby)->pos.z += sand_shark_sink_depth;
            g = probe_ground_height(moby + 0x10, 0, 0.5f);
            if (g < d->home[2]) {
                g = d->home[2];
            }
            qcopy(v0, moby + 0x10);
            v0[2] = g;
            ApproachValue((float *)(moby + 0x18), g, g_frame_time * 4.0f);
            if (g == ((Moby *)moby)->pos.z) {
                if (((Moby *)moby)->prev_seq != 0xC) {
                    MobyBlendAnimation(moby, 0xC, 0, scale_ticks(5));
                }
                if (((Moby *)moby)->state == 0x11) {
                    ((Moby *)moby)->state = 0x16;
                } else {
                    ((Moby *)moby)->state = 0x17;
                }
            }
            ((Moby *)moby)->pos.z -= sand_shark_sink_depth;
            if (scale_ticks(0xA) < frame_counter_ref) {
                emit_sand_shark_radial_particles(v0, 6);
            }
            break;
        case 0x13:
        case 0x14:
        case 0x15: {
            float z;
            ((Moby *)moby)->pos.z += sand_shark_sink_depth;
            z = probe_ground_height(moby + 0x10, 0, 0.5f);
            qcopy(v0, moby + 0x10);
            v0[2] = z;
            if (ApproachValue((float *)(moby + 0x18), z + sand_shark_sink_depth, g_frame_time * 4.0f) == 0.0f) {
                if (((Moby *)moby)->state == 0x13) {
                    ((Moby *)moby)->state = 5;
                    if (((Moby *)moby)->prev_seq != 0) {
                        MobyBlendAnimation(moby, 0, 0, scale_ticks(3));
                    }
                } else if (((Moby *)moby)->state == 0x14) {
                    ((Moby *)moby)->state = 7;
                    if (((Moby *)moby)->prev_seq != 0) {
                        MobyBlendAnimation(moby, 0, 0, scale_ticks(3));
                    }
                } else {
                    d->f294 = random_float_between(2.0f, 3.5f);
                    d->f1A4 = g_frame_time * 4.0f;
                    ((Moby *)moby)->state = 0xE;
                    if (((Moby *)moby)->prev_seq != 0) {
                        MobyBlendAnimation(moby, 0, 0, scale_ticks(3));
                    }
                }
            }
            ((Moby *)moby)->pos.z -= sand_shark_sink_depth;
            emit_sand_shark_radial_particles(v0, 6);
            break;
        }
        case 0x5:
        case 0x16:
            if (((Moby *)moby)->state == 0x16) {
                ((Moby *)moby)->pos.z += sand_shark_sink_depth;
                if (random_integer_below(0xFF) & 1) {
                    *(u128b *)v4 = *(u128b *)(moby + 0x10);
                    emit_sand_shark_single_particle(v4);
                }
            }
            qcopy(v3, d->tgt);
            a = fast_atan2(v3[0] - ((Moby *)moby)->pos.x, v3[1] - ((Moby *)moby)->pos.y);
            MobySpringTurn_v((char *)moby, a + d->f244, &d->f240, 0.05f, 0.3f, 0.2f);
            v0[0] = FastCos(((Moby *)moby)->rot.z) * 2.0f;
            v0[1] = FastSin(((Moby *)moby)->rot.z) * 2.0f;
            v0[2] = 0.0f;
            FastVecAdd(v0, v0, moby + 0x10);
            MobySteerToward(moby, &d->i180, v0, v1, 1.0f);
            begin_sand_shark_return_home_if_outside_path(moby);
            if (((Moby *)moby)->state == 0x16) {
                ((Moby *)moby)->pos.z -= sand_shark_sink_depth;
            }
            if (d->i214 != 2) {
                if (d->i278 >= 0 && is_point_inside_clip_volume(d->tgt, d->i278) == 0) {
                    ((Moby *)moby)->state = 7;
                    break;
                }
                FastVecSub(v2, d->tgt, d->home);
                if (vec_distance_xy(moby + 0x10, v3) < 12.0f && ((Moby *)moby)->state == 0x16) {
                    ((Moby *)moby)->state = 0x13;
                    Moby_PlaySound(4, 0, (int)moby);
                    if (((Moby *)moby)->prev_seq != 0) {
                        MobyBlendAnimation(moby, 0, 0, scale_ticks(3));
                    }
                    break;
                }
                if (vec_distance_xy(moby + 0x10, v3) < 1.5f && ((Moby *)moby)->state == 5) {
                    ((Moby *)moby)->state = 6;
                    if (((Moby *)moby)->prev_seq != 1) {
                        MobyBlendAnimation(moby, 1, 0, scale_ticks(3));
                    }
                }
                break;
            }
            if (d->i278 >= 0) {
                ((Moby *)moby)->state = 7;
            } else if (d->t25E != 0) {
                ((Moby *)moby)->state = 0xE;
            } else {
                ((Moby *)moby)->state = 0x12;
            }
            break;
        case 0x6:
            a = fast_atan2(d->tgt[0] - ((Moby *)moby)->pos.x, d->tgt[1] - ((Moby *)moby)->pos.y);
            MobySpringTurn_v((char *)moby, a, &d->f240, 0.05f, 0.3f, 0.2f);
            if (((Moby *)moby)->seq == ((Moby *)moby)->prev_seq &&
                is_value_within_interpolated_window((char *)moby, 13.0f)) {
                vec_scale_to_length(v0, moby + 0xC0, 0.75f);
                FastVecAdd(v0, v0, moby + 0x10);
                v0[2] += 0.5f;
                MobySphereHitDirectional(moby, 0.5f, v0, 1, 1.0f, 1.0f, 0, 1, 0);
            }
            if (((Moby *)moby)->anim_flags & 2) {
                if (vec_distance_xy(moby + 0x10, d->tgt) > 1.5f) {
                    ((Moby *)moby)->state = 5;
                    if (((Moby *)moby)->prev_seq != 0) {
                        MobyBlendAnimation(moby, 0, 0, scale_ticks(3));
                    }
                }
            }
            {
                float gr = probe_ground_height(moby + 0x10, 0, 0.5f);
                d->f198 -= g_frame_time_sq * 9.8f;
                ((Moby *)moby)->pos.z += d->f198;
                if (((Moby *)moby)->pos.z < gr) {
                    *(int *)&d->f198 = 0;
                    ((Moby *)moby)->pos.z = gr;
                }
            }
            break;
        case 0x7:
        case 0x17:
            if (((Moby *)moby)->state == 0x17) {
                ((Moby *)moby)->pos.z += sand_shark_sink_depth;
                if (random_integer_below(0xFF) & 1) {
                    *(u128b *)v4 = *(u128b *)(moby + 0x10);
                    emit_sand_shark_single_particle(v4);
                }
            }
            qcopy(v3, d->home);
            a = fast_atan2(v3[0] - ((Moby *)moby)->pos.x, v3[1] - ((Moby *)moby)->pos.y);
            MobySpringTurn_v((char *)moby, a, &d->f240, 0.02f, 0.3f, 0.1f);
            v0[0] = FastCos(((Moby *)moby)->rot.z) * 2.0f;
            v0[1] = FastSin(((Moby *)moby)->rot.z) * 2.0f;
            v0[2] = 0.0f;
            FastVecAdd(v0, v0, moby + 0x10);
            MobySteerToward(moby, &d->i180, v0, v1, 1.0f);
            if (((Moby *)moby)->state == 0x17) {
                ((Moby *)moby)->pos.z -= sand_shark_sink_depth;
            }
            if (vec_distance_xy(moby + 0x10, d->home) < 0.5f) {
                d->i298 = 0;
                if (((Moby *)moby)->state == 7) {
                    if (d->i278 >= 0) {
                        ((Moby *)moby)->state = 3;
                        if (((Moby *)moby)->prev_seq != 2) {
                            MobyBlendAnimation(moby, 2, 0, scale_ticks(3));
                        }
                    } else {
                        ((Moby *)moby)->state = 0x12;
                        d->f1A4 = g_frame_time * 1.5f;
                        if (((Moby *)moby)->prev_seq != 0) {
                            MobyBlendAnimation(moby, 0, 0, scale_ticks(3));
                        }
                    }
                } else {
                    ((Moby *)moby)->state = 2;
                    d->f1A4 = g_frame_time * 1.5f;
                    if (((Moby *)moby)->prev_seq != 0xC) {
                        MobyBlendAnimation(moby, 0xC, 0, scale_ticks(5));
                    }
                }
            }
            if (d->i298 == 0) {
                if (d->i214 != 2) {
                    FastVecSub(v2, d->tgt, d->home);
                    if (d->i278 >= 0) {
                        if (is_point_inside_clip_volume(d->tgt, d->i278)) {
                            ((Moby *)moby)->state = 5;
                            d->f1A4 = d->f254 * g_frame_time;
                        }
                        break;
                    }
                    if (FastVecLength(v2) < d->f24C) {
                        if (((Moby *)moby)->state == 7) {
                            ((Moby *)moby)->state = 5;
                        } else {
                            ((Moby *)moby)->state = 0x16;
                        }
                        d->f1A4 = d->f254 * g_frame_time;
                        if (((Moby *)moby)->prev_seq != 0) {
                            MobyBlendAnimation(moby, 0, 0, scale_ticks(3));
                        }
                    }
                    break;
                }
                if (d->t25E == 0) {
                    break;
                }
                if (random_integer_below(0x13) != 0) {
                    break;
                }
                ((Moby *)moby)->state = 0x15;
                Moby_PlaySound(4, 0, (int)moby);
                if (((Moby *)moby)->prev_seq != 0) {
                    MobyBlendAnimation(moby, 0, 0, 0);
                }
                d->i274 = -1;
                break;
            }
            vec_scale_to_length(v4, moby + 0xC0, g_frame_time * 0.5f);
            FastVecAdd(moby + 0x10, moby + 0x10, v4);
            break;
        case 0x8:
            if (step_moby_motion(moby, d->knock) & 1) {
                ((Moby *)moby)->state = 0xB;
                if (((Moby *)moby)->prev_seq != 4) {
                    MobyBlendAnimation(moby, 4, 8, scale_ticks(3));
                }
                return;
            }
            if (((Moby *)moby)->pos.z < 0.0f) {
                emit_sand_shark_dust_burst(moby);
                DeleteMoby(moby);
                return;
            }
            break;
        case 0x9:
            if (CarriedMobyUpdate(moby, d->knock)) {
                ((Moby *)moby)->state = 1;
                d->sC8 = 0;
            }
            break;
        case 0xC: {
            int in;
            if (is_point_inside_clip_volume(&g_hero.motion.pos, d->i26C)) {
                in = 1;
            } else {
                in = is_point_inside_clip_volume(&g_hero.motion.pos, d->i270) != 0;
            }
            if (in) {
                float dz, vz, ft, k;
                m = (char *)(aridia_mobys_m + (d->i28C << 8));
                dz = *(float *)(m + 0x18) - d->home[2];
                vz = random_float_between(6.0f, 10.0f) * g_frame_time;
                ft = (square_root(g_frame_time_sq * 10.8f * 2.0f * dz) + vz * 2.0f) / (g_frame_time_sq * 10.8f);
                k = vec_distance_xy(d->home, m + 0x10) / ft;
                FastVecSub(d->vel, d->home, m + 0x10);
                d->vel[2] = 0.0f;
                vec_scale_to_length(d->vel, d->vel, k);
                d->vel[2] = vz;
                qcopy(moby + 0x10, m + 0x10);
                *(int *)&((Moby *)moby)->collision = *(int *)(*(char **)(moby + 0x24) + 0x10);
                ((Moby *)moby)->flags &= 0xFFFE;
                ((Moby *)moby)->flags |= 0x1000;
                if (((Moby *)moby)->prev_seq != 0xB) {
                    MobyBlendAnimation(moby, 0xB, 0, 0);
                }
                ((Moby *)moby)->state = 0xD;
            }
            break;
        }
        case 0x1B: {
            int n;
            random_vector(v1, 0.0f, 0.2f);
            FastVecAdd(v0, v1, moby + 0x10);
            FastVecScale(v1, v1, g_frame_time * 5.0f);
            n = scale_ticks(random_int_between(0x14, 0x28));
            spawn_effect(moby + 0x10, v1, sand_shark_puff_start_color, sand_shark_puff_end_color, n, 0x64,
                         random_int_between(0x78, 0x96), 0);
        }
            /* fall through */
        case 0xD: {
            float lim;
            ((Moby *)moby)->rot.y = FastSubRots(((Moby *)moby)->rot.y, g_frame_time * 12.566371f);
            FastVecAdd(moby + 0x10, moby + 0x10, d->vel);
            d->vel[2] -= g_frame_time_sq * 10.8f;
            lim = sand_shark_sink_depth;
            if (((Moby *)moby)->pos.z < d->home[2] - lim) {
                *(int *)&((Moby *)moby)->collision = *(int *)(*(char **)(moby + 0x24) + 0x10);
                ((Moby *)moby)->flags |= 0x1000;
                ((Moby *)moby)->pos.z = d->home[2] - lim;
                ((Moby *)moby)->state = 0x17;
                ((Moby *)moby)->rot.y = 0.0f;
                if (((Moby *)moby)->prev_seq != 0xC) {
                    MobyBlendAnimation(moby, 0xC, 0, scale_ticks(1));
                }
                break;
            }
            if (((Moby *)moby)->pos.z < d->home[2] + 0.6f) {
                *(u128b *)v0 = *(u128b *)(moby + 0x10);
                emit_sand_shark_radial_particles(v0, 6);
            }
            break;
        }
        case 0x18: {
            int res = step_moby_motion(moby, d->knock);
            if ((res & 1) && d->i29C == 0) {
                d->i29C = 1;
                *(u128b *)v0 = *(u128b *)(moby + 0x10);
                emit_sand_shark_radial_particles(v0, 0x1B);
            }
            if (res & 0x140) {
                SetDeathBits(moby, 0, -1);
                if ((d->i27C = select_group_sand_shark_nest_moby(moby)) != 0) {
                    emit_sand_shark_dust_burst(moby);
                    ((Moby *)moby)->anim_speed = 0.0f;
                    ((Moby *)moby)->state = 0x1A;
                    qcopy(moby + 0x10, d->home);
                    ((Moby *)moby)->pos.z -= 10.0f;
                    *(int *)&((Moby *)moby)->collision = 0;
                    ((Moby *)moby)->flags |= 0x41;
                    break;
                }
                emit_sand_shark_dust_burst(moby);
                DeleteMoby(moby);
                return;
            }
            if (((Moby *)moby)->pos.z < 0.0f) {
                emit_sand_shark_dust_burst(moby);
                DeleteMoby(moby);
                return;
            }
            break;
        }
        case 0x19:
            if (((Moby *)moby)->alpha >= 8) {
                ((Moby *)moby)->alpha -= 8;
                break;
            }
            if ((d->i27C = select_group_sand_shark_nest_moby(moby)) != 0) {
                emit_sand_shark_dust_burst(moby);
                ((Moby *)moby)->alpha = 0x80;
                ((Moby *)moby)->state = 0x1A;
                qcopy(moby + 0x10, d->home);
                *(int *)&((Moby *)moby)->collision = 0;
                ((Moby *)moby)->pos.z -= 10.0f;
                break;
            }
            emit_sand_shark_dust_burst(moby);
            DeleteMoby(moby);
            return;
        case 0x1A:
            if ((d->i27C = select_group_sand_shark_nest_moby(moby)) == 0) {
                DeleteMoby(moby);
                return;
            }
            break;
    }
    *(volatile int *)0x10000000;
}

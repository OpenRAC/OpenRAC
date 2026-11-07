extern int D_E1_L07_0020C500[][4];
void snagglebeast_update(unsigned char *m) {
    SbVec oldpos;
    SbTarget tgt;
    SbVec va;
    SbVec vb;
    SbVec vc;
    SbVec ve;
    SbVec vf;
    SbVec vg;
    SbVec vh;
    struct SnagglebeastVars *d = (struct SnagglebeastVars *)((Moby *)m)->pvars;
    int see;
    int t194;
    int t19A;
    int a;
    int b;
    int c;
    int e;
    float dist;
    float ang;
    float hdist;
    float hang;
    int i;
    float *t20;

    Manipulator_ApplyScale(2.5f, (int)m, 0xB, (unsigned char *)&d->scale_manip);
    apply_snagglebeast_big_head_cheat((char *)m);
    if (umbris_scene_state == 2)
        return;
    if (d->unk_1A4 != -1 && !is_point_inside_clip_volume(g_hero + 0x80, d->unk_1A4))
        return;
    if (((Moby *)m)->visible != 0) {
        if (fast_vec_dist(&((Moby *)m)->pos, debris_far_point) < 35.0f) {
            set_ground_height_band(m);
            ((Moby *)m)->shadow_range = 0x1C;
        }
    }
    qcopy(&oldpos, &((Moby *)m)->pos);
    t20 = &d->health;
    process_snagglebeast_damage(m, ((unsigned char *)d), t20);
    see = 0;
    FindNearestTargetMoby(m, &tgt, 30.0f);
    dist = vec_distance_xy(&tgt, &((Moby *)m)->pos);
    ang = fast_atan2(tgt.pos.f[0] - ((Moby *)m)->pos.x, tgt.pos.f[1] - ((Moby *)m)->pos.y);
    hdist = vec_distance_xy(&((Moby *)m)->pos, g_hero + 0x80);
    {
        Hero *g = (Hero *)g_hero;
        hang = fast_atan2(g->motion.pos.f[0] - ((Moby *)m)->pos.x, g->motion.pos.f[1] - ((Moby *)m)->pos.y);
    }
    if (fast_subtract_rotations_cf(&d->unk_1F7)) {
        if ((vec_distance_xy(&((Moby *)m)->pos, g_hero + 0x80) > 8.0f && ((Moby *)m)->state != 5) ||
            (vec_distance_xy(&((Moby *)m)->pos, g_hero + 0x80) > 6.0f &&
             (((Moby *)m)->state == 5 || ((Moby *)m)->command % 3U == 1))) {
            see = 1;
        }
    }
    t19A = 0;
    t194 = FastDecTimer(&d->timer) != 0;
    if (FastDecTimer(&d->unk_19A) && dist > 9.0f)
        t19A = 1;
    a = 0;
    if (d->unk_1C7 != 0) {
        Hero *g = (Hero *)g_hero;
        if (tgt.moby == *(void **)&g->moby && dist < 30.0f &&
            absolute_float(FastSubRots(ang, ((Moby *)m)->rot.z)) < 1.3962634f) {
            a = 1;
        }
    }
    b = 0;
    if (FastDecTimer(&d->unk_198)) {
        if (((((Moby *)m)->state == 5 || ((Moby *)m)->command % 3U == 1) && dist < 14.0f) ||
            (((Moby *)m)->state != 5 && dist < 10.0f))
            b = 1;
    }
    {
        Hero *g = (Hero *)g_hero;
        c = 0;
        e = 0;
        if (((Moby *)m)->pos.z + 1.0f < g->ground_z.f) {
            if (*(char **)&g->ground_moby != 0 && *(short *)(*(char **)&g->ground_moby + 0xA6) >= 0x459 &&
                *(short *)(*(char **)&g->ground_moby + 0xA6) < 0x45D) {
                e = 1;
            }
        }
    }
    switch (((Moby *)m)->command) {
        case 0:
            see = 0;
            a = 0;
            break;
        case 3:
            if (d->unk_1A8 != -1) {
                see = 1;
                c = 1;
                t194 = 0;
                b = 0;
            } else {
                see = 0;
            }
            a = 0;
            break;
        case 6:
            if (d->unk_1A8 != -1) {
                see = 1;
                c = 1;
                t194 = 0;
                b = 0;
            } else {
                see = 0;
            }
            break;
        case 1:
        case 4:
        case 7:
            t194 = 0;
            a = 0;
            if (((Moby *)m)->state != 1)
                t19A = 0;
            break;
        case 2:
        case 5:
        case 8:
            b = 0;
            t194 = 0;
            a = 0;
            t19A = 0;
            break;
    }
    d->unk_1B4 = truncate_float_to_s32(d->health * 16.0f) & 0xFFF0;
    if (d->unk_1B4 >= truncate_float_to_s32(((float)d->hit_timer - 0.5f) * 16.0f)) {
        d->unk_1B4 |= 0xF;
    }
    if (((Moby *)m)->command % 3U == 0) {
        if (d->health > 0.5f) {
            int r = ((truncate_float_to_s32(d->health) % (d->hit_timer / 6)) * 16) / (d->hit_timer / 6);
            if (r < 0)
                r = 0;
            d->unk_1B4 += r & 0xF;
        } else {
            d->unk_1B4 = 0;
        }
    }
    if (((Moby *)m)->command % 3U == 2)
        d->unk_1B4 += 0xF;
    if (((Moby *)m)->state != 8 && ((Moby *)m)->state != 0)
        suck_beam_queue_draw((int)m);
    if (d->unk_1C5 == 1) {
        if (d->voice == -1)
            d->voice = Moby_PlaySound(0xF, 4, (int)m);
        d->sphere_t += frame_scale * 0.01f;
        if (d->sphere_t >= 1.0f)
            d->unk_1C5 = 0;
        if (d->sphere_t > 1.0f)
            d->sphere_t = 1.0f;
    }
    if (d->unk_1C5 == 2) {
        if (d->voice == -1)
            d->voice = Moby_PlaySound(0xF, 4, (int)m);
        d->sphere_t -= frame_scale * 0.01f;
        if (d->sphere_t <= 0.0f) {
            if (d->voice != -1) {
                char *sl = voice_pool + d->voice * 0x70;
                if (*(void **)(sl + 0x88) == m && *(unsigned char *)(sl + 0x74) != 0) {
                    Sound_StopSlot(d->voice);
                }
            }
            d->unk_1C5 = 0;
            d->voice = -1;
        }
        if (d->sphere_t < 0.0f)
            d->sphere_t = 0.0f;
    }
    if (d->sphere_t > 0.0f)
        AddDrawCallback(draw_snagglebeast_sphere_effect, m);
    update_snagglebeast_target_joint_animation((char *)m, (char *)((unsigned char *)d), (float *)(g_hero + 0x80));
    if (((Moby *)m)->state != 0 && ((Moby *)m)->state != 0xA && ((Moby *)m)->state != 0x10) {
        d->hud_anim = queue_animation_update(0x16, 0xFFFF, (int)reset_numeric_hud_element, (int)Hud_UpdateCounter,
                                             (int)((char *)draw_segmented_hud_meter_base + 0x20), (int)&d->unk_1B4,
                                             (d->hit_timer << 4) | 0xF);
    }
    switch (((Moby *)m)->state) {
        case 0:
            snagglebeast_start_cleared_word = 0;
            ((Moby *)m)->state = 1;
            ((Moby *)m)->command = 0;
            d->unk_280 = -1;
            d->unk_190 = 0.0f;
            if (d->unk_18C != -1) {
                int i;
                for (i = 0; i < *(int *)umbris_paths[d->unk_18C]; i++) {
                    ((SbPath *)umbris_paths[d->unk_18C])->pts[i][3] = fast_vec_dist(
                        umbris_paths[d->unk_18C] + (i * 16 + 0x10),
                        umbris_paths[d->unk_18C] + (((i + 1) % *(int *)umbris_paths[d->unk_18C]) * 16 + 0x10));
                    d->unk_190 += ((SbPath *)umbris_paths[d->unk_18C])->pts[i][3];
                }
            }
            if (d->home_path != -1) {
                char *tbl = umbris_paths[d->home_path];
                int i;
                for (i = 0; i < *(int *)tbl; i++) {
                    ((SbPath *)tbl)->pts[i][3] =
                        fast_vec_dist(tbl + (i * 16 + 0x10), tbl + (((i + 1) % *(int *)tbl) * 16 + 0x10));
                }
            }
            if (d->path_idx != -1) {
                int i;
                for (i = 0; i < *(int *)umbris_paths[d->path_idx]; i++) {
                    int k = truncate_float_to_s32(((SbPath *)umbris_paths[d->path_idx])->pts[i][3]);
                    if ((unsigned int)k < 4)
                        ((SbData *)((unsigned char *)d))->b1D4[k] = i;
                }
            }
            d->unk_A8 &= ~8;
            ((Moby *)m)->flags |= 0x1000;
            build_symmetric_adjacency_masks(&d->unk_170, 3, d->path_idx);
            d->unk_19C = 7;
            ((Moby *)m)->update_dist = 0xFF;
            ((Moby *)m)->draw_dist = 0xFF;
            d->unk_1A8 = -1;
            d->hud_anim = -1;
            d->unk_1C0 = select_point_index_by_distance(&((Moby *)m)->pos, umbris_paths[d->path_idx], 0.0f);
            suck_reset(m);
            ((Moby *)m)->scale *= 1.75f;
            d->unk_1C7 = 0;
            d->turn_limit = 1.7453293f;
            ((unsigned char *)d)[0x29] = 1;
            d->unk_58 = truncate_float_to_s32(30.0f);
            d->unk_5A = truncate_float_to_s32(40.0f);
            break;
        case 1:
            if (((Moby *)m)->anim_flags & 2) {
                if (((Moby *)m)->prev_seq != (func_001160D8() & 1)) {
                    int r1 = func_001160D8() & 1;
                    int r2 = random_int_between(0, 0xF);
                    MobyBlendAnimation(m, r1, r2, random_int_between(0x1E, 0x2A));
                }
            }
            if (fast_dec_timer(&d->phase_timer)) {
                if (t19A) {
                    d->phase = 0;
                    d->phase_timer = 0;
                    if (((Moby *)m)->prev_seq != 5) {
                        int r = random_int_between(0, 2);
                        MobyBlendAnimation(m, 5, r, random_int_between(0x12, 0x14));
                    }
                    ((Moby *)m)->state = 4;
                }
                if (t194) {
                    if (((Moby *)m)->state != 5 || random_integer_below(4) == 0) {
                        d->phase = 0;
                        if (((Moby *)m)->prev_seq != 4) {
                            int r = random_int_between(0, 9);
                            MobyBlendAnimation(m, 4, r, random_int_between(0x14, 0x28));
                        }
                        ((Moby *)m)->state = 3;
                        ((Moby *)m)->anim_speed = 1.0f;
                    }
                }
                if (a) {
                    if (((Moby *)m)->prev_seq != 6) {
                        int r = random_int_between(0, 2);
                        MobyBlendAnimation(m, 6, r, random_int_between(0xF, 0x19));
                    }
                    d->phase = 0;
                    d->rope_len = 0.0f;
                    d->unk_1C4 = 0;
                    ((Moby *)m)->state = 8;
                    d->unk_294 = 0;
                    d->voice = Moby_PlaySound(9, 4, (int)m);
                }
                if (b) {
                    if (((Moby *)m)->prev_seq != 9) {
                        int r = random_int_between(0, 2);
                        MobyBlendAnimation(m, 9, r, random_int_between(0x11, 0x19));
                    }
                    ((Moby *)m)->state = 2;
                }
            }
            if (see && ((Moby *)m)->state != 8) {
                if (((Moby *)m)->prev_seq != 0xA) {
                    int r = random_int_between(0, 4);
                    MobyBlendAnimation(m, 0xA, r, random_int_between(0x14, 0x1E));
                }
                ((Moby *)m)->anim_speed = 1.5f;
                d->unk_298 = 0;
                ((Moby *)m)->state = 5;
            }
            break;
        case 2: {
            float rot;
            rot = ((Moby *)m)->rot.z;
            if (umbris_frame_count % 6 == 0)
                spawn_moby_point_offset_particle((char *)m, 0, 600000.0f, 5.0f);
            if ((umbris_frame_count - 2) % 6 == 0)
                spawn_moby_point_offset_particle((char *)m, 1, 600000.0f, 5.0f);
            if (start_snagglebeast_turn_toward_target(m, ((unsigned char *)d), (float *)(g_hero + 0x80), 9, 0))
                break;
            MobySpringTurn_v((char *)m, ang, (float *)&d->turn_vel, g_frame_time_sq * 2.0943952f,
                             g_frame_time_sq * 0.6981317f, g_frame_time * 2.0943952f);
            if (FastDiffRots(((Moby *)m)->rot.z, rot) > g_frame_time * 0.17453292f) {
                if (((Moby *)m)->prev_seq != 0xD) {
                    int r = random_int_between(0, 1);
                    MobyBlendAnimation(m, 0xD, r, random_int_between(0xF, 0x16));
                }
            } else {
                if (((Moby *)m)->prev_seq != 9) {
                    int r = random_int_between(0, 1);
                    MobyBlendAnimation(m, 9, r, random_int_between(0xF, 0x16));
                }
            }
            if (((Moby *)m)->seq == 9 || ((Moby *)m)->seq == 0xD)
                ((Moby *)m)->state =
                    update_alternating_joint_emission_burst(m, (char *)((unsigned char *)d), (float *)&tgt);
            if ((((Moby *)m)->command % 3U == 0 && vec_distance_xy(&tgt, &((Moby *)m)->pos) > 13.0f) ||
                (((Moby *)m)->command % 3U == 1 && vec_distance_xy(&tgt, &((Moby *)m)->pos) > 20.0f)) {
                d->unk_198 = scale_ticks(0x23);
                d->phase_timer = scale_ticks(0x1E);
                ((Moby *)m)->state = 1;
            }
            break;
        }
        case 3:
            MobySpringTurn_v((char *)m, ang, (float *)&d->turn_vel, g_frame_time_sq * 1.5707964f,
                             g_frame_time_sq * 0.6981317f, g_frame_time * 1.0471976f);
            if (is_value_within_interpolated_window(m, 31.0f) || is_value_within_interpolated_window(m, 67.0f) ||
                is_value_within_interpolated_window(m, 101.0f)) {
                d->phase += 1;
                qcopy(&va, &((Moby *)m)->pos);
                d->turn_vel = 0;
                spawn_fading_ring_effect((int)m, &va, 0x3A, 1.0f, 25.0f, 2.0f, g_frame_time * 8.0f, 1.0f);
                moby_point_position(m, (d->phase & 1) + 5, &va);
                configure_timed_point_light(m, ((unsigned char *)d), 1, &va, scale_ticks(0x1E), 0x800060FF, 0, 10.0f);
                va.f[2] = ((Moby *)m)->pos.z;
                for (i = 0; i < 55; i++) {
                    float r1 = random_angle_radians();
                    float r2 = random_float_between(0.3926991f, 0.78539819f);
                    BuildSphericalOffset(&vb, g_frame_time * 5.0f, r1, r2);
                    vec_scale_to_length(&vc, &vb, 0.75f);
                    vc.f[2] += 0.1f;
                    FastVecAdd(&vc, &vc, &va);
                    *(int *)&vb.f[3] = 0;
                    spawn_white_particle_with_payload(&vc, &vb);
                }
                rumble_by_distance((char *)m);
            }
            if (d->phase >= 3 && is_value_within_interpolated_window(m, 51.0f)) {
                if (t19A) {
                    d->timer = 0xC0;
                    d->phase_timer = 0xAA;
                } else {
                    d->timer = scale_ticks(0x3C);
                    d->phase_timer = scale_ticks(0x37);
                    if (d->unk_19A <= scale_ticks(0x37))
                        d->unk_19A += scale_ticks(0x3C);
                }
                ((Moby *)m)->anim_speed = 1.0f;
                if (((Moby *)m)->prev_seq != (func_001160D8() & 1)) {
                    int r1 = func_001160D8() & 1;
                    int r2 = random_int_between(0, 0xF);
                    MobyBlendAnimation(m, r1, r2, random_int_between(0x1E, 0x2A));
                }
                ((Moby *)m)->state = 1;
            }
            break;
        case 4:
            if (start_snagglebeast_turn_toward_target(m, ((unsigned char *)d), (float *)(g_hero + 0x80), 5, 0))
                break;
            if (d->phase_timer < scale_ticks(0x3C) / 2) {
                moby_point_position(m, 2, &va);
                configure_timed_point_light(m, ((unsigned char *)d), 2, &va, scale_ticks(0x1E), 0x800060FF, 0, 10.0f);
                if ((umbris_frame_count & 7) == 0)
                    spawn_moby_point_offset_particle((char *)m, 2, 0.0f, 0.2f);
                if (((umbris_frame_count - 4) & 7) == 0)
                    spawn_moby_point_offset_particle((char *)m, 3, 0.0f, 0.2f);
            }
            if (((Moby *)m)->seq == 5) {
                qcopy(&va, &tgt);
                va.f[2] = probe_ground_height(&va, 0, 0.5f);
                d->phase_timer += 1;
                if (d->phase == 0) {
                    FastVecSub(&vb, &((Moby *)m)->pos, &va);
                    vb.f[2] = 0.0f;
                    vec_scale_to_length(&vb, &vb, 1.0f);
                    vec_scale_to_length(&vg, &vb, 0.075f);
                    vc.f[0] = FastCos(tgt.v10.f[2]);
                    vc.f[1] = FastSin(tgt.v10.f[2]);
                    vc.f[2] = 0.0f;
                    {
                        float s = FastVecDot(&vc, &vb);
                        vec_scale_to_length(&vh, &vb, s);
                    }
                    FastVecSub(&vc, &vc, &vh);
                    vc.f[2] = 0.0f;
                    vec_scale_to_length(&ve, &vb, 8.0f);
                    vec_scale_to_length(&vc, &vc, 4.0f);
                    FastVecAdd(&ve, &ve, &va);
                    FastVecSub(&ve, &ve, &vc);
                    vec_scale_to_length(&vc, &vc, 8.0f);
                    FastVecAdd(&vc, &vc, &va);
                    vec_scale_to_length(&vb, &vb, 2.0f);
                    FastVecSub(&vc, &vc, &vb);
                    qcopy(&vh, g_hero + 0x100);
                    vh.f[2] = 0.0f;
                    FastVecScale(&vf, &vh, (float)scale_ticks(0x28));
                    FastVecSub(&vb, &vc, &ve);
                    FastVecScale(&vb, &vb, 0.5f);
                    FastVecAdd(&vb, &vb, &ve);
                    FastVecSub(&vb, &vb, &va);
                    FastVecSub(&vb, &vb, &vg);
                    FastVecSub(&vb, &vf, &vb);
                    FastVecAdd(&ve, &ve, &vb);
                    FastVecAdd(&vc, &vc, &vb);
                    qcopy(d->rope_start, &ve);
                    qcopy(d->rope_end, &vc);
                    qcopy(d->attach_pos, &ve);
                    FastVecSub(&d->rope_dir, &vc, &ve);
                    FastVecScale(&d->rope_dir, &d->rope_dir, 1.0f / (float)scale_ticks(0x3C));
                    qcopy(&d->unk_150, d->attach_pos);
                    d->unk_15C = 0;
                    d->phase = 1;
                } else {
                    if (d->phase_timer < scale_ticks(0x3C)) {
                        FastVecAdd(d->attach_pos, d->attach_pos, &d->rope_dir);
                        AddDrawCallback(draw_collision_clipped_moby_beam, m);
                        d->phase = 2;
                    } else {
                        d->phase = 0;
                        d->unk_19A = scale_ticks(0xF0);
                        d->phase_timer = scale_ticks(0xC8);
                        ((Moby *)m)->state = 1;
                    }
                }
            }
            break;
        case 5:
        case 6: {
            float thr = 2.0f;
            float rot = ((Moby *)m)->rot.z;
            float dd;
            collect_snagglebeast_navigation_ids(m, ((unsigned char *)d), &vc);
            if (!c) {
                thr = 6.0f;
                if (d->clip_volume != -1 && (d->unk_19C & 3) == 0 &&
                    is_point_inside_clip_volume(&tgt, d->clip_volume)) {
                    qcopy(&vb, umbris_paths[d->path_idx2] +
                                   select_point_index_by_distance(&tgt, umbris_paths[d->path_idx2], 0.0f) * 16 + 0x10);
                    dd = vec_distance_xy(&((Moby *)m)->pos, &vb);
                } else {
                    qcopy(&vb, &tgt);
                    qcopy(&vb, umbris_paths[d->home_path] +
                                   select_point_index_by_distance(&vb, umbris_paths[d->home_path], 0.0f) * 16 + 0x10);
                    dd = vec_distance_xy(&((Moby *)m)->pos, &vb);
                    if (dd < d->unk_9C + 1.0f)
                        qcopy(&vb, &tgt);
                }
            } else {
                qcopy(&vb, umbris_paths[d->path_idx] + d->unk_1A8 * 16 + 0x10);
                dd = vec_distance_xy(&((Moby *)m)->pos, &vb);
            }
            update_snagglebeast_chase_speed(m, ((unsigned char *)d), dd, thr, 1.5f);
            if ((d->unk_298 & 3) == 0)
                select_snagglebeast_chase_waypoint((char *)m, (char *)((unsigned char *)d), &vb, d->attach_pos);
            qcopy(&va, d->attach_pos);
            d->unk_298 += 1;
            if (d->turn_limit < FastDiffRots(((Moby *)m)->rot.z,
                                             fast_atan2(va.f[0] - ((Moby *)m)->pos.x, va.f[1] - ((Moby *)m)->pos.y))) {
                ((Moby *)m)->anim_speed = 1.0f;
                d->unk_1F4 = ((Moby *)m)->state;
                d->unk_1F6 = 0xA;
                d->unk_1F5 = 1;
                qcopy(&d->target_x, &va);
                if (((Moby *)m)->prev_seq != 3) {
                    int r = random_int_between(0, 1);
                    MobyBlendAnimation(m, 3, r, random_int_between(0xF, 0x16));
                }
                ((Moby *)m)->state = 7;
            } else {
                int hit =
                    update_snagglebeast_chase_movement((char *)m, (char *)((unsigned char *)d), &vb, (float *)&va, thr);
                float spd;
                int near;
                int moving;
                FastDiffRots(((Moby *)m)->rot.z, rot);
                spd = FastVecLength(d->move_vec);
                near = 0;
                if (!c && dist < 20.0f)
                    near = 1;
                moving = g_frame_time * 0.01f < spd;
                /* FAKEMATCH: a guess at code v1 had here and later versions dropped. Retail only shows the
                   compares against 4 and 2 and a leftover `li 2`; the variable, the values and the test on 1
                   are invented. Flow deletes the arms, jump deletes the last compare and reorg makes bnel. */
                if (hit == 4)
                    spd = 0.0f;
                else if (hit == 2)
                    spd = 1.0f;
                else if (hit == 1)
                    spd = 2.0f;
                if (hit == 4 || !moving) {
                    d->unk_1F7 = scale_ticks(0x5A);
                    if (((Moby *)m)->prev_seq != (func_001160D8() & 1)) {
                        int r1 = func_001160D8() & 1;
                        int r2 = random_int_between(0, 9);
                        MobyBlendAnimation(m, r1, r2, random_int_between(0x14, 0x28));
                    }
                    d->unk_1A8 = -1;
                    ((Moby *)m)->state = 1;
                    break;
                }
                if (((Moby *)m)->prev_seq != 0xA) {
                    if (!near && (((Moby *)m)->anim_flags & 2))
                        MobyBlendAnimation(m, 0xA, 5, random_int_between(0xA, 0xD));
                } else {
                    if (near && (((Moby *)m)->anim_flags & 2))
                        MobyBlendAnimation(m, 0xC, 5, random_int_between(0xA, 0xD));
                }
                if (fast_dec_timer(&d->phase_timer)) {
                    if (t19A) {
                        d->phase = 0;
                        d->phase_timer = 0;
                        ((Moby *)m)->anim_speed = 1.0f;
                        if (((Moby *)m)->prev_seq != 5) {
                            int r = random_int_between(0, 2);
                            MobyBlendAnimation(m, 5, r, random_int_between(0x14, 0x19));
                        }
                        ((Moby *)m)->state = 4;
                    }
                    if (t194) {
                        d->phase = 0;
                        if (((Moby *)m)->prev_seq != 4) {
                            int r = random_int_between(0, 5);
                            MobyBlendAnimation(m, 4, r, random_int_between(0x14, 0x28));
                        }
                        ((Moby *)m)->state = 3;
                        ((Moby *)m)->anim_speed = 1.0f;
                    }
                    if (e) {
                        d->phase = 0;
                        {
                            Hero *g = (Hero *)g_hero;
                            qcopy(&d->unk_150, *(char **)&g->ground_moby + 0x10);
                        }
                        d->unk_158 += 3.0f;
                        ((Moby *)m)->state = 9;
                    }
                    if (a) {
                        ((Moby *)m)->anim_speed = 1.0f;
                        if (((Moby *)m)->prev_seq != 6) {
                            int r = random_int_between(0, 2);
                            MobyBlendAnimation(m, 6, r, random_int_between(0x14, 0x19));
                        }
                        d->phase = 0;
                        d->rope_len = 0.0f;
                        d->unk_1C4 = 0;
                        d->unk_294 = 0;
                        ((Moby *)m)->state = 8;
                        d->voice = Moby_PlaySound(9, 4, (int)m);
                    }
                }
                if (!see) {
                    ((Moby *)m)->anim_speed = 1.0f;
                    if (((Moby *)m)->prev_seq != (func_001160D8() & 1)) {
                        int r1 = func_001160D8() & 1;
                        int r2 = random_int_between(0, 2);
                        MobyBlendAnimation(m, r1, r2, random_int_between(0x1C, 0x28));
                    }
                    ((Moby *)m)->state = 1;
                }
                if (((Moby *)m)->seq == 0xC)
                    update_alternating_joint_emission_burst(m, (char *)((unsigned char *)d), (float *)&tgt);
            }
            if (((Moby *)m)->seq == 0xA || ((Moby *)m)->seq == 0xC) {
                if (is_value_within_interpolated_window(m, 27.5f))
                    rumble_by_distance((char *)m);
                if (is_value_within_interpolated_window(m, 73.0f))
                    rumble_by_distance((char *)m);
            }
            break;
        }
        case 7: {
            float diff;
            if (d->unk_1F5 == 0)
                qcopy(&d->target_x, g_hero + 0x80);
            diff = FastSubRots(fast_atan2(d->target_x - ((Moby *)m)->pos.x, d->target_y - ((Moby *)m)->pos.y),
                               ((Moby *)m)->rot.z);
            if (!b && (((Moby *)m)->anim_flags & 2)) {
                if (diff < 0.0001f) {
                    if (((Moby *)m)->prev_seq != 3)
                        MobyBlendAnimation(m, 3, 5, random_int_between(0xA, 0xD));
                } else {
                    if (((Moby *)m)->prev_seq != 0xE)
                        MobyBlendAnimation(m, 0xE, 5, random_int_between(0xA, 0xD));
                }
            }
            if (((Moby *)m)->seq == 3 || ((Moby *)m)->seq == 0xE || ((Moby *)m)->seq == 0xD) {
                float dz = compute_interpolated_record_value(m);
                if (dz < 14.5f || (dz > 19.0f && dz < 35.5f) || dz > 40.5f) {
                    MobySpringTurn_v((char *)m,
                                     fast_atan2(d->target_x - ((Moby *)m)->pos.x, d->target_y - ((Moby *)m)->pos.y),
                                     (float *)&d->turn_vel, g_frame_time_sq * 2.0943952f, g_frame_time_sq * 0.6981317f,
                                     g_frame_time * 2.0943952f);
                }
                if (diff > 0.001f && ((Moby *)m)->prev_seq == 3) {
                    MobyBlendAnimation(m, 0xE, 5, random_int_between(0xA, 0xD));
                } else if (diff < 0.001f && ((Moby *)m)->prev_seq == 0xE) {
                    MobyBlendAnimation(m, 3, 5, random_int_between(0xA, 0xD));
                }
            } else {
                d->unk_84 = 0;
            }
            if (FastDiffRots(((Moby *)m)->rot.z,
                             fast_atan2(d->target_x - ((Moby *)m)->pos.x, d->target_y - ((Moby *)m)->pos.y)) <
                d->turn_limit * 0.25f) {
                qzero(d->move_vec);
                if (((Moby *)m)->prev_seq != d->unk_1F6) {
                    int anim = d->unk_1F6;
                    int r = random_int_between(0, 2);
                    MobyBlendAnimation(m, anim, r, random_int_between(0xF, 0x1B));
                }
                ((Moby *)m)->state = d->unk_1F4;
            }
            if (((Moby *)m)->seq == 3 || ((Moby *)m)->seq == 0xE) {
                if (is_value_within_interpolated_window(m, 16.0f))
                    rumble_by_distance((char *)m);
                if (is_value_within_interpolated_window(m, 37.0f))
                    rumble_by_distance((char *)m);
            } else if (((Moby *)m)->seq == 0xD) {
                if (is_value_within_interpolated_window(m, 13.0f))
                    rumble_by_distance((char *)m);
                if (is_value_within_interpolated_window(m, 34.0f))
                    rumble_by_distance((char *)m);
            }
            break;
        }
        case 12: {
            int k;
            if (d->health <= IntToFloat(d->hit_timer / 6)) {
                if (((Moby *)m)->prev_seq != 8)
                    MobyBlendAnimation(m, 8, 0x35, random_int_between(0xA, 0xD));
            } else {
                if (((Moby *)m)->prev_seq != 0xB)
                    MobyBlendAnimation(m, 0xB, 0x3D, 1);
            }
            ((Moby *)m)->state = 0xD;
            d->unk_108 = 0.0f;
            if (((Moby *)m)->command % 3U < 2)
                ((Moby *)m)->command += 1;
            if (((Moby *)m)->command % 3U < 2)
                ((Moby *)m)->command += 1;
            if (((Moby *)m)->command >= 8)
                ((Moby *)m)->command = 8;
            if (((SbData *)((unsigned char *)d))->i1D8[d->unk_19D] != -1) {
                int k2;
                k = d->unk_19D;
                k2 = k + 3;
                if (((SbData *)((unsigned char *)d))->i1D8[k2] != -1) {
                    char *o1 = *(char **)(level_mobys + (((SbData *)((unsigned char *)d))->i1D8[k] << 8) + 0x78);
                    char *o2 = *(char **)(level_mobys + (((SbData *)((unsigned char *)d))->i1D8[k2] << 8) + 0x78);
                    if (*(int *)(o1 + 0x28) != -1 && *(int *)(o2 + 0x28) != -1) {
                        float d1 = vec_distance_xy(g_hero + 0x80, l07_cuboids + (*(int *)(o1 + 0x28) << 7) + 0x30);
                        int sel;
                        if (vec_distance_xy(g_hero + 0x80, l07_cuboids + (*(int *)(o2 + 0x28) << 7) + 0x30) < d1) {
                            sel = *(int *)(o2 + 0x28);
                            k = k2;
                        } else {
                            sel = *(int *)(o1 + 0x28);
                        }
                        ((Moby *)m)->rot.z =
                            fast_atan2(*(float *)(l07_cuboids + sel * 0x80 + 0x30) - ((Moby *)m)->pos.x,
                                       *(float *)(l07_cuboids + sel * 0x80 + 0x34) - ((Moby *)m)->pos.y);
                    }
                }
                *(unsigned char *)(level_mobys + ((SbData *)((unsigned char *)d))->i1D8[k] * 0x100 + 0xBC) = 1;
            }
            break;
        }
        case 13: {
            int done = 0;
            if (compute_interpolated_record_value(m) > 10.0f) {
                moby_point_position(m, 0xA, &va);
                va.f[2] = 34.6f;
                if (random_integer_below(0x1A) == 0)
                    spawn_moving_resizing_height_snapped_alpha_fade_particle(&va, 0, 0x40103080, 1.6f, 14700.0f);
            }
            if (((Moby *)m)->command == 8) {
                float z = d->unk_108;
                FastVecScale(d->move_vec, d->move_vec, 0.7f);
                d->unk_108 = z;
                d->unk_108 -= g_frame_time * 9.8f;
                FastVecAdd(&((Moby *)m)->pos, &((Moby *)m)->pos, d->move_vec);
                if (((Moby *)m)->pos.z < d->unk_1B8)
                    done = 1;
            } else {
                if (compute_interpolated_record_value(m) > 79.0f)
                    done = 1;
            }
            if (done) {
                d->phase_timer = scale_ticks(0xA);
                d->unk_1C5 = 2;
                ((Moby *)m)->state = 0xE;
                d->phase = d->hit_timer / 6;
            }
            break;
        }
        case 14:
            FastVecScale(d->move_vec, d->move_vec, 0.4f);
            FastVecAdd(&((Moby *)m)->pos, &((Moby *)m)->pos, d->move_vec);
            if (compute_interpolated_record_value(m) < 168.0f) {
                moby_point_position(m, 0xA, &va);
                va.f[2] = 34.6f;
                if (random_integer_below(0x1A) == 0)
                    spawn_moving_resizing_height_snapped_alpha_fade_particle(&va, 0, 0x40103080, 1.6f, 14700.0f);
            }
            if (is_value_within_interpolated_window(m, 168.0f)) {
                moby_point_position(m, 5, &va);
                spawn_fading_ring_effect((int)m, &va, 0x3A, 1.0f, 25.0f, 2.0f, g_frame_time * 8.0f, 1.0f);
            }
            if (is_value_within_interpolated_window(m, 168.5f)) {
                moby_point_position(m, 6, &va);
                spawn_fading_ring_effect((int)m, &va, 0x3A, 1.0f, 25.0f, 2.0f, g_frame_time * 8.0f, 1.0f);
                rumble_by_distance((char *)m);
            }
            if (d->phase > 0) {
                if (fast_dec_timer(&d->phase_timer)) {
                    d->phase -= 1;
                    *t20 -= 1.0f;
                    d->unk_67 = 0xFA;
                    RerollEffectCounter(m, &d->unk_60);
                    if (*t20 <= 0.0f) {
                        *t20 = 0.0f;
                        if (((Moby *)m)->prev_seq != 8)
                            MobyBlendAnimation(m, 8, 0, random_int_between(0xA, 0xD));
                        if (d->unk_1F0 != -1) {
                            set_moby_dialogue_record_value(m, 1);
                            SetDeathBits(m, 0, -1);
                            jet_clear_persistent(((Moby *)m)->mission);
                            jet_reset_camera(l07_cuboids + (d->unk_1F0 << 7) + 0x30,
                                             l07_cuboids + (d->unk_1F0 << 7) + 0x70);
                        }
                        ((Moby *)m)->state = 0xA;
                    } else {
                        if (d->phase > 0) {
                            d->phase_timer = scale_ticks(0xA);
                        } else {
                            d->phase_timer = scale_ticks(0x14);
                        }
                    }
                }
            } else {
                if (fast_dec_timer(&d->phase_timer)) {
                    switch (d->unk_19D) {
                        case 0:
                            qcopy(d->rope_end, umbris_paths[d->path_idx2] + 0x10);
                            d->unk_19C &= 0xFE;
                            break;
                        case 1:
                            qcopy(d->rope_end, umbris_paths[d->path_idx2] + 0x20);
                            d->unk_19C &= 0xFD;
                            break;
                        case 2: {
                            float d1 = vec_distance_xy(g_hero + 0x80, umbris_paths[d->path_idx2] + 0x30);
                            if (vec_distance_xy(g_hero + 0x80, umbris_paths[d->path_idx2] + 0x40) < d1) {
                                qcopy(d->rope_end, umbris_paths[d->path_idx2] + 0x30);
                            } else {
                                qcopy(d->rope_end, umbris_paths[d->path_idx2] + 0x40);
                            }
                            d->unk_19C &= 0xFB;
                            break;
                        }
                    }
                    d->unk_1A8 = ((SbData *)((unsigned char *)d))->b1D4[d->unk_19D];
                    build_symmetric_adjacency_masks(
                        (int *)&va, (int)collect_snagglebeast_navigation_ids(m, ((unsigned char *)d), &va),
                        d->path_idx);
                    ((Moby *)m)->state = 0xF;
                }
            }
            break;
        case 15: {
            int iF0;
            if (compute_interpolated_record_value(m) > 198.0f) {
                if (((Moby *)m)->prev_seq != 0xA)
                    MobyBlendAnimation(m, 0xA, 0, 0);
                moby_point_position(m, 0xA, &va);
                qcopy(&d->target_x, &va);
                d->unk_C8 = d->unk_128;
                iF0 = 0xA;
                get_moby_joint_matrices(m, 1, &iF0, &vb);
                matrix_to_euler(&vb, &vg);
                d->unk_CC = FastAddRots(((Moby *)m)->rot.z, vg.f[2]);
            }
            if (((Moby *)m)->seq == 0xA) {
                qcopy(&((Moby *)m)->pos, &d->target_x);
                ((Moby *)m)->rot.z = d->unk_CC;
                if (((SbData *)((unsigned char *)d))->i1D8[d->unk_19D] != -1) {
                    *(unsigned char *)(level_mobys + ((SbData *)((unsigned char *)d))->i1D8[d->unk_19D] * 0x100 +
                                       0xBC) = 0;
                }
                ((Moby *)m)->anim_speed = 1.5f;
                ((Moby *)m)->command += 1;
                d->unk_298 = 0;
                ((Moby *)m)->state = 6;
                d->phase_timer = scale_ticks(0x37);
            }
            break;
        }
        case 8: {
            float fr;
            if (d->unk_294 <= 0) {
                if (start_snagglebeast_turn_toward_target(m, ((unsigned char *)d), (float *)(g_hero + 0x80), 6, 0))
                    break;
            }
            d->unk_294 = 1;
            MobySpringTurn_v((char *)m, hang, (float *)&d->turn_vel, g_frame_time_sq * 2.0943952f,
                             g_frame_time_sq * 0.6981317f, g_frame_time * 2.0943952f);
            moby_point_position(m, 4, &va);
            fr = update_snagglebeast_rope(m, &va);
            if (((Moby *)m)->seq != 6)
                break;
            qcopy(d->attach_pos, g_hero + 0x80);
            d->unk_148 += 0.6f;
            if (d->rope_len < hdist) {
                d->phase_timer = 0;
                d->rope_len += g_frame_time * 15.0f;
            } else {
                if (d->unk_1C4 != 0 || (fast_dec_timer(&d->phase_timer) && fr < 0.6f)) {
                    int slot;
                    Hero *g = (Hero *)g_hero;
                    d->unk_1C4 = 1;
                    if (g->state.current != HERO_STATE_CLEAR_VELOCITY) {
                        qcopy(d->rope_start, g_hero + 0x80);
                        qcopy(d->rope_end, g_hero + 0x80);
                        if (((Moby *)m)->pos.z < g->motion.pos.f[2]) {
                            d->unk_128 = g->motion.pos.f[2] + 2.0f;
                        } else {
                            d->unk_128 = ((Moby *)m)->pos.z + 2.0f;
                        }
                        HeroEnterMoveState(HERO_STATE_CLEAR_VELOCITY, 1);
                        d->phase = 0;
                        d->unk_1AC = 20.0f;
                        d->unk_19E = 0;
                    }
                    FastVecSub(&vb, d->rope_end, d->rope_start);
                    FastVecScale(&vb, &vb, frame_scale * 0.1f);
                    FastVecAdd(d->rope_start, d->rope_start, &vb);
                    qcopy(g_hero + 0x1D30, d->rope_start);
                    qcopy(g_hero + 0x80, d->rope_start);
                    {
                        char *g2 = g_hero;
                        *(float *)(g2 + 0x1D48) = FastAddRots(*(float *)(g2 + 0x1D48), g_frame_time * 25.132742f);
                        ((Hero *)g2)->motion.rot.f[2] = FastAddRots(*(float *)(g2 + 0x1D48), g_frame_time * 25.132742f);
                    }
                    d->rope_len = hdist;
                    slot = D_E1_L07_0020C500[d->phase][0];
                    if (slot == -1) {
                        d->phase = 0;
                        d->unk_196 = scale_ticks(0x320);
                        d->phase_timer = scale_ticks(0xF0);
                        detach_snagglebeast_rope_particles((int)m);
                        {
                            char *g3 = g_hero;
                            if (*(int *)(g3 + 0x2084) == 0x78)
                                HeroEnterMoveState(HERO_STATE_IDLE, 1);
                        }
                        d->unk_1C7 = 0;
                        if (d->voice != -1) {
                            char *sl = voice_pool + d->voice * 0x70;
                            if (*(void **)(sl + 0x88) == m && *(unsigned char *)(sl + 0x74) != 0) {
                                Sound_StopSlot(d->voice);
                            }
                        }
                        d->voice = -1;
                        ((Moby *)m)->state = 1;
                    } else {
                        int cnt = (l07_ammo_transfer_records + slot)->s3C;
                        int rem;
                        int pos;
                        if (d->unk_19E == 0) {
                            d->unk_19E = *(int *)(weapon_ammo_counts + slot * 4) / 8;
                            if (d->unk_19E < cnt)
                                d->unk_19E = cnt;
                            d->unk_1AC = random_float_between(d->unk_1AC + 20.0f, d->unk_1AC + 100.0f);
                        }
                        cnt = d->unk_19E;
                        if (*(int *)(weapon_ammo_counts + slot * 4) < cnt) {
                            cnt = *(int *)(weapon_ammo_counts + slot * 4);
                            d->unk_19E = 0;
                            d->phase += 1;
                        }
                        rem = *(int *)(weapon_ammo_counts + slot * 4) - cnt * 8;
                        pos = rem > 0;
                        cnt = cnt + pos * rem;
                        if (cnt > 0) {
                            float zv = 0.0f;
                            if (d->unk_18C == -1) {
                                random_vector((float *)&vb, g_frame_time * 4.0f, g_frame_time * 10.0f);
                                if (vb.f[2] < g_frame_time * 2.0f)
                                    vb.f[2] = random_float_between(g_frame_time * 3.0f, g_frame_time * 6.0f);
                                vec_scale_to_length(&vc, &vb, 0.9f);
                                FastVecAdd(&vc, &vc, g_hero + 0x80);
                            } else {
                                char *path = umbris_paths[d->unk_18C];
                                int pidx = 0;
                                float pfrac = 0.0f;
                                float d1;
                                float d2;
                                pidx = select_point_index_by_distance(g_hero + 0x80, path, 0.0f);
                                do {
                                    d->unk_1AC += 10.0f;
                                    Spline_AdvanceByDistance_v(path, &ve, &pidx, &pfrac, d->unk_1AC, 1);
                                    d1 = vec_distance_xy(g_hero + 0x80, &ve);
                                    d2 = vec_distance_xy(&((Moby *)m)->pos, &ve);
                                } while (d1 < 8.0f || d2 < 12.0f);
                                add_random_xy_offset((float *)&ve, 0.5f);
                                FastVecSub(&vc, &ve, g_hero + 0x80);
                                zv = ve.f[2];
                                vec_scale_to_length(&vc, &vc, 0.9f);
                                qcopy(&vb, &vc);
                                vb.f[2] = 0.0f;
                                vec_scale_to_length(&vb, &vb, g_frame_time * 15.0f);
                                FastVecAdd(&vc, &vc, g_hero + 0x80);
                                vb.f[2] = jump_speed_4318(&vc, &ve, 0, g_frame_time * 15.0f, g_frame_time_sq * -9.8f);
                                d->unk_1AC += random_float_between(0.7f, 2.0f);
                            }
                            Moby_PlaySound(0xB, 0, (int)m);
                            {
                                char *obj = spawn_on_path(&vc, slot, cnt, 0);
                                if (obj != 0) {
                                    qcopy_nc(((char *)((Moby *)obj)->pvars) + 0x10, &vb);
                                    ((struct SnagglebeastVars *)((Moby *)obj)->pvars)->unk_28 = zv;
                                    *(int *)(weapon_ammo_counts + slot * 4) -= cnt;
                                    d->phase_timer = scale_ticks(0);
                                }
                            }
                        }
                    }
                }
            }
            if (!a) {
                d->phase = 0;
                d->unk_196 = scale_ticks(0xF);
                detach_snagglebeast_rope_particles((int)m);
                {
                    char *g3 = g_hero;
                    if (*(int *)(g3 + 0x2084) == 0x78)
                        HeroEnterMoveState(HERO_STATE_IDLE, 1);
                }
                d->phase_timer = scale_ticks(0x46);
                d->unk_1C7 = 0;
                if (d->voice != -1) {
                    char *sl = voice_pool + d->voice * 0x70;
                    if (*(void **)(sl + 0x88) == m && *(unsigned char *)(sl + 0x74) != 0) {
                        Sound_StopSlot(d->voice);
                    }
                }
                d->voice = -1;
                ((Moby *)m)->state = 1;
            }
            if (b) {
                d->unk_196 = scale_ticks(0x1E);
                if (((Moby *)m)->prev_seq != 9) {
                    int r = random_int_between(0, 2);
                    MobyBlendAnimation(m, 9, r, random_int_between(0x10, 0x19));
                }
                detach_snagglebeast_rope_particles((int)m);
                {
                    char *g3 = g_hero;
                    if (*(int *)(g3 + 0x2084) == 0x78)
                        HeroEnterMoveState(HERO_STATE_IDLE, 1);
                }
                if (d->voice != -1) {
                    char *sl = voice_pool + d->voice * 0x70;
                    if (*(void **)(sl + 0x88) == m && *(unsigned char *)(sl + 0x74) != 0) {
                        Sound_StopSlot(d->voice);
                    }
                }
                d->voice = -1;
                ((Moby *)m)->state = 2;
            }
            break;
        }
        case 9:
            if (d->phase <= 0) {
                if (start_snagglebeast_turn_toward_target(m, ((unsigned char *)d), &d->unk_150, 7, 1))
                    break;
            }
            MobySpringTurn_v((char *)m, fast_atan2(d->unk_150 - ((Moby *)m)->pos.x, d->unk_154 - ((Moby *)m)->pos.y),
                             (float *)&d->turn_vel, g_frame_time_sq * 2.0943952f, g_frame_time_sq * 0.6981317f,
                             g_frame_time * 2.0943952f);
            switch (d->phase) {
                case 0:
                    if (((Moby *)m)->prev_seq != 7)
                        MobyBlendAnimation(m, 7, 0, random_int_between(0xA, 0xD));
                    d->phase_timer = 0;
                    d->phase = 1;
                    break;
                case 1:
                    if ((*(unsigned long *)&((Moby *)m)->frame & 0xFFFF0000) != 0x07070000 &&
                        ((Moby *)m)->prev_seq != 7) {
                        MobyBlendAnimation(m, 7, 0, random_int_between(0xA, 0xD));
                    }
                    if (((Moby *)m)->seq == 7) {
                        if (compute_interpolated_record_value(m) >= 9.5f) {
                            moby_point_position(m, 0, &vb);
                            moby_point_position(m, 1, &vc);
                            FastVecAdd(&va, &vb, &vc);
                            FastVecScale(&va, &va, 0.5f);
                            qcopy(d->rope_start, &va);
                            FastVecSub(&d->rope_dir, &d->unk_150, d->rope_start);
                            vec_scale_to_length(&d->rope_dir, &d->rope_dir, frame_scale);
                            d->phase = 2;
                        }
                    }
                    break;
                case 2:
                    if ((*(unsigned long *)&((Moby *)m)->frame & 0xFFFF0000) != 0x07070000 &&
                        ((Moby *)m)->prev_seq != 7) {
                        MobyBlendAnimation(m, 7, 0, random_int_between(7, 0xA));
                    }
                    if (fast_dec_timer(&d->phase_timer)) {
                        d->phase_timer = scale_ticks(0xA);
                        spawn_moby_point_offset_particle((char *)m, 0, 800000.0f, 5.0f);
                        spawn_moby_point_offset_particle((char *)m, 1, 800000.0f, 5.0f);
                    }
                    if (compute_interpolated_record_value(m) >= 31.5f) {
                        d->rope_len = 0.0f;
                        d->phase = 3;
                        d->unk_11C = 0;
                    }
                    break;
                case 3: {
                    float r = vec_distance_xy(&((Moby *)m)->pos, &tgt.v30);
                    d->rope_len += frame_scale;
                    if (r <= d->rope_len)
                        d->phase = 4;
                    if (fast_dec_timer(&d->phase_timer))
                        d->phase_timer = scale_ticks(0x14);
                    AddDrawCallback(draw_and_probe_snagglebeast_rope_attack, m);
                    break;
                }
                case 4:
                    FastVecSub(&va, d->rope_end, d->rope_start);
                    vec_scale_to_length(&va, &va, frame_scale);
                    FastVecAdd(d->rope_start, d->rope_start, &va);
                    configure_timed_point_light(m, ((unsigned char *)d), 3, d->rope_start, scale_ticks(0x1E),
                                                0x800060FF, 0, 10.0f);
                    if (d->phase != 0) {
                        if (fast_dec_timer(&d->phase_timer))
                            d->phase = 5;
                    }
                    AddDrawCallback(draw_and_probe_snagglebeast_rope_attack, m);
                    break;
                case 5:
                    ((Moby *)m)->anim_speed = 1.0f;
                    d->phase = 0;
                    d->phase_timer = 0;
                    if (((Moby *)m)->prev_seq != (func_001160D8() & 1)) {
                        int r1 = func_001160D8() & 1;
                        int r2 = random_int_between(0, 7);
                        MobyBlendAnimation(m, r1, r2, random_int_between(0x1B, 0x2D));
                    }
                    ((Moby *)m)->state = 1;
                    break;
            }
            break;
        case 10:
            if (d->hud_anim != -1) {
                set_animation_parameter(d->hud_anim, 0);
                d->hud_anim = -1;
            }
            if (((Moby *)m)->anim_flags & 2) {
                vec_scale_to_length(d->move_vec, d->move_vec, 1.0f);
                d->unk_108 = 1.0f;
                qcopy(&va, &((Moby *)m)->pos);
                va.f[2] = d->unk_1B8 + 1.0f;
                spawn_explosion_v(m, d->move_vec, &va, 0.0f, 0.0f, 0x14, 0xC, 0x40, 7.0f, 5.0f, 10.0f, -1, 1.0f, 50.0f,
                                  1, 1, -1, 0);
                ((Moby *)m)->flags |= 0x41;
                if (d->unk_1CC != -1)
                    initialize_moby_particle_effect_state(level_mobys + (d->unk_1CC << 8));
                ((Moby *)m)->state = 0x10;
                ((Moby *)m)->pos.z = 20.0f;
            }
            break;
        case 11:
            if (((Moby *)m)->anim_flags & 2) {
                if (e) {
                    d->phase = 0;
                    {
                        Hero *g = (Hero *)g_hero;
                        qcopy(&d->unk_150, *(char **)&g->ground_moby + 0x10);
                    }
                    d->unk_158 += 3.0f;
                    ((Moby *)m)->state = 9;
                } else {
                    ((Moby *)m)->state = 1;
                }
            }
            break;
        case 16:
            if (d->unk_280 != -1)
                release_point_light_slot(d->unk_280);
            if (d->unk_1D0 == -1 || !(*(float *)(level_mobys + (d->unk_1D0 << 8) + 0x18) < 34.5f)) {
                DeleteMoby(m);
                return;
            }
            *(float *)(level_mobys + (d->unk_1D0 << 8) + 0x18) +=
                (34.6f - *(float *)(level_mobys + (d->unk_1D0 << 8) + 0x18)) * 0.1f;
            break;
        case 17:
            break;
    }
    FastVecSub(d->move_vec, &((Moby *)m)->pos, &oldpos);
    if (((Moby *)m)->state < 0xA)
        jet_animate(m, 2.5f);
    update_timed_point_light_color((char *)((unsigned char *)d));
}

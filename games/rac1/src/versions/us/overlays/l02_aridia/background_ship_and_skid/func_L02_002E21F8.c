void skids_agent_update(Moby *m) {
    float v0[4];
    float v1[4];
    float v2[4];
    float v3[4];
    float v4[4];
    Data_2E21F8 *d = *(struct Data_2E21F8 **)&m->pvars;
    float rate, head_rate;
    int tracking;
    attach_big_head_cheat_manipulator_to_matching_moby(m);
    switch (m->state) {
        case 0:
            if (gHavePersuader[0xA] != 0)
                goto del;
            d->f20 = skids_agent_dialogue_name;
            m->state = 1;
            DialogRequest_Init(m, d);
            initialize_moby_grounding(m);
            d->f178 = (Moby *)CreateMoby(0x38);
            d->f178->draw_dist = 0x40;
            d->f178->visible = 1;
            *(long *)&d->f178->spawn_frame = *(long *)&m->spawn_frame;
            d->f178->flags |= 0x100;
            break;
        case 1:
            if (d->f8 == 1 && is_point_inside_clip_volume(
                                  &((struct Hero *)((char *)&hero_globals_base_r + 0xE1D))->motion.pos, d->f154) != 0) {
                d->fC = 255.0f;
            } else {
                d->fC = 3.7f;
            }
            if (Npc_TryStartDialog(m, d) != 0) {
                begin_dialogue_camera_transition(2.2f, m);
                m->state = 2;
            } else if (gHaveHeliPack[3] == 0 &&
                       vec_distance_xy(((float *)&m->pos),
                                       &((struct Hero *)((char *)&hero_globals_base_r + 0xE1D))->motion.pos) < 3.0f) {
                Stats *stats = (Stats *)help_records;
                if (stats->f70 != 0) {
                    int elapsed, period;
                    elapsed = scale_ticks(gameplay_event_frame_count);
                    period = 600;
                    elapsed -= stats->f72 * period;
                    if (elapsed > (int)((float)scale_ticks(18) * 60.0f) || stats->f72 * period == 0) {
                        help_request(0x7D4, 0xE);
                    } else if (scale_ticks(gameplay_event_frame_count) / 600 > stats->f72) {
                        stats->f72 = scale_ticks(gameplay_event_frame_count) / 600;
                    }
                } else {
                    int now, per;
                    stats->f70++;
                    now = scale_ticks(gameplay_event_frame_count);
                    per = 600;
                    if (now / per > stats->f72) {
                        stats->f72 = scale_ticks(gameplay_event_frame_count) / 600;
                    }
                    stats->f74 = stats->f74 | (1 << g_current_level_index) | 0x80000000;
                }
            }
            if (m->prev_seq == 0 && FastDecTimer((int *)&d->f172) != 0) {
                d->f172 = truncate_float_to_s32(multiply_global_factor(random_float_between(1200.0f, 2400.0f)));
                if (m->prev_seq != 1) {
                    MobyBlendAnimation(m, 1, 0, scale_ticks(10));
                }
            } else if (m->prev_seq == 1 && (m->anim_flags & 2)) {
                MobyBlendAnimation(m, 0, 0, scale_ticks(10));
            }
            break;
        case 2:
            if (aridia_cinematic_mode != 2) {
                m->state = 1;
                if (d->f4 == 0) {
                    jet_clear_persistent(m->mission);
                    if (d->f150 != -1) {
                        char *t = l02_cuboids + (d->f150 << 7);
                        jet_reset_camera(t + 0x30, t + 0x70);
                    }
                } else if (d->f4 == 2) {
                    m->visible = 0;
                    m->flags |= 1;
                    *(int *)&m->collision = 0;
                    if (d->f178 != 0) {
                        d->f178->flags |= 1;
                        d->f178->visible = 0;
                    }
                    PlayMovieEntryIfIdle(2);
                    m->state = 5;
                }
            } else {
                Warp *w = &talk_camera_r;
                if (*(short *)((char *)w + 0x46) == 0) {
                    vec_scale_to_length(w->pos, ((char *)&m->rot_row0), 2.5f);
                    FastVecAdd(w->pos, w->pos, ((float *)&m->pos));
                    w->pos[2] = probe_ground_height(w->pos, 0, 0.5f);
                    clear_u64_value(w->rot);
                    w->rot[2] = FastAddRots(m->rot.z, 3.14159f);
                    *(short *)((char *)w + 0x46) = 1;
                }
            }
            break;
        case 5:
            grant_inventory_item(5, 1);
            memcard_Save(0, -1);
            goto del;
        case 3:
            ApproachWithVelocity(&d->f180, &d->f184, 1.0f, g_frame_time_sq * 0.1f, g_frame_time_sq * 0.25f,
                                 g_frame_time * 0.5f);
            lerp_vector(v0, l02_cuboids + (d->f158 << 7) + 0x30, l02_cuboids + (d->f15C << 7) + 0x30, d->f180);
            {
                Node_2E21F8 *t = (Node_2E21F8 *)l02_cuboids;
                qcopy(v2, t[d->f158].f70);
                qcopy(v3, t[d->f15C].f70);
            }
            v1[2] = FastAddRots(FastSubRots(v3[2], v2[2]) * d->f180, v2[2]);
            v1[1] = FastAddRots(FastSubRots(v3[1], v2[1]) * d->f180, v2[1]);
            v1[0] = FastAddRots(FastSubRots(v3[0], v2[0]) * d->f180, v2[0]);
            set_camera_position(v0);
            set_camera_rotation(v1);
            if (d->f180 < 1.0f) {
                ApproachValue(&d->f17C, 0.0f, 0.1f);
            } else {
                ApproachValue(&d->f17C, 1.0f, 0.1f);
                if (d->f17C == 1.0f) {
                    m->state = 4;
                    jet_end_mount(0);
                }
            }
            aridia_fade_level = d->f17C;
            break;
        case 4:
            ApproachValue(&d->f17C, 0.0f, 0.1f);
            aridia_fade_level = d->f17C;
            if (d->f17C == 0.0f) {
                if (d->f178 != 0) {
                    DeleteMoby(d->f178);
                }
            del:
                DeleteMoby(m);
                return;
            }
            break;
    }
    if (d->f178 != 0) {
        moby_point_position(m, 2, ((float *)&d->f178->pos));
        build_moby_bone_transform(m, 2, v0);
        copy_matrix3x4(((char *)&d->f178->rot_row0), v0);
        transform_vector_by_basis(v4, &skids_agent_prop_offset, ((char *)&d->f178->rot_row0));
        FastVecAdd(((float *)&d->f178->pos), ((float *)&d->f178->pos), v4);
        moby_build_matrix(d->f178);
    }
    rate = 0.02f;
    head_rate = 0.3f;
    tracking = 0;
    if (m->prev_seq == 0) {
        char *player = &((struct Hero *)((char *)&hero_globals_base_r + 0xE1D))->motion.pos;
        tracking = 1;
        if (vec_distance_xy(((float *)&m->pos), player) < 8.0f &&
            FastDiffRots(
                m->rot.z,
                fast_atan2(((Hero *)((char *)&hero_globals_base_r + 0xE1D))->motion.body_center.f[0] - m->pos.x,
                           ((Hero *)((char *)&hero_globals_base_r + 0xE1D))->motion.body_center.f[1] - m->pos.y)) <
                1.5707964f) {
            if (FastVecLength(player + 0x80) > 0.01f)
                d->f174 = scale_ticks(120);
            else
                FastDecTimer((int *)&d->f174);
        } else if (d->f174) {
            d->f174 = 0;
            qcopy(d->f160, &((struct Hero *)((char *)&hero_globals_base_r + 0xE1D))->motion.body_center);
        }
        if (FastDecTimer((int *)&d->f176)) {
            float heading;
            d->f176 = truncate_float_to_s32(multiply_global_factor(random_float_between(180.0f, 300.0f)));
            heading = FastAddRots(m->rot.z, random_float_between(-90.0f, 90.0f) * 0.017453292f);
            BuildSphericalOffset(d->f160, 6.0f, heading, random_float_between(0.0f, 30.0f) * 0.017453292f);
            FastVecAdd(d->f160, d->f160, ((float *)&m->pos));
        }
        if (d->f174) {
            qcopy(v0, &((struct Hero *)((char *)&hero_globals_base_r + 0xE1D))->motion.body_center);
            rate = 0.04f;
            head_rate = 0.3f;
        } else {
            qcopy(v0, d->f160);
        }
    }
    if (tracking) {
        float yaw;
        float pitch;
        qcopy(v1, ((float *)&m->pos));
        v1[2] += 1.0f;
        FastVecSub(v2, v0, v1);
        yaw = FastSubRots(fast_atan2(v2[0], v2[1]), m->rot.z);
        pitch = -fast_atan2(vector_length_xy(v2), v2[2]);
        if (yaw > 1.5707964f)
            yaw = 1.5707964f;
        else if (yaw < -1.5707964f)
            yaw = -1.5707964f;
        if (pitch > 0.5235988f)
            pitch = 0.5235988f;
        else if (pitch < -0.5235988f)
            pitch = -0.5235988f;
        d->fA4 = pitch;
        d->fA8 = yaw * 0.7f;
        d->f128 = yaw * 0.3f;
    }
    if (gCheats[0])
        d->fB0 = 2.75f;
    EaseManipulatorOffsets_r(rate * g_frame_scale_sq, head_rate * g_frame_scale_sq, m, d->f40, 0);
    EaseManipulatorOffsets_r(rate * g_frame_scale_sq, head_rate * g_frame_scale_sq, m, d->fC0, 1);
}

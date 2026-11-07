void build_beam_and_arc_points(u8 *o) {
    Vec4 target0, target1;
    struct {
        Vec4 direction;
        int words[2];
        u8 kind, mode;
        short id;
        float amount;
        int flag;
    } hit;
    Vec4 camera_direction, direction, delta, blend, step, offset;
    M43 *collided[128];
    Vec4 target_delta;
    union {
        struct {
            Vec4 origin;
            HitDescriptor hit0;
        } first;
        HitDescriptor hit1;
    } effects;
    /* Retail uses a five-vector scratch span with differences beginning at slot 1. */
    Vec4 differences[5];
    WalloperBeam *v = *(WalloperBeam **)(o + 0x78);
    int i;
    M43 *best = 0, *second = 0, *candidate, **list;
    char *data;
    float best_score = 0.0f, second_score = 0.0f;
    int enabled = (u8)hero_globals_base[0];
    float spacing = tesla_beam_length / 20.0f;
    float distance, angle, score, n, amplitude, wave, length, factor, phase, side_wave;
    float side_phase;
    float x_s, x_c, x_a, x_m;
    Hero43 *player;
    float *camera;
    Hero43 *aim_hero;
    float primary_distance, sign;
    float distance_w17;
    float distance_w16;
    float factor_w15;
    float factor_w13;
    float length_w4;
    float length_w3;
    int k;
    int m;
    int mirror;
    int seg;
    int j, hit_result, collided_count, spin;
    char *previous, *side;
    Vec4 *beam;
    float *side_waves;
    tesla_beam_reset_short_a = FastAddRots(tesla_beam_reset_short_a, tesla_beam_phase_a_speed);
    tesla_beam_reset_short_b = FastSubRots(tesla_beam_reset_short_b, tesla_beam_phase_b_speed);
    tesla_beam_reset_short_c = FastAddRots(tesla_beam_reset_short_c, tesla_beam_phase_c_speed);
    if (!v->target || (enabled && !v->second_target)) {
        if (v->second_target) {
            v->target = v->second_target;
            v->target_timer = v->second_timer;
            v->second_target = 0;
            v->second_timer = 0;
        } else {
            for (k = 0; (candidate = ((M43 **)active_mobies)[k]) != 0; k++) {
                camera = ((float *)&gCameraLvl);
                aim_hero = &hero43;
                if (candidate == v->target || candidate == v->second_target || !candidate || !candidate->cls ||
                    *(short *)((char *)candidate->cls + 0x46) != 5)
                    continue;
                if (candidate->state == 0xfe)
                    goto next_candidate;
                if (candidate->state == 0xfd)
                    goto next_candidate;
                if (!GetFlaggedPvarWord(candidate))
                    continue;
                FastVecSub(&target0, &candidate->position, v);
                distance_w16 = vector_length_xy(&target0);
                if (tesla_beam_length < distance_w16)
                    continue;
                length_w3 = FastDiffRots(camera[0x56], fast_atan2(candidate->position.v.x - camera[0x50],
                                                                  candidate->position.v.y - camera[0x51]));
                if (tesla_target_max_camera_angle_diff < length_w3)
                    continue;
                score = FastDiffRots(fast_atan2(candidate->position.v.x - aim_hero->position.f[0],
                                                candidate->position.v.y - aim_hero->position.f[1]),
                                     aim_hero->yaw);
                if (tesla_target_max_hero_angle_diff < score)
                    continue;
                score = ((tesla_target_max_camera_angle_diff - length_w3) / tesla_target_max_camera_angle_diff) *
                            tesla_target_angle_score_weight +
                        (tesla_beam_length - distance_w16) / tesla_beam_length;
                if (best_score < score) {
                    second_score = best_score;
                    second = best;
                    best_score = score;
                    best = candidate;
                } else if (second_score < score) {
                    second_score = score;
                    second = candidate;
                }
            next_candidate:;
            }
        }
    }
    if (best) {
        if (v->target)
            second = best;
        else {
            v->target = best;
            v->target_timer = 0;
            Moby_PlaySound(1, 0, o);
        }
    }
    if (second && enabled) {
        distance_w17 = fast_vec_dist(v, &best->position);
        if (distance_w17 < tesla_beam_length) {
            angle = fast_vec_dist(&best->position, &second->position);
            if (distance_w17 + angle < tesla_beam_length) {
                v->second_target = second;
                v->second_timer = 0;
            }
        }
    }
    primary_distance = 0.0f;
    if (v->target) {
        data = (char *)GetFlaggedPvarWord(v->target);
        qcopy(&target0, &v->target->position);
        target0.f[2] += *(float *)(data + 0x10);
        primary_distance = fast_vec_dist(v, &target0);
        spacing = primary_distance * 0.05f;
    }
    if (v->second_target) {
        data = (char *)GetFlaggedPvarWord(v->second_target);
        qcopy(&target1, &v->second_target->position);
        target1.f[2] += *(float *)(data + 0x10);
        spacing = (fast_vec_dist(&target0, &target1) + primary_distance) * 0.05f;
    }
    if (enabled)
        (beam_arc_outer_color) = 0x207f20;
    else
        (beam_arc_outer_color) = 0x7f2020;
    clear_u64_value(&camera_direction);
    InitDirectionalHitQuery_v(&hit, o, 0x210000, &camera_direction, enabled + 2.0f);
    hit.kind = 5;
    hit.mode = 1;
    hit.id = ((Moby *)o)->oclass;
    v->count3e = 20;
    player = &hero43;
    coll_sphere_mobys_v(v, 0x10, player->moby, &hit, 0.3f);
    FastVecSub(&direction, &v->tip, v);
    vec_scale_to_length(&direction, &direction, 1.0f);
    if (v->target || v->second_target) {
        FastVecCross(&delta, &direction, &player->direction);
        build_look_at_matrix(&direction, &direction, &delta, 0.17453292f);
    }
    hit_result = 0;
    collided_count = 0;
    FastVecScale(&step, &direction, spacing);
    qcopy(tesla_beam_points, v);
    qcopy(tesla_beam_points + 0x140, v);
    beam = (Vec4 *)tesla_beam_points;
    side_waves = (float *)tesla_beam_point_state_b;
    for (seg = 1; seg < 20; seg++) {
        previous = (char *)&beam[seg - 1];
        side = (char *)&beam[seg + 20];
        n = (float)seg;
        factor = tesla_beam_wave_amplitude_start +
                 (tesla_beam_wave_amplitude_end - tesla_beam_wave_amplitude_start) * (IntToFloat(seg) * 0.05f);
        phase = normalize_angle(tesla_beam_reset_short_a + n * tesla_beam_wave_phase_step);
        wave = factor * FastSin(phase);
        wave += randf_sym(tesla_beam_wave_jitter_min, tesla_beam_wave_jitter_max);

        beam[seg].f[2] -= ((float *)tesla_beam_point_state_a)[seg];
        ((float *)tesla_beam_point_state_a)[seg] = wave;
        FastVecSub(&delta, &beam[seg], (char *)tesla_beam_points - 16 + seg * 16);
        lerp_vector(&blend, &delta, &step, 0.85f);
        {
            float blend_length = FastVecLength(&blend);
            if (blend_length == 0.0f)
                qcopy(&beam[seg], previous);
            else {
                FastVecScale(&blend, &blend, spacing / blend_length);
                FastVecAdd(&beam[seg], (char *)tesla_beam_points - 16 + seg * 16, &blend);
                qcopy(&step, &blend);
            }
        }
        beam[seg].f[2] += wave;
        FastVecSub(&delta, &beam[seg], (char *)tesla_beam_points - 16 + seg * 16);
        if (v->target) {
            if (!enabled || !v->second_target) {
                FastVecSub(&target_delta, &target0, &beam[seg]);
                length_w4 = FastVecLength(&target_delta);
                if (length_w4 != 0.0f) {
                    if (seg == 19) {
                        factor_w13 = 0.5f;
                        spacing = length_w4;
                    } else if (seg >= 16) {
                        factor_w13 = 0.5f;
                        spacing = length_w4 / IntToFloat(19 - seg);
                    } else
                        factor_w13 = 0.1f;
                    FastVecScale(&target_delta, &target_delta, spacing / length_w4);
                    lerp_vector(&step, &step, &target_delta, factor_w13);
                }
            } else {
                if (seg >= 10)
                    FastVecSub(&target_delta, &target1, &beam[seg]);
                else
                    FastVecSub(&target_delta, &target0, &beam[seg]);
                length_w4 = FastVecLength(&target_delta);
                if (length_w4 != 0.0f) {
                    if (seg == 9 || seg == 19) {
                        factor_w13 = 0.7f;
                        spacing = length_w4;
                    } else if ((unsigned)(seg - 5) < 5) {
                        factor_w13 = 0.7f;
                        spacing = length_w4 / IntToFloat(9 - seg);
                    } else if (seg > 14) {
                        factor_w13 = 0.7f;
                        spacing = length_w4 / IntToFloat(19 - seg);
                    } else
                        factor_w13 = 0.2f;
                    FastVecScale(&target_delta, &target_delta, spacing / length_w4);
                    lerp_vector(&step, &step, &target_delta, factor_w13);
                }
            }
        }
        if (seg < 10) {
            Vec4 *nz, *dst;
            side_phase = normalize_angle(tesla_beam_reset_short_b + (float)seg * tesla_beam_side_phase_step);
            angle = normalize_angle(tesla_beam_reset_short_c + (float)seg * tesla_beam_side_amplitude_phase_step);
            factor = tesla_beam_side_wave_amplitude * FastSin(angle);
            qcopy(side, &beam[seg]);
            x_s = FastSin(side_phase);
            side_wave = randf_sym(0.0f, 0.1f);
            x_c = factor * x_s + side_wave;
            side_waves[seg] = x_c;
            *(float *)((char *)&beam[seg] + 0x148) += x_c;
            x_a = FastAddRots(fast_atan2(delta.f[0], delta.f[1]), 1.5707964f);
            x_m = tesla_beam_side_offset_scale * FastCos(side_phase);
            offset.f[0] = FastCos(x_a) * x_m;
            offset.f[1] = FastSin(x_a) * x_m;
            dst = &tesla_beam_point_vectors[seg];
            nz = &offset;
            offset.f[2] = 0.0f;
            qcopy_nc(dst, nz);
            FastVecAdd(side, side, nz);
        } else {
            mirror = 21 - seg;
            qcopy(side, &beam[seg]);
            *(float *)((char *)&beam[seg] + 0x148) += side_waves[mirror];
            FastVecAdd(side, side, &tesla_beam_point_vectors[mirror]);
            qcopy(&offset, &tesla_beam_point_vectors[seg]);
        }
        if (seg == 1) {
            Hero43 *h = &hero43;
            FastVecScale(&effects.first.origin, (char *)h->moby + 0xe0, 0.5f);
            FastVecAdd(&target_delta, &h->position, &effects.first.origin);
            hit_result = coll_line((Vec4f *)&target_delta, (Vec4f *)v, 0x10, h->moby, 0);
        } else if (seg == 19 || !(seg & 1)) {
            hit_result =
                coll_line((Vec4f *)((char *)tesla_beam_points - 32 + seg * 16), (Vec4f *)&beam[seg], 0x10, o, 0);
        } else
            continue;
        FastVecSub(&camera_direction, &beam[seg], g_debug_camera);
        camera_direction.f[2] = 0.0f;
        length = FastVecLength(&camera_direction);
        if (length == 0.0f)
            camera_direction.f[2] = 1.0f;
        else
            FastVecScale(&camera_direction, &camera_direction, 1.0f / length);
        camera_direction.f[3] = 5627.9248046875f;
        qcopy(&hit.direction, &camera_direction);
        if (hit_result) {
            candidate = (M43 *)(*(int *)((char *)&collision_hit_point - 8));
            if (candidate) {
                if (!GetFlaggedPvarWord(candidate) || !candidate->cls || *(short *)((char *)candidate->cls + 0x46) != 5)
                    record_moby_hit(candidate, &hit);
                else {
                    hit_result = 0;
                    collided[collided_count++] = candidate;
                    candidate->collision = 0;
                }
            }
            if (hit_result) {
                qcopy(&beam[seg], &collision_hit_point);
                v->count3e = seg;
                qcopy(&v->tip, &collision_hit_point);
                if (!(*(int *)((char *)&collision_hit_point - 8)) ||
                    test_moby_linked_record_word8((*(int *)((char *)&collision_hit_point - 8)))) {
                    v->target = 0;
                    v->target_timer = 0;
                }
                break;
            }
        }
    }
    for (k = 0; k < collided_count; k++)
        collided[k]->collision = *(int *)((char *)collided[k]->cls + 0x10);
    if (!hit_result)
        qcopy(&v->tip, tesla_beam_points + 0x130);
    if (v->target && !(*(unsigned long *)&v->target_timer & 0xffff0000ffffUL)) {
        FastVecSub(&target_delta, &v->target->position, g_debug_camera);
        target_delta.f[2] = 0.0f;
        vec_scale_to_length(&target_delta, &target_delta, 1.0f);
        target_delta.f[3] = 5627.9248046875f;
        InitDirectionalHitQuery_v(&effects.first.hit0, o, 0x210000, &target_delta, enabled + 2.0f);
        effects.first.hit0.kind = 5;
        effects.first.hit0.mode = 3;
        effects.first.hit0.id = ((Moby *)o)->oclass;
        record_moby_hit(v->target, &effects.first.hit0);
        {
            char *aim;
            int frames = 5;
            aim = (char *)GetFlaggedPvarWord(v->target);
            if (aim && *(short *)(aim + 4) == 1)
                frames = 1;
            v->unk44 = scale_ticks(frames);
        }
    }
    if (v->second_target && !(*(unsigned long *)&v->target_timer & 0xffff0000ffff0000UL)) {
        FastVecSub(&target_delta, &v->second_target->position, g_debug_camera);
        target_delta.f[2] = 0.0f;
        vec_scale_to_length(&target_delta, &target_delta, 1.0f);
        target_delta.f[3] = 5627.9248046875f;
        InitDirectionalHitQuery_v(&effects.hit1, o, 0x210000, &target_delta, enabled + 2.0f);
        effects.hit1.kind = 5;
        effects.hit1.mode = 3;
        effects.hit1.id = ((Moby *)o)->oclass;
        record_moby_hit(v->second_target, &effects.hit1);
        {
            char *aim;
            int frames = 5;
            aim = (char *)GetFlaggedPvarWord(v->second_target);
            if (aim && *(short *)(aim + 4) == 1)
                frames = 1;
            v->unk46 = scale_ticks(frames);
        }
    }
    for (i = 0; i < 4; i++) {
        Vec4 *rows;
        if (i < 2) {
            qcopy(&effects.first.origin, v);
            factor_w15 = 0.4f;
        } else {
            qcopy(&effects.first.origin, &v->tip);
            factor_w15 = 0.5f;
        }
        if (FastDecTimer(&beam_arc_bolt_slots[i])) {
            beam_arc_bolt_slots[i] = scale_ticks(beam_arc_bolt_params[i]);
            ((short *)&beam_arc_bolt0_alpha)[i] = 0;
            angle = randf_sym(0.5235988f, 1.0471976f);
            phase = random_float_between(-1.0471976f, 0.17453292f);
            FastVecScale(&target_delta, &direction, factor_w15);
            rows = (Vec4 *)&hero43.row1;
            build_look_at_matrix(&target_delta, &target_delta, rows, phase);
            build_look_at_matrix(&target_delta, &target_delta, rows + 1, angle);
            qcopy(&arcs_lb[i][0], &effects.first.origin);
            FastVecAdd(&arcs_lb[i][1], &effects.first.origin, &target_delta);
            sign = 1.0f;
            for (m = 2; m < 5; m++) {
                FastVecSub(&effects.first.hit0, &arcs_lb[i][-1] + m, g_debug_camera);
                phase = random_float_between(0.17453292f, 0.9599311f) * sign;
                sign = -sign;
                build_look_at_matrix(&target_delta, &target_delta, &effects.first.hit0, phase);
                FastVecAdd(&arcs_lb[i][m], &arcs_lb[i][-1] + m, &target_delta);
            }
        } else {
            int d, n0;
            for (j = 1; j < 5; j++) {
                FastVecSub(&differences[j], &arcs_lb[i][j], &arcs_lb[i][-1] + j);
                differences[j].f[0] += randf_sym(0.0f, 0.1f);
                differences[j].f[1] += randf_sym(0.0f, 0.1f);
                differences[j].f[2] += randf_sym(0.0f, 0.1f);
            }
            qcopy(&arcs_lb[i][0], &effects.first.origin);
            for (j = 1; j < 5; j++)
                FastVecAdd(&arcs_lb[i][j], &arcs_lb[i][-1] + j, &differences[j]);
            d = beam_arc_bolt_params_4[i];
            n0 = beam_arc_bolt_slots[i];
            if (d < n0)
                d = n0 - d;
            else
                d = d - n0;
            distance = IntToFloat(d) / IntToFloat(scale_ticks(15));
            ((short *)&beam_arc_bolt0_alpha)[i] = truncate_float_to_s32((1.0f - distance) * 32.0f);
        }
    }
    angle = randf_sym(0.5235988f, 1.0471976f);
    phase = random_float_between(-0.5235988f, 1.0471976f);
    FastVecScale(&target_delta, &direction, D_L00_001616F8 * frame_scale);
    build_look_at_matrix(&target_delta, &target_delta, &hero43.row1, phase);
    build_look_at_matrix(&target_delta, &target_delta, &hero43.row2, angle);
    spin = random_int_between(1, 8);
    if (random_integer_below(2))
        spin = -spin;
    amplitude = random_float_between(0.2f, 0.6f);
    PartType53Spawn(v, scale_ticks(15), amplitude * 0.3f, amplitude, (beam_arc_outer_color) | 0x7f000000, 0, spin,
                    (float *)&target_delta, 0.0f);
    spin = -spin + (*(unsigned char *)v >> 8) + (((unsigned char *)v)[1] >> 8);
    PartType53Spawn(v, scale_ticks(15), amplitude * 0.15f, amplitude * 0.5f, 0x307f7f7f, 0, spin,
                    (float *)&target_delta, 0.0f);
    spin = random_int_between(1, 8);
    if (random_integer_below(2))
        spin = -spin;
    amplitude = random_float_between(0.15f, 3.0f);
    {
        /* FAKEMATCH: a never-taken branch (cprop proves dead == 0, then jump deletes it). Retail
           had about 30 more insns before gcse that vanish later; they size the PRE hash table,
           which orders the &target_delta and &effects spill slots. */
        int dead = 0;
        if (amplitude < 0.0f)
            dead = 0;
        if (dead) {
            PartType53Spawn(v, scale_ticks(15), amplitude, amplitude, 0, 0, spin, (float *)&target_delta, 0.0f);
            PartType53Spawn(v, scale_ticks(15), amplitude, amplitude, 0, 0, spin, (float *)&target_delta, 0.0f);
            PartType53Spawn(v, scale_ticks(15), amplitude, amplitude, 0, 0, spin, (float *)&target_delta, 0.0f);
        }
    }
    PartType53Spawn(&v->tip, scale_ticks(12), amplitude * 0.1f, amplitude, (beam_arc_outer_color) | 0x7f000000, 0, spin,
                    (float *)veldin1_zero_vec, 0.0f);
}

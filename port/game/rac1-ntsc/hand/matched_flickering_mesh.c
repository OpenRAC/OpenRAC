/* SPDX-License-Identifier: GPL-3.0-or-later */
/* FUN_L08_002f5e58 (draw_flickering_mesh_effect), Batalia's animated grid effect. As the port
 * runs it: the NTSC decomp's matching C (src/overlays/l08/mobys/flickering_mesh_effect.c) with its
 * three variable-length arrays (the grid of (ny+1) x (nx+1) points, the u and v rows) taken from
 * the game stack with alloca instead, which hostgen translates and a variable-length array it does
 * not. The space is the same, kept until the function returns rather than until its block ends,
 * which here is the same point. Otherwise the matching C, line for line; it goes at the end of
 * flickering_mesh_effect.c, whose declarations give the name its address. Not a PS2 match; the
 * decomp's C stays as it is (hostgen.json "overrides": this C replaces it in the port only). PAL's
 * func_L08_002F7318 has the same override (port/game/rac1-pal/hand/matched_flickering_mesh.c). */
void draw_flickering_mesh_effect(struct Moby *moby) {
    Vec4              corner[4], step, left_step, right_step, left, right, point, edge, normal;
    float             matrix[16];
    FlickerQuad       quad;
    Vec4              sv0, sv1, sv2, sv3, sv4;
    float             fraction;
    FlickerLightVars *flickering_mesh_vars;
    if (!moby || !(flickering_mesh_vars = (FlickerLightVars *)moby->pvars))
        return;
    {
        int            row_bytes = (flickering_mesh_vars->nx + 1) * sizeof(Vec4);
        unsigned char *points =
            (unsigned char *)__builtin_alloca((flickering_mesh_vars->ny + 1) * row_bytes);
        int            remaining;
        float          inv_x, inv_y;
        inv_x = 1.0f / convert_integer_to_float(flickering_mesh_vars->nx);
        inv_y = 1.0f / convert_integer_to_float(flickering_mesh_vars->ny);
        {
            float *u = (float *)__builtin_alloca((flickering_mesh_vars->nx + 1) * sizeof(float));
            float *v = (float *)__builtin_alloca((flickering_mesh_vars->ny + 1) * sizeof(float));
            float        phase, phase_step, amp, amp_step, amp2, amp2_step;
            float        phase0_step, phase1_step, time, z0, z1, bias;
            int          alpha, frame;
            float        row_noise, phase0, phase1;
            int          i, j;
            unsigned int color;
            time = convert_integer_to_float(l08_effect_animation_time);
            for (remaining = 0; remaining < 8; remaining++) {
                float *goal;
                if (flickering_mesh_vars->noise[remaining] <
                    *(goal = &flickering_mesh_vars->goal[remaining])) {
                    flickering_mesh_vars->noise[remaining] +=
                        flickering_mesh_vars->jitter * flickering_mesh_vars->noise_rate;
                    if (!(*goal <= flickering_mesh_vars->noise[remaining]))
                        continue;
                } else {
                    flickering_mesh_vars->noise[remaining] -=
                        flickering_mesh_vars->jitter * flickering_mesh_vars->noise_rate;
                    if (!(flickering_mesh_vars->noise[remaining] <= *goal))
                        continue;
                }
                *goal = random_float_between(1.0f - flickering_mesh_vars->jitter, 1.0f);
            }
            alpha = truncate_float_to_s32(flickering_mesh_vars->intensity *
                                          convert_integer_to_float(flickering_mesh_vars->alpha) *
                                          convert_integer_to_float(moby->alpha) * 0.0078125f) &
                    0xFF;
            if (flickering_mesh_vars->frames) {
                frame = l08_effect_animation_time;
                frame /= flickering_mesh_vars->frame_ticks + 1;
                flickering_mesh_vars->current =
                    flickering_mesh_vars->texture + frame % flickering_mesh_vars->frames;
            } else
                flickering_mesh_vars->current = flickering_mesh_vars->texture;
            if (flickering_mesh_vars->flags & 1)
                quad.gs[1] = get_effect_texture(flickering_mesh_vars->current);
            else
                quad.gs[1] = get_effect_texture(flickering_mesh_vars->current + 0x28);
            quad.gs[0] = 0;
            quad.gs[2] = 0xFF9000000260L;
            quad.gs[3] = (long)flickering_mesh_vars->blend0 |
                         ((long)flickering_mesh_vars->blend1 << 2) |
                         ((long)flickering_mesh_vars->blend2 << 4) |
                         ((long)flickering_mesh_vars->blend3 << 6) |
                         ((long)flickering_mesh_vars->alpha << 32);
            color = (flickering_mesh_vars->alpha << 24) | (flickering_mesh_vars->blue << 16) |
                    (flickering_mesh_vars->green << 8) | flickering_mesh_vars->red;
            quad.color[0] = color;
            quad.color[1] = color;
            quad.color[2] = color;
            quad.color[3] = color;
            qzero(&corner[0]);
            qzero(&corner[1]);
            qzero(&corner[2]);
            qzero(&corner[3]);
            {
                float half_width = flickering_mesh_vars->size_x * 0.5f;
                corner[1].f[0] = corner[3].f[0] = half_width;
                corner[0].f[0] = corner[2].f[0] = -half_width;
            }
            corner[0].f[2] = flickering_mesh_vars->z0 + 0.01f;
            corner[1].f[2] = flickering_mesh_vars->z1 + 0.01f;
            euler_rotation_matrix(matrix, &moby->rot);
            transform_vector(&corner[0], &corner[0], matrix);
            transform_vector(&corner[1], &corner[1], matrix);
            transform_vector(&corner[2], &corner[2], matrix);
            transform_vector(&corner[3], &corner[3], matrix);
            fast_vec_sub(&left_step, &corner[2], &corner[0]);
            fast_vec_scale(&left_step, &left_step, inv_y);
            fast_vec_sub(&right_step, &corner[3], &corner[1]);
            fast_vec_scale(&right_step, &right_step, inv_y);
            phase_step =
                (flickering_mesh_vars->phase_end - flickering_mesh_vars->phase_start) * inv_y;
            amp_step = (flickering_mesh_vars->amp_end - flickering_mesh_vars->amp_start) * inv_y;
            amp2_step = (flickering_mesh_vars->amp2_end - flickering_mesh_vars->amp2_start) * inv_y;
            phase0_step = (flickering_mesh_vars->phase0_end - flickering_mesh_vars->phase0) * inv_x;
            phase1_step = (flickering_mesh_vars->phase1_end - flickering_mesh_vars->phase1) * inv_y;
            z0 = flickering_mesh_vars->size_y - flickering_mesh_vars->z0;
            z1 = flickering_mesh_vars->size_y - flickering_mesh_vars->z1;
            fast_vec_sub(&edge, &corner[1], &corner[0]);
            fast_vec_cross(&normal, &edge, &left_step);
            fast_vec_normalize(&normal, &normal, 1.0f);
            qcopy(&left, &corner[0]);
            qcopy(&right, &corner[1]);
            bias = flickering_mesh_vars->noise[0];
            phase = flickering_mesh_vars->phase_start;
            amp = flickering_mesh_vars->amp_start;
            amp2 = flickering_mesh_vars->amp2_start;
            phase0 = flickering_mesh_vars->phase0;
            phase1 = flickering_mesh_vars->phase1;
            {
                /* Fakematch: 30 unused locals before i, so i+1 hashes after &data->puff_timer. */
                int pad0, pad1, pad2, pad3, pad4, pad5, pad6, pad7, pad8, pad9, pad10, pad11, pad12,
                    pad13, pad14, pad15, pad16, pad17, pad18, pad19, pad20, pad21, pad22, pad23,
                    pad24, pad25, pad26, pad27, pad28, pad29;
                int          i;
                register int j;
                for (i = 0; i <= flickering_mesh_vars->ny; phase += phase_step,
                    amp += amp_step,
                    amp2 += amp2_step,
                    phase0 += phase0_step,
                    phase1 += phase1_step,
                    i++) {
                    float y = convert_integer_to_float(i) /
                              convert_integer_to_float(flickering_mesh_vars->ny);
                    qcopy_nc(&point, &left);
                    j = 0;
                    fast_vec_sub(&step, &right, &left);
                    fast_vec_scale(&step, &step, inv_x);
                    row_noise = interpolate_sample_ring_data_first((char *)flickering_mesh_vars, y);
                    for (; j <= flickering_mesh_vars->nx; j++) {
                        float x = convert_integer_to_float(j) /
                                  convert_integer_to_float(flickering_mesh_vars->nx);
                        float theta;
                        float wave =
                            time * phase +
                            flickering_mesh_vars->noise_amp *
                                fast_sin(normalize_angle(flickering_mesh_vars->noise_speed * time));
                        float noise =
                            interpolate_sample_ring_data_first((char *)flickering_mesh_vars, x);
                        float angle0, angle1, shape, drop, a, b, normal_angle0, normal_angle1;
                        drop = z0 * x + z1 * (1.0f - x);
                        theta = x * 3.1415927f;
                        wave += noise * flickering_mesh_vars->noise_u +
                                row_noise * flickering_mesh_vars->noise_v +
                                bias * flickering_mesh_vars->noise_bias;
                        angle0 = normalize_angle(wave + phase0 + flickering_mesh_vars->wave_u * x +
                                                 flickering_mesh_vars->wave2_v * y);
                        angle1 = normalize_angle(wave + phase1 + flickering_mesh_vars->wave_v * y +
                                                 flickering_mesh_vars->wave2_u * x);
                        shape = fast_sin(theta);
                        shape = square_root(shape);
                        drop *= shape;
                        drop *= 1.0f - y;
                        fast_vec_normalize(&sv0, &step, amp * fast_cos(angle0) * shape * noise);
                        fast_vec_normalize(
                            &sv1, &left_step, amp2 * fast_sin(angle1) * shape * noise);
                        a = fast_sin(y * 3.1415927f);
                        b = fast_sin(theta);
                        normal_angle0 =
                            fast_sin(normalize_angle(wave + flickering_mesh_vars->wave_u * x));
                        normal_angle1 =
                            fast_sin(normalize_angle(wave + flickering_mesh_vars->wave2_v * y));
                        fast_vec_normalize(&sv3,
                                           &normal,
                                           a * b *
                                               (flickering_mesh_vars->normal_u * normal_angle0 +
                                                flickering_mesh_vars->normal_v * normal_angle1));
                        fast_vec_normalize(&sv4, &left_step, drop);
                        fast_vec_add(&sv2, &sv0, &sv1);
                        fast_vec_add(&sv2, &sv2, &sv3);
                        fast_vec_sub(&sv2, &sv2, &sv4);
                        fast_vec_add((Vec4 *)(points + i * row_bytes) + j, &point, &sv2);
                        *(float *)(points + (i * row_bytes + j * sizeof(Vec4)) + 12) =
                            1.0f / ((1.0f - flickering_mesh_vars->alpha_u +
                                     flickering_mesh_vars->alpha_u * shape * shape * shape) *
                                    (1.0f - flickering_mesh_vars->alpha_v +
                                     flickering_mesh_vars->alpha_v * y));
                        fast_vec_add(&point, &point, &step);
                    }
                    fast_vec_add(&left, &left, &left_step);
                    fast_vec_add(&right, &right, &right_step);
                }
            }
            {
                int   i;
                float f = split_float_fraction(
                    &fraction,
                    time * flickering_mesh_vars->u_rate +
                        flickering_mesh_vars->u_amp *
                            fast_sin(normalize_angle(time * flickering_mesh_vars->u_phase)));
                for (i = 0; i <= flickering_mesh_vars->nx; i++) {
                    u[i] = f;
                    f += inv_x / flickering_mesh_vars->texture_w_count;
                }
                f = split_float_fraction(
                    &fraction,
                    time * flickering_mesh_vars->v_rate +
                        flickering_mesh_vars->v_amp *
                            fast_sin(normalize_angle(time * flickering_mesh_vars->v_phase)));
                for (i = 0; i <= flickering_mesh_vars->ny; i++) {
                    v[i] = f;
                    f += inv_y / flickering_mesh_vars->texture_h_count;
                }
            }
            {
                int i;
                for (i = 0; i < flickering_mesh_vars->ny; i++) {
                    int j;
                    for (j = 0; j < flickering_mesh_vars->nx; j++) {
                        int          j1 = j + 1;
                        int          i1 = i + 1;
                        float       *vi = &v[i], *vi1 = &v[i1];
                        FlickerQuad *mesh_quad = &quad;
                        int          row_offset = i * row_bytes, next_offset = i1 * row_bytes;
                        Vec4        *row = (Vec4 *)(points + row_offset),
                             *next_row = (Vec4 *)(points + next_offset);
                        quad.uv[0] = u[j];
                        quad.uv[1] = *vi;
                        quad.uv[2] = u[j1];
                        quad.uv[3] = *vi;
                        quad.uv[4] = u[j];
                        quad.uv[5] = *vi1;
                        quad.uv[6] = u[j1];
                        quad.uv[7] = *vi1;
                        fast_vec_add(&mesh_quad->vertex[0], &row[j], &moby->pos);
                        fast_vec_add(&quad.vertex[1], &row[j + 1], &moby->pos);
                        fast_vec_add(&quad.vertex[2], &next_row[j], &moby->pos);
                        fast_vec_add(&quad.vertex[3], &next_row[j + 1], &moby->pos);
                        quad.color[0] =
                            ((alpha /
                              truncate_float_to_s32(
                                  *(float *)(points + (row_offset + (j) * sizeof(Vec4)) + 12)))
                             << 24) |
                            (flickering_mesh_vars->blue << 16) |
                            (flickering_mesh_vars->green << 8) | flickering_mesh_vars->red;
                        quad.color[1] =
                            ((alpha / truncate_float_to_s32(*(
                                          float *)(points + (row_offset + j1 * sizeof(Vec4)) + 12)))
                             << 24) |
                            (flickering_mesh_vars->blue << 16) |
                            (flickering_mesh_vars->green << 8) | flickering_mesh_vars->red;
                        quad.color[2] =
                            ((alpha /
                              truncate_float_to_s32(
                                  *(float *)(points + (next_offset + (j) * sizeof(Vec4)) + 12)))
                             << 24) |
                            (flickering_mesh_vars->blue << 16) |
                            (flickering_mesh_vars->green << 8) | flickering_mesh_vars->red;
                        quad.color[3] =
                            ((alpha /
                              truncate_float_to_s32(
                                  *(float *)(points + (next_offset + j1 * sizeof(Vec4)) + 12)))
                             << 24) |
                            (flickering_mesh_vars->blue << 16) |
                            (flickering_mesh_vars->green << 8) | flickering_mesh_vars->red;
                        draw_geometry_quad(mesh_quad, 0, 0);
                    }
                }
            }
            {
                /* Fakematch: a never-taken branch (the cmov folds at combine, jump2 deletes it).
               Retail had about 50 more insns at gcse time that vanish later; they size the PRE
               expression hash table (n_insns/2|1 = 583 buckets), whose walk order numbers the
               PRE pseudos and so orders the spill slots. 42 one-insn calls (41..44 equal). */
                int dead = 0;
                if (flickering_mesh_vars->flags & 0x8000)
                    dead = 0;
                if (dead) {
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                    font_queue_vu_state();
                }
            }
            if (flickering_mesh_vars->flags & 2) {
                if (fast_dec_timer(&flickering_mesh_vars->particle_timer)) {
                    flickering_mesh_vars->particle_timer =
                        scale_game_frames(flickering_mesh_vars->particle_ticks);
                    for (i = 0; i < flickering_mesh_vars->particle_count; i++) {
                        float width = flickering_mesh_vars->size_x * 0.5f;
                        float x = random_float_between(-1.0f, 1.0f) *
                                  flickering_mesh_vars->particle_range;
                        float fraction = random_float_between(0.0f, x);
                        float size;
                        short life;
                        int   r, g, b, a;
                        Vec4 *particle_motion = &sv0;
                        fast_vec_normalize(&sv2,
                                           &left_step,
                                           -(flickering_mesh_vars->particle_offset +
                                             square_root(fast_cos(fraction * 3.1415927f)) *
                                                 flickering_mesh_vars->particle_height));
                        fast_vec_normalize(&sv1, &step, fraction * width);
                        fast_vec_add(&sv1, &sv1, &moby->pos);
                        fast_vec_add(&sv1, &sv1, &sv2);
                        sv0.f[0] = random_float_between(-flickering_mesh_vars->particle_xy,
                                                        flickering_mesh_vars->particle_xy) *
                                   frame_scale;
                        sv0.f[1] = random_float_between(-flickering_mesh_vars->particle_xy,
                                                        flickering_mesh_vars->particle_xy) *
                                   frame_scale;
                        sv0.f[2] = random_float_between(0.0f, flickering_mesh_vars->particle_z) *
                                   frame_scale;
                        size = random_float_between(flickering_mesh_vars->particle_size0,
                                                    flickering_mesh_vars->particle_size1);
                        life = scale_game_frames(flickering_mesh_vars->particle_life);
                        r = flickering_mesh_vars->particle_r;
                        g = flickering_mesh_vars->particle_g;
                        b = flickering_mesh_vars->particle_b;
                        a = truncate_float_to_s32(
                            convert_integer_to_float(flickering_mesh_vars->particle_a) * fraction);
                        spawn_colored_effect_size_first(
                            moby, &sv1, particle_motion, size, life, r, g, b, a);
                    }
                }
                if (fast_dec_timer_s16(&flickering_mesh_vars->puff_timer)) {
                    flickering_mesh_vars->puff_timer =
                        scale_game_frames(flickering_mesh_vars->puff_ticks);
                    for (i = 0; i < flickering_mesh_vars->puff_count; i++) {
                        int puff_spawn_color = (flickering_mesh_vars->puff_c0[3] << 24) |
                                               (flickering_mesh_vars->puff_c0[2] << 16) |
                                               (flickering_mesh_vars->puff_c0[1] << 8) |
                                               flickering_mesh_vars->puff_c0[0];
                        int puff_payload_color = (flickering_mesh_vars->puff_c1[3] << 24) |
                                                 (flickering_mesh_vars->puff_c1[2] << 16) |
                                                 (flickering_mesh_vars->puff_c1[1] << 8) |
                                                 flickering_mesh_vars->puff_c1[0];
                        float width = flickering_mesh_vars->size_x * 0.5f;
                        float size = random_float_between(flickering_mesh_vars->puff_size0,
                                                          flickering_mesh_vars->puff_size1);
                        int   life = scale_game_frames(random_int_between(
                            flickering_mesh_vars->puff_life0, flickering_mesh_vars->puff_life1));
                        float fraction =
                            random_float_between(-1.0f, 1.0f) * flickering_mesh_vars->puff_range;
                        float shape;
                        fraction = random_float_between(-fraction, fraction);
                        shape = square_root(fast_cos(fraction * 3.1415927f));
                        fast_vec_normalize(&sv2,
                                           &left_step,
                                           -(flickering_mesh_vars->puff_offset +
                                             shape * flickering_mesh_vars->puff_height));
                        fast_vec_normalize(&sv1, &step, fraction * width);
                        fast_vec_add(&sv1, &sv1, &moby->pos);
                        fast_vec_add(&sv1, &sv1, &sv2);
                        sv0.f[0] = random_float_between(-flickering_mesh_vars->puff_xy,
                                                        flickering_mesh_vars->puff_xy) *
                                   frame_scale;
                        sv0.f[1] = random_float_between(-flickering_mesh_vars->puff_xy,
                                                        flickering_mesh_vars->puff_xy) *
                                   frame_scale;
                        sv0.f[2] =
                            random_float_between(0.0f, flickering_mesh_vars->puff_z) * frame_scale;
                        fast_vec_scale(&sv0,
                                       &sv0,
                                       1.0f - flickering_mesh_vars->puff_blend +
                                           shape * flickering_mesh_vars->puff_blend);
                        spawn_effect_particle_with_target_scalar(
                            &sv1, &sv0, puff_spawn_color, puff_payload_color, size, life);
                    }
                }
                if (flickering_mesh_vars->light != -1) {
                    PointLight *light = &l08_point_light_slots[flickering_mesh_vars->light];
                    if (fast_vec_dist(&flickering_mesh_vars->light_pos, &light->pos) <
                        flickering_mesh_vars->light_speed * frame_scale) {
                        float x = (random_float_between(-(int)flickering_mesh_vars->light_range_x,
                                                        flickering_mesh_vars->light_range_x) +
                                   flickering_mesh_vars->size_x) *
                                  0.5f;
                        float y = (random_float_between(-(int)flickering_mesh_vars->light_range_y,
                                                        flickering_mesh_vars->light_range_y) +
                                   flickering_mesh_vars->size_y) *
                                  0.5f;
                        fast_vec_normalize(&sv0, &step, x);
                        fast_vec_normalize(&sv1, &left_step, -y);
                        fast_vec_add(&flickering_mesh_vars->light_pos, &sv0, &sv1);
                        fast_vec_add(&flickering_mesh_vars->light_pos,
                                     &flickering_mesh_vars->light_pos,
                                     &corner[0]);
                        fast_vec_add(&flickering_mesh_vars->light_pos,
                                     &flickering_mesh_vars->light_pos,
                                     &moby->pos);
                    } else {
                        fast_vec_sub(&sv0, &flickering_mesh_vars->light_pos, &light->pos);
                        fast_vec_normalize(
                            &sv0, &sv0, flickering_mesh_vars->light_speed * frame_scale);
                        fast_vec_add(&light->pos, &light->pos, &sv0);
                        light->pos.f[3] = convert_integer_to_float(flickering_mesh_vars->radius);
                    }
                }
                flickering_mesh_vars->flags ^= 2;
            }
        }
    }
}

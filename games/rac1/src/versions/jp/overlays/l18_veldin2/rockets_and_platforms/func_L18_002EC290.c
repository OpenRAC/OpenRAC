extern short D_J_L18_001620AC;
void update_electric_bolt_geometry_and_particles(void *moby) {
    V_ec290 s[10];
    V_ec290 d[5];
    unsigned char flag = *(unsigned char *)hero_globals_base;
    char *data = *(char **)((char *)moby + 0x78);
    float scale;
    int i;
    int k;
    float f20;
    int r;

    *(float *)&electric_bolt_wave_phase = FastAddRots(*(float *)&electric_bolt_wave_phase, *(float *)&electric_bolt_wave_phase_speed);
    *(float *)&electric_bolt_twist_phase = FastSubRots(*(float *)&electric_bolt_twist_phase, *(float *)&electric_bolt_twist_phase_speed);
    *(float *)&electric_bolt_swell_phase = FastAddRots(*(float *)&electric_bolt_swell_phase, *(float *)&electric_bolt_swell_phase_speed);
    scale = FastVecDist((char *)moby + 0x10, data + 0x10) * 0.05f;
    if (flag) {
        *(int *)&electric_bolt_color = 0x7F2040;
    } else {
        *(int *)&electric_bolt_color = 0x7F2020;
    }
    FastVecSub(&s[0], data + 0x10, (char *)moby + 0x10);
    vec_scale_to_length(&s[0], &s[0], 1.0f);
    FastVecCross(&s[1], &s[0], hero_gravity_dir);
    build_look_at_matrix(&s[0], &s[0], &s[1], 0.17453292f);
    FastVecScale(&s[3], &s[0], scale);
    qcopy(electric_bolt_points[0], (char *)moby + 0x10);
    qcopy(electric_bolt_points[1], (char *)moby + 0x10);
    for (i = 1; i < 20; i++) {
        float f21;
        float len;
        float len0;
        float a;
        float b;
        f21 = *(float *)&electric_bolt_wave_height_start
              + (*(float *)&electric_bolt_wave_height_end - *(float *)&electric_bolt_wave_height_start) * (IntToFloat(i) * 0.05f);
        f21 = f21 * FastSin(normalize_angle(*(float *)&electric_bolt_wave_phase + (float)i * *(float *)&electric_bolt_wave_phase_step));
        f21 = f21 + randf_sym(*(float *)&primary_bolt_z_jitter_range_start_gp, *(float *)&primary_bolt_z_jitter_range_end_gp);
        electric_bolt_points[0][i].f[2] = electric_bolt_points[0][i].f[2] - electric_bolt_wave_heights[i];
        electric_bolt_wave_heights[i] = f21;
        a = random_angle_radians();
        b = random_angle_radians();
        BuildSphericalOffset(&s[1], scale, a, b);
        s[1].f[2] = s[1].f[2] * 0.5f;
        lerp_vector(&s[2], &s[3], &s[1], *(float *)&electric_bolt_wander_blend);
        len0 = FastVecLength(&s[2]);
        if (len0 == 0.0f) {
            qcopy(&electric_bolt_points[0][i], &electric_bolt_points[0][i - 1]);
        } else {
            FastVecScale(&s[2], &s[2], scale / len0);
            FastVecAdd(&electric_bolt_points[0][i], &electric_bolt_points_base[i], &s[2]);
            qcopy(&s[3], &s[2]);
        }
        electric_bolt_points[0][i].f[2] = electric_bolt_points[0][i].f[2] + f21;
        FastVecSub(&s[1], &electric_bolt_points[0][i], &electric_bolt_points_base[i]);
        FastVecSub(&s[5], data + 0x10, &electric_bolt_points[0][i]);
        len = FastVecLength(&s[5]);
        if (len != 0.0f) {
            float w;
            if (i == 19) {
                w = 0.5f;
                scale = len;
            } else if (i >= 16) {
                w = 0.5f;
                scale = len / IntToFloat(19 - i);
            } else {
                w = 0.1f;
            }
            FastVecScale(&s[5], &s[5], scale / len);
            lerp_vector(&s[3], &s[3], &s[5], w);
        }
        if (i < 10) {
            float g;
            float h;
            float ang;
            float m;
            g = normalize_angle(*(float *)&electric_bolt_twist_phase + (float)i * *(float *)&electric_bolt_twist_phase_step);
            h = *(float *)&electric_bolt_twist_height
                * FastSin(normalize_angle(*(float *)&electric_bolt_swell_phase + (float)i * *(float *)&D_J_L18_001620AC));
            qcopy(&electric_bolt_points[1][i], &electric_bolt_points[0][i]);
            h = h * FastSin(g) + randf_sym(0.0f, 0.1f);
            electric_bolt_points[1][i].f[2] = electric_bolt_points[1][i].f[2] + h;
            electric_bolt_twist_heights[i] = h;
            ang = FastAddRots(fast_atan2(s[1].f[0], s[1].f[1]), 1.5707964f);
            m = *(float *)&electric_bolt_twist_width * FastCos(g);
            s[4].f[0] = FastCos(ang) * m;
            s[4].f[1] = FastSin(ang) * m;
            s[4].f[2] = 0.0f;
            qcopy(&electric_bolt_twist_offsets[i], &s[4]);
            FastVecAdd(&electric_bolt_points[1][i], &electric_bolt_points[1][i], &s[4]);
        } else {
            int r = 21 - i;
            qcopy(&electric_bolt_points[1][i], &electric_bolt_points[0][i]);
            electric_bolt_points[1][i].f[2] = electric_bolt_points[1][i].f[2] + electric_bolt_twist_heights[r];
            FastVecAdd(&electric_bolt_points[1][i], &electric_bolt_points[1][i], &electric_bolt_twist_offsets[r]);
            qcopy(&s[4], &electric_bolt_twist_offsets[i]);
        }
    }
    FastVecCross(&s[6], &s[0], hero_gravity_dir);
    vec_scale_to_length(&s[6], &s[6], 1.0f);
    vec_scale_to_length(&s[7], hero_gravity_dir, 1.0f);
    for (k = 0; k < 4; k++) {
        float t;
        if (k < 2) {
            qcopy(&s[8], (char *)moby + 0x10);
            t = 0.4f;
        } else {
            qcopy(&s[8], data + 0x10);
            t = 0.5f;
        }
        if (FastDecTimer(electric_bolt_branch_timers + k)) {
            int j;
            electric_bolt_branch_timers[k] = scale_ticks(electric_bolt_branch_periods[k]);
            electric_bolt_branch_alpha[k] = 0;
            scale = randf_sym(0.52359879f, 1.04719758f);
            f20 = random_float_between(-1.04719758f, 0.17453292f);
            FastVecScale(&s[5], &s[0], t);
            build_look_at_matrix(&s[5], &s[5], &s[6], f20);
            build_look_at_matrix(&s[5], &s[5], &s[7], scale);
            qcopy(&electric_bolt_branch_points[k].p[0], &s[8]);
            FastVecAdd(&electric_bolt_branch_points[k].p[1], &s[8], &s[5]);
            t = 1.0f;
            for (j = 2; j < 5; j++) {
                FastVecSub(&s[9], &electric_bolt_branch_points_base[k].p[j - 1], veldin2_camera_pos);
                f20 = random_float_between(0.17453292f, 0.959931076f) * t;
                t = -t;
                build_look_at_matrix(&s[5], &s[5], &s[9], f20);
                FastVecAdd((electric_bolt_branch_points[k].p + j), &electric_bolt_branch_points_base[k].p[j - 1], &s[5]);
            }
        } else {
            int j;
            for (j = 1; j < 5; j++) {
                FastVecSub(&d[j], (electric_bolt_branch_points[k].p + j), &electric_bolt_branch_points_base[k].p[j - 1]);
                d[j].f[0] += randf_sym(0.0f, 0.1f);
                d[j].f[1] += randf_sym(0.0f, 0.1f);
                d[j].f[2] += randf_sym(0.0f, 0.1f);
            }
            qcopy(&electric_bolt_branch_points[k].p[0], &s[8]);
            for (j = 1; j < 5; j++) {
                FastVecAdd((electric_bolt_branch_points[k].p + j), &electric_bolt_branch_points_base[k].p[j - 1], &d[j]);
            }
            {
                int x = electric_bolt_branch_peak_ticks[k];
                int y = electric_bolt_branch_timers[k];
                float fr;
                if (x < y) {
                    x = y - x;
                } else {
                    x = x - y;
                }
                fr = IntToFloat(x);
                fr = fr / IntToFloat(scale_ticks(15));
                electric_bolt_branch_alpha[k] = truncate_float_to_s32_r((1.0f - fr) * 32.0f);
            }
        }
    }
    scale = randf_sym(0.52359879f, 1.04719758f);
    f20 = random_float_between(-0.52359879f, 1.04719758f);
    FastVecScale(&s[5], &s[0], *(float *)&electric_bolt_particle_direction_scale_gp * frame_scale);
    build_look_at_matrix(&s[5], &s[5], &s[6], f20);
    build_look_at_matrix(&s[5], &s[5], &s[7], scale);
    r = rand_range(1, 8);
    if (random_integer_below(2)) r = -r;
    f20 = random_float_between(0.2f, 0.6f);
    spawn_ec290((char *)moby + 0x10, scale_ticks(15), f20 * 0.3f, f20, *(int *)&electric_bolt_color | 0x7F000000, 0, r, &s[5], 0.0f);
    r = -r;
    spawn_ec290((char *)moby + 0x10, scale_ticks(15), f20 * 0.15f, f20 * 0.5f, 0x307F7F7F, 0, r, &s[5], 0.0f);
    r = rand_range(1, 8);
    if (random_integer_below(2)) r = -r;
    f20 = random_float_between(0.15f, 3.0f);
    spawn_ec290(data + 0x10, scale_ticks(12), f20 * 0.1f, f20, *(int *)&electric_bolt_color | 0x7F000000, 0, r, veldin2_zero_vec_r, 0.0f);
}

void spawn_explosion(struct Moby *m, void *unused, char *pos, float fA, float fB, int n, int k8, int k9, float fC,
                     float fD, float fE, float fF, int k10, float fG, int k11, int s16, int s18, unsigned char flag) {
    Q_25F4A8 vb;
    Q_25F4A8 v0;
    float dist;
    float f3;
    float t;
    int cnt4;
    int i;

    *(u128_25F4A8 *)&v0 = 0;
    v0.f[2] = g_frame_time_r * 8.0f * fF;
    f3 = level_draw_frame_load + cpu_frame_load;
    *(u128_25F4A8 *)&vb = *(u128_25F4A8 *)&v0;
    if (s18 == -1) {
        s18 = 0;
        if (1.7f < f3)
            s18 = 1;
    }
    if (pos == 0) {
        if (m == 0)
            return;
        pos = (char *)&m->pos;
    }
    {
        if (fA > 0.0f) {
            Ev ev;
            initialize_hit_descriptor_r(&ev, m, 0x810001, fB);
            ev.b18 = 2;
            ev.b19 = 1;
            ev.h1A = *(unsigned short *)&m->oclass;
            coll_sphere_mobys(fA, pos, 0x10, m, &ev);
        }
    }
    for (i = 0; i < n; i++) {
        {
            Q_25F4A8 dir;
            float a;
            float b;
            a = random_float_between(0.2617994f, 1.3962634f);
            b = random_angle_radians();
            BuildSphericalOffset(&dir, fF * (random_float_between(7.0f, 10.5f) * g_frame_time_r), b, a);
            dir.f[2] += g_frame_time_r * 3.0f;
            dist = fF * 40000.0f;
            spawn_shrinking_effect_particle_with_gravity_and_secondary_spawns_v(
                pos, &dir, swap_packed_color_channels_r(0x4F007FFF, flag),
                swap_packed_color_channels_r(0x1F00007F, flag),
                rand_range_r(scale_ticks(0x3C), scale_ticks(0x78)) - s18 * 10, 1, -1, -1, dist);
        }
    }
    FastVecSub(&v0, g_debug_camera, pos);
    dist = FastVecLength(&v0);
    {
        if (s16 != 0) {
            Q_25F4A8 d2;
            Q_25F4A8 rnd;
            int i;
            *(u128_25F4A8 *)&rnd = 0;
            rnd.f[0] = random_float_between(-1.0f, 1.0f);
            rnd.f[1] = random_float_between(-1.0f, 1.0f);
            rnd.f[2] = random_float_between(-1.0f, 1.0f);
            *(u128_25F4A8 *)&d2 = *(u128_25F4A8 *)&rnd;
            if (dist < 14.0f) {
                v0.f[2] += dist * 0.5f;
                vec_scale_to_length(&d2, &d2, dist / 5.0f * g_frame_time_r);
                vec_scale_to_length(&v0, &v0, (dist + dist) * g_frame_time_r);
                FastVecAdd(&d2, &d2, &v0);
                clamp_vec_length(&d2, &d2, g_frame_time_r * 10.0f);
                SpawnFallingSpark_r(pos, &d2, rand_range_r(scale_ticks(0x3C), scale_ticks(0x5A)), 0, 0);
            }
            s16 = s16 - 1;
            for (i = 0; i < s16; i++) {
                float a;
                float b;
                int sel;
                a = random_float_between(0.2617994f, 1.3089969f);
                b = random_angle_radians();
                BuildSphericalOffset(&rnd, fF * (random_float_between(7.0f, 10.5f) * g_frame_time_r), b, a);
                rnd.f[2] += g_frame_time_r + g_frame_time_r;
                sel = 0;
                if (i % 3 == 0)
                    sel = 1;
                SpawnFallingSpark_r(pos, &rnd, rand_range_r(scale_ticks(0x3C), scale_ticks(0x5A)), sel, 0);
            }
        }
    }
    if (dist < IntToFloat(k8) * 2.0f) {
        cnt4 = truncate_float_to_s32_r(dist) / 2;
    } else {
        cnt4 = k8;
    }
    t = 0.0f;
    if (dist < 7.0f)
        t = 7.0f - dist;
    {
        int i;
        for (i = 0; i < cnt4; i++) {
            S6_25F4A8 c0;
            S6_25F4A8 c1;
            float sp;
            sp = random_float_between(8.0f, 10.0f) * g_frame_time_r;
            c0 = color_swapped_burst_colors_r;
            c1 = color_swapped_burst_fade_colors_r;
            sp = sp - t * g_frame_time_r;
            sp = sp * fF;
            PartType11Spawn(pos, &vb, swap_packed_color_channels_r(c0.v[random_integer_below(6)], flag),
                            swap_packed_color_channels_r(c1.v[random_integer_below(6)], flag), fF * 400000.0f,
                            rand_range_r(scale_ticks(0xF), scale_ticks(0x14)) - s18 * 3,
                            rand_range_r(scale_ticks(0x1E), scale_ticks(0x2D)) - s18 * 5, 0, 0, sp);
            PartType11Spawn(pos, &vb, 0x7FFFFFFF, 0xFFFFFF, fF * 400000.0f,
                            rand_range_r(scale_ticks(5), scale_ticks(0xA)) - s18 * 2,
                            rand_range_r(scale_ticks(0xF), scale_ticks(0x14)) - s18 * 3, 0, 0, sp * 0.5f);
        }
    }
    {
        int i;
        for (i = 0; i < k9; i++) {
            S6_25F4A8 c0 = color_swapped_burst_colors_r;
            S6_25F4A8 c1 = color_swapped_burst_fade_colors_r;
            Q_25F4A8 vel;
            Q_25F4A8 rnd;
            int p1;
            int p2;
            *(u128_25F4A8 *)&rnd = 0;
            rnd.f[0] = random_float_between(-1.0f, 1.0f);
            rnd.f[1] = random_float_between(-1.0f, 1.0f);
            rnd.f[2] = random_float_between(-1.0f, 1.0f);
            *(u128_25F4A8 *)&vel = *(u128_25F4A8 *)&rnd;
            vec_scale_to_length(&vel, &vel, fF * (random_float_between(0.0f, 3.0f) * g_frame_time_r));
            p1 = swap_packed_color_channels_r(c0.v[random_integer_below(6)], flag);
            p2 = swap_packed_color_channels_r(c1.v[random_integer_below(6)], flag);
            spawn_particle_alt_9768(pos, &vel, p1, p2, rand_range_r(scale_ticks(0x1E), scale_ticks(0x2D)) - s18 * 5,
                                    200000.0f);
        }
    }
    if (m != 0 && fC > 0.0f) {
        int r1 = 150;
        int r2 = 150;
        int r3 = 127;
        int u;
        if (flag) {
            r1 = 70;
            r2 = 70;
            r3 = 60;
        }
        u = r3 & 0xFF;
        if (cpu_frame_load < 0.95f && fE < dist) {
            spawn_glow_particle_r(m, pos, &vb, fC, scale_ticks(0x10), r1, 0x96, r2, 0x20);
            spawn_glow_particle_r(m, pos, &vb, fC, scale_ticks(0x16), u, 0x7F, 0x50, 0x20);
        }
        spawn_glow_particle_r(m, pos, &vb, fC, scale_ticks(0x1E), u, 0x7F, 0, 0x30);
    }
    if (m != 0 && fD > 0.0f) {
        spawn_glow_particle_r(m, pos, &vb, fD, scale_ticks(0x1B), 0xFF, 0xFF, 0xFF, 0x20);
    }
    {
        if (k11) {
            Q_25F4A8 v;
            *(u128_25F4A8 *)&v = *(u128_25F4A8 *)pos;
            v.f[3] = 2.0f;
            if (FastBSphereCheck(&v, 10.0f) != -1) {
                ((Cam *)player_camera_r)->f160 = (dist < 20.0f) ? 0.4f - dist * 0.0175f : 0.050000012f;
                ((Cam *)player_camera_r)->f168 = scale_ticks(0x19);
            }
        }
    }
    if (m != 0 && k10 != -1) {
        Moby_PlaySound_r(k10, 0, m);
    }
    if (fG != 0.0f) {
        if (s18 == 0) {
            Q_25F4A8 v;
            qcopy(&v, pos);
            v.f[3] = fG;
            if (FastBSphereCheck(&v, 100.0f) != -1) {
                if (0.0f < fG) {
                    ((Fl *)beam_explosion_blast_params)->f[9] = fG;
                    ((Fl *)beam_explosion_blast_params)->f[10] = fG;
                    ((Fl *)beam_explosion_blast_params)->f[8] = fG;
                } else {
                    ((Fl *)beam_explosion_blast_params)->f[9] = 15.0f;
                    ((Fl *)beam_explosion_blast_params)->f[10] = 15.0f;
                    ((Fl *)beam_explosion_blast_params)->f[8] = 15.0f;
                }
                if (flag) {
                    mine_camera_blast(beam_explosion_alt_color_blast_params, pos, 0, 0);
                } else {
                    mine_camera_blast(beam_explosion_blast_params, pos, 0, 0);
                }
            }
        }
    }
}

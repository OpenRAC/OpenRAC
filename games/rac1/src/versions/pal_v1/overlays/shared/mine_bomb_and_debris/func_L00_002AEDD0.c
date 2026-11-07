/* Adapted from Lombyte's project (GPL-3.0). */
void bomb_update(M_2aedd0 *m) {
    B910 rec = bomb_blast_fx;
    V2AE110 delta;
    V2AE110 old;
    V2AE110 a;
    V2AE110 b;
    V2AE110 proj;
    D_2aedd0 *d;

    b.q = 0;
    b.f[2] = g_frame_time * 2.0f;
    a.q = b.q;
    b.q = 0;
    b.f[2] = 1.0f;
    d = m->d;
    if ((((M910 *)D_G910.items[0].moby) == 0 || ((M910 *)D_G910.items[0].moby)->cls != 0xC0) && m->state == 0) {
        DeleteMoby(m);
        return;
    }
    if (D_G910.vehicle_mode != 0 && m->state == 0) {
        DeleteMoby(m);
        return;
    }
    if ((D_G910.unk20A5 != 0 || D_G910.unk20AF != 0) && m->state == 0) {
        m->flags |= 0x41;
    } else {
        m->flags &= ~0x41;
    }
    if (d->owner->cls == 0xC0 && d->h56 == 0 && FastVecLength(d) > 0.0f) {
        if (((unsigned char *)D_G910.ground_moby) == 0 ||
            get_moby_pvars_link((unsigned char *)D_G910.ground_moby) == 0) {
            predict_throw_landing(d, m, 1);
        } else if (predict_throw_landing(d, m, 0) == 0) {
            predict_throw_landing(d, m, 2);
        }
    }
    if (m->state == 1) {
        V2AE110 c;
        float t;
        int cr;
        int cg;
        int cb;

        c.q = 0;
        c.f[0] = 128.0f;
        c.f[1] = 128.0f;
        c.f[2] = 128.0f;
        c.f[3] = 255.0f;
        FastDecTimer(&d->h6a);
        t = IntToFloat(d->h6a) / IntToFloat(scale_ticks(0x1E));
        cr = truncate_float_to_s32((c.f[0] - 255.0f) * t + 255.0f);
        cg = truncate_float_to_s32((c.f[1] - 64.0f) * t + 64.0f);
        cb = truncate_float_to_s32((c.f[2] - 64.0f) * t + 64.0f);
        m->color = (cb << 16) | (0xFF << 24) | (cg << 8) | cr;
        if (d->h68 == 0) {
            if ((level_frame_counter & 7) == 0) {
                int i;

                for (i = 0; i < bomb_trail_spark_count; i++) {
                    V2AE110 v1;
                    V2AE110 v2;
                    V2AE110 v3;
                    int p0;
                    int p1;
                    int n0;
                    int n1;
                    int n2;

                    FastVecScale(&v1, d, bomb_trail_spark_speed_scale);
                    random_vector(v3.f, 0.0f, bomb_trail_spark_jitter * g_frame_time);
                    FastVecAdd(&v1, &v3, &v1);
                    clear_u64_value(v2.f);
                    v1.f[3] = random_float_between(bomb_trail_spark_start_size_min, bomb_trail_spark_start_size_max);
                    v2.f[3] = random_float_between(bomb_trail_spark_end_size_min, bomb_trail_spark_end_size_max);
                    p0 = FastTweenColor(bomb_trail_spark_lower_color_a, bomb_trail_spark_lower_color_b,
                                        random_float_between(0.0f, 1.0f));
                    p1 = FastTweenColor(bomb_trail_spark_upper_color_a, bomb_trail_spark_upper_color_b,
                                        random_float_between(0.0f, 1.0f));
                    n0 = truncate_float_to_s32(multiply_global_factor(
                        (float)bomb_trail_spark_ticks1 * random_float_between(0.0f, 1.0f) + 1.0f));
                    n1 = truncate_float_to_s32(multiply_global_factor(
                        (float)bomb_trail_spark_ticks2 *
                        (random_float_between(-bomb_trail_spark_time_jitter, bomb_trail_spark_time_jitter) + 1.0f)));
                    n2 = truncate_float_to_s32(multiply_global_factor(
                        (float)bomb_trail_spark_ticks3 *
                        (random_float_between(-bomb_trail_spark_time_jitter, bomb_trail_spark_time_jitter) + 1.0f)));
                    v2.f[2] -= bomb_trail_spark_gravity * g_frame_time_sq * (float)n1;
                    PartType02Spawn(&m->pos, &v1, &v2, p0, p1, n0, n1, n2, bomb_trail_spark_texture);
                }
            }
        }
        {
            Q_2aedd0 q;

            m->rx = FastAddRots(m->rx, g_frame_time * 2.0943952f);
            m->ry = FastAddRots(m->ry, g_frame_time * 7.8539815f);
            qcopy_nc(&old, &m->pos);
            FastVecAdd(&m->pos, &m->pos, d);
            if (d->h68 == 0) {
                d->v.f[2] -= g_frame_time_sq * 11.0f;
            } else {
                d->v.f[2] -= g_frame_time_sq * 11.0f / 10.0f;
                if (random_integer_below(scale_ticks(6) - 1) == 0) {
                    float h = resolve_camera_surface_height((float *)&m->pos, &q);

                    FastVecScale(&q, d, 0.25f);
                    PartType34Spawn(&m->pos, &q, random_float_between(0.05f, 0.1f) * 210000.0f, h);
                }
            }
            q.moby = m;
            q.flags = 0x830000;
            if (d->h54 < scale_ticks(0x12C) - scale_ticks(0xA)) {
                q.flags |= 1;
            }
            q.iE0 = 1;
            q.fDC = 2.0f;
            qcopy(&q, d);
            vec_set_length_xy(&q, &q, 1.0f);
            q.dir.f[2] = 1.0f;
            q.dir.f[3] = 5627.9248f;
            q.kind = 2;
            q.cls = m->cls;
            q.on = 1;
            if (d->h68 != 0) {
                d->h68++;
            }
            if (coll_line(&old, &m->pos, 0x10, (int)m, (int)&q)) {
                if (d->h68 == 0 && CollType() == 0 && d->v.f[2] < 0.0f) {
                    V2AE110 p;
                    V2AE110 vel;
                    char *s;
                    int i;

                    pool_splash(water_patches, water_patch_count, 1, m->pos.f[0], m->pos.f[1], 0.5f, -0.35f);
                    d->h68 = 1;
                    clear_u64_value(d->v.f);
                    d->v.f[2] = g_frame_time * -1.5f;
                    p.f[0] = m->pos.f[0];
                    p.f[1] = m->pos.f[1];
                    p.f[2] = collision_hit_z[0];
                    s = spawn_burst_moby(&p, 2.0f);
                    if (s != 0) {
                        s[0x23] = 0x70;
                    }
                    for (i = 0; i < 16; i++) {
                        float ang = random_angle_radians();
                        float r = random_float_between(g_frame_time * 0.0f, g_frame_time * 3.0f);
                        int n;

                        vel.f[0] = FastCos(ang) * r;
                        vel.f[1] = FastSin(ang) * r;
                        vel.f[2] = random_float_between(g_frame_time * 3.0f, g_frame_time * 6.5f);
                        n = random_int_between(0x5A, 0x78);
                        PartType35Spawn(&p, &vel, random_integer_below(2), n);
                    }
                } else if (d->h68 != 0 || CollType() != 0) {
                    if (D_H910.moby != 0) {
                        if ((D_H910.moby != ((M910 *)D_G910.moby) && D_H910.moby != d->owner) ||
                            d->h54 < scale_ticks(0x12C) - scale_ticks(0xA)) {
                            if (((M_2aedd0 *)D_H910.moby)->f74 != bomb_hit_skip_update_fn ||
                                D_H910.moby->cls == 0x336) {
                                H910 *h = &D_H910;

                                m->bBC = 1;
                                qcopy(&m->pos, &h->pos);
                                reflect_vector(&a, d, &h->nrm);
                                vec_scale_to_length(&a, &a, g_frame_time * 2.0f);
                                vec_scale_to_length(&b, &h->nrm, 1.0f);
                            }
                        }
                    } else if (D_H910.i1C > 0) {
                        H910 *h = &D_H910;

                        m->bBC = 1;
                        qcopy(&m->pos, &h->pos);
                        reflect_vector(&a, d, &h->nrm);
                        vec_scale_to_length(&a, &a, g_frame_time * 2.0f);
                        vec_scale_to_length(&b, &h->nrm, 1.0f);
                    }
                }
            } else if (FastDecTimer(&d->h54)) {
                m->bBC = 1;
                FastVecAdd(&a, &a, d);
            }
            if (m->bBC != 0) {
                float s = IntToFloat((unsigned char)D_E910.p0[0xA]) + 1.0f;
                float dist;
                V2AE110 v;
                int i;

                FastVecScale(&a, &a, s);
                FastVecSub(&delta, g_debug_camera, &m->pos);
                dist = FastVecLength(&delta);
                if (d->h68 < scale_ticks(0x14)) {
                    int n;

                    n = coll_sphere_mobys_v(&m->pos, 0x10, m, 0, s * 0.5f);
                    v.q = m->pos.q;
                    SendRadialMessageToMobys(m, &v, coll_sphere_moby_hits, n, 0, 0x830000, 2, 1, 2.0f, 1.0f, 1.0f);
                    m->state = 2;
                }
                {
                    int k;

                    for (k = 0; k < 10; k++) {
                        V2AE110 t;
                        float dp;

                        t.q = 0;
                        t.f[0] = random_float_between(-1.0f, 1.0f);
                        t.f[1] = random_float_between(-1.0f, 1.0f);
                        v.q = t.q;
                        dp = FastVecDot(&v, &b);
                        vec_scale_to_length(&proj, &b, dp);
                        FastVecSub(&v, &v, &proj);
                        vec_scale_to_length(&proj, &b, random_float_between(0.0f, 1.0f));
                        FastVecAdd(&v, &v, &proj);
                        vec_scale_to_length(&v, &v, s * random_float_between(3.5f, 6.5f) * g_frame_time);
                        FastVecAdd(&v, &v, &a);
                        SpawnFallingSpark((char *)&m->pos, (char *)&v,
                                          random_int_between(scale_ticks(0x3C), scale_ticks(0x78)), 0,
                                          (unsigned char)D_E910.p0[0xA]);
                    }
                }
                if (d->h68 < scale_ticks(0x14)) {
                    V2AE110 t;
                    float g;
                    int r2;
                    int g2;
                    int b2;
                    int cnt;

                    {
                        int k;

                        for (k = 0; k < 4; k++) {
                            float dp;

                            t.q = 0;
                            t.f[0] = random_float_between(-1.0f, 1.0f);
                            t.f[1] = random_float_between(-1.0f, 1.0f);
                            v.q = t.q;
                            dp = FastVecDot(&v, &b);
                            vec_scale_to_length(&proj, &b, dp);
                            FastVecSub(&v, &v, &proj);
                            vec_scale_to_length(&proj, &b, random_float_between(0.0f, 1.0f));
                            FastVecAdd(&v, &v, &proj);
                            vec_scale_to_length(&v, &v, s * random_float_between(6.5f, 10.0f) * g_frame_time);
                            FastVecAdd(&v, &v, &a);
                            SpawnFallingSpark((char *)&m->pos, (char *)&v,
                                              random_int_between(scale_ticks(0x3C), scale_ticks(0x5A)), 1,
                                              (unsigned char)D_E910.p0[0xA]);
                        }
                    }
                    t.q = 0;
                    t.f[0] = random_float_between(-1.0f, 1.0f);
                    t.f[1] = random_float_between(-1.0f, 1.0f);
                    t.f[2] = random_float_between(-1.0f, 1.0f);
                    v.q = t.q;
                    delta.f[2] += dist * 0.5f;
                    vec_scale_to_length(&v, &v, dist / 5.0f * g_frame_time);
                    vec_scale_to_length(&delta, &delta, dist * 2.0f * g_frame_time);
                    FastVecAdd(&v, &v, &delta);
                    clamp_vec_length(&v, &v, g_frame_time * 10.0f);
                    SpawnFallingSpark((char *)&m->pos, (char *)&v,
                                      random_int_between(scale_ticks(0x3C), scale_ticks(0x5A)), 1,
                                      (unsigned char)D_E910.p0[0xA]);
                    if (dist < 6.0f) {
                        cnt = truncate_float_to_s32(dist) + 1;
                    } else {
                        cnt = 4;
                    }
                    g = 0.0f;
                    if (dist < 7.0f) {
                        g = 7.0f - dist;
                    }
                    for (i = 0; i < cnt; i++) {
                        float spd = s * random_float_between(8.0f, 10.0f) * g_frame_time - g * g_frame_time;
                        C6_2aedd0 c1 = bomb_blast_colors;
                        C6_2aedd0 c2 = bomb_blast_fade_colors;

                        PartType11Spawn(
                            &m->pos, &a,
                            swap_packed_color_channels(c1.c[random_integer_below(6)], (unsigned char)D_E910.p0[0xA]),
                            swap_packed_color_channels(c2.c[random_integer_below(6)], (unsigned char)D_E910.p0[0xA]),
                            s * 400000.0f, random_int_between(scale_ticks(0xF), scale_ticks(0x14)),
                            random_int_between(scale_ticks(0x19), scale_ticks(0x1E)), 0, 0, spd);
                    }
                    if (dist > 9.0f) {
                        int r;
                        int gg;
                        int bb;

                        spawn_randomly_rotated_colored_moby(m, &m->pos, &a, s * 4.0f, scale_ticks(0xF), 0x7F, 0x7F,
                                                            0x7F, 0x20);
                        r = 0x7F;
                        gg = 0x20;
                        bb = 0;
                        swap_color_channel_words(&r, &gg, &bb, (unsigned char)D_E910.p0[0xA]);
                        spawn_randomly_rotated_colored_moby(m, &m->pos, &a, s * 4.0f, scale_ticks(0x18), r, gg, bb,
                                                            0x20);
                    }
                    r2 = 0x7F;
                    g2 = 0x3F;
                    b2 = 0;
                    swap_color_channel_words(&r2, &g2, &b2, (unsigned char)D_E910.p0[0xA]);
                    spawn_randomly_rotated_colored_moby(m, &m->pos, &a, s * 4.0f, scale_ticks(0x14), r2, g2, b2, 0x30);
                    r2 = 0x60;
                    g2 = 0x10;
                    b2 = 0;
                    swap_color_channel_words(&r2, &g2, &b2, (unsigned char)D_E910.p0[0xA]);
                    spawn_randomly_rotated_colored_moby(m, &m->pos, &a, s * 3.5f, scale_ticks(0x1B), r2, g2, b2, 0x40);
                    r2 = 0x20;
                    g2 = 0;
                    b2 = 0;
                    swap_color_channel_words(&r2, &g2, &b2, (unsigned char)D_E910.p0[0xA]);
                    spawn_randomly_rotated_colored_moby(m, &m->pos, &a, s * 3.0f, scale_ticks(0x1D), r2, g2, b2, 0x20);
                }
                if (d->h68 >= scale_ticks(0x14)) {
                    float h;

                    m->pos.f[2] += 1.0f;
                    h = resolve_camera_surface_height((float *)&m->pos, &v);
                    m->pos.f[2] -= 1.0f;
                    {
                        int i;

                        for (i = 0; i < 150; i++) {
                            V2AE110 u;
                            V2AE110 t;

                            t.q = 0;
                            t.f[0] = random_float_between(-1.0f, 1.0f);
                            t.f[1] = random_float_between(-1.0f, 1.0f);
                            t.f[2] = random_float_between(-0.5f, 2.0f);
                            u.q = t.q;
                            vec_scale_to_length(&u, &u,
                                                s * random_float_between(g_frame_time * 4.0f, g_frame_time * 8.0f));
                            PartType34Spawn(&m->pos, &u, random_float_between(0.05f, 0.1f) * 210000.0f, h);
                        }
                    }
                }
                d->v.f[0] = 0.0f;
                m->bBC = 0;
                if (d->h68 >= scale_ticks(0x14)) {
                    Sound_PlayClassSound(0x16, 0, (int)m, 0);
                } else if ((unsigned char)D_E910.p0[0xA] != 0) {
                    Moby_PlaySound(1, 0, (int)m);
                } else {
                    Moby_PlaySound(0, 0, (int)m);
                }
                if (d->h68 != 0) {
                    float gz;
                    float wz;
                    int cr = 0x60;
                    int cg = 0x60;
                    int cb = 0x7F;
                    int delay = 0;

                    v.q = m->pos.q;
                    v.f[2] += 3.5f;
                    gz = probe_ground_height(&v, 0, 0.5f);
                    wz = probe_ground_height_flag20(&v);
                    if (gz != wz && gz - wz < 1.2f) {
                        V2AE110 v2;
                        V2AE110 w;
                        int i;
                        int j;

                        for (i = 0; i < 100; i++) {
                            float r;
                            float ang;

                            w.q = 0;
                            w.f[0] = random_float_between(g_frame_time * -0.75f, g_frame_time * 0.75f);
                            w.f[1] = random_float_between(g_frame_time * -0.75f, g_frame_time * 0.75f);
                            w.f[2] = random_float_between(g_frame_time * 4.0f, g_frame_time * 14.0f);
                            v2.q = w.q;
                            r = random_float_between(0.0f, 0.35f);
                            ang = random_angle_radians();
                            w.f[0] = FastCos(ang) * r;
                            w.f[1] = FastSin(ang) * r;
                            w.f[2] = 0.0f;
                            FastVecAdd(&w, &w, &m->pos);
                            w.f[2] = gz + 0.5f;
                            v2.f[2] -= r * g_frame_time * 8.0f;
                            spawn_gravity_floor_burst_particle(
                                &w, &v2, delay, random_float_between(0.25f, 1.0f), gz, g_frame_time_sq * 20.0f,
                                (0x16 << 24) | (cb << 16) | (cg << 8) | cr, (cb << 16) | (cg << 8) | cr, 1);
                        }
                        for (j = 0; j < 20; j++) {
                            int c0 = DefaultVtbl_DeleteMoby_v(0x7F << 24, (0x7F << 24) | (cb << 16) | (cg << 8) | cr,
                                                              random_float_between(0.25f, 1.0f));
                            int c1 = DefaultVtbl_DeleteMoby_v(0, ((cb / 2) << 16) | ((cg / 2) << 8) | (cr / 2),
                                                              random_float_between(0.5f, 1.0f));
                            float sz;
                            int life;

                            w.f[0] = random_float_between(-3.0f, 3.0f);
                            w.f[1] = random_float_between(-3.0f, 3.0f);
                            w.f[2] = random_float_between(g_frame_time * 4.0f, g_frame_time * 14.0f);
                            vec_scale_to_length(&w, &w, random_float_between(1.5f, 3.0f) * g_frame_time);
                            w.f[2] = random_float_between(3.0f, 6.0f) * g_frame_time;
                            v2.q = m->pos.q;
                            v2.f[2] = gz;
                            sz = random_float_between(31500.002f, 52500.0f);
                            life = random_int_between(scale_ticks(0x5A), scale_ticks(0x96));
                            spawn_shrinking_effect_particle_with_gravity_and_secondary_spawns(
                                &v2, &w, c0, c1, sz, life, 1, part_type_textures[11][0], -1);
                        }
                    }
                }
                gCameraLvl.f160 = dist < 20.0f ? 0.4f - dist * 0.0175f : 0.4f - 20.0f * 0.0175f;
                gCameraLvl.i168 = scale_ticks(0x19);
                fast_vec_dist(&m->pos, D_G910.motion.pos.f);
                mine_camera_blast((char *)&rec, (char *)&m->pos, 0, 0);
                if (m->state != 2) {
                    DeleteMoby(m);
                    return;
                }
            }
        }
    } else if (m->state != 2) {
        int v;

        d->h6a++;
        v = truncate_float_to_s32(FastSin(normalize_angle((float)d->h6a * 0.068f)) * 96.0f) + 0x9F;
        m->color = (v << 16) | (0xFF << 24) | (v << 8) | v;
    }
    if (m->state == 2) {
        V2AE110 p;
        float r;
        int n;

        m->flags |= 0x41;
        r = interpolate_scalar_linear(0.5f, (IntToFloat((unsigned char)D_E910.p0[0xA]) + 1.0f) * 3.0f - 0.5f,
                                      IntToFloat(m->bBC) / (float)scale_ticks(0xF));
        n = coll_sphere_mobys_v(&m->pos, 0x10, m, 0, r);
        p.q = m->pos.q;
        SendRadialMessageToMobys(m, &p, coll_sphere_moby_hits, n, 0, 0x830000, 2, 1, 2.0f, 1.0f, 1.0f);
        if (++m->bBC > scale_ticks(0xF)) {
            DeleteMoby(m);
        }
    }
}

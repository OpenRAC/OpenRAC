void morphoray_chicken_update(Moby *m) {
    Vec4A T;
    Vec4A v;
    Vec4A W;
    Vec4A V;
    Vars *d;
    Hero *h;
    float save58;
    float ang;
    float s;
    float speed;
    float r;
    float a;
    int hit;
    int i;
    int j;
    int n;
    Moby *o;

    hit = 0;
    h = (Hero *)g_hero_CB;
    d = (Vars *)m->pvars;
    save58 = m->anim_speed;
    ang = fast_atan2(m->pos.x - h->motion.pos.f[0], m->pos.y - h->motion.pos.f[1]);
    if (m->state == 0) {
        m->state = 1;
        d->h66 = 0;
        d->i70 = -1;
        d->h64 = random_int_between(scale_ticks(0x78), scale_ticks(0x12C));
        d->f78 = 1.0f;
    }
    if (m->state == 6) {
        return;
    }
    if (hero_globals_base_CB[2] != 0) {
        Manipulator_ApplyScale_v(m, G1920_CB, d->v20, 5.0f);
    } else {
        Manipulator_ApplyScale_v(m, G1920_CB, d->v20, 7.7f);
    }
    qcopy(&T, &m->pos);
    if (d->i70 >= 0 && t32_CB(&d->i70) != 0 && m->visible == 0) {
        del_CB(m);
        return;
    }
    if (d->i70 >= 0) {
        if ((0.9f < level_draw_frame_load || 0.9f < cpu_frame_load) && m->visible == 0 && d->i70 != -1) {
            if (random_integer_below(0xC7) == 0) {
                del_CB(m);
                return;
            }
            d->i70 = random_int_between(0x14, 0x1E);
        }
        if (1.0f < level_draw_frame_load || 1.0f < cpu_frame_load) {
            if (m->visible != 0) {
                if (random_integer_below(0x63) == 0) {
                    d->f78 = 0.0f;
                } else {
                    d->i70 = random_int_between(0x14, 0x1E);
                }
            } else if (random_integer_below(0x18) == 0) {
                del_CB(m);
                return;
            }
        }
    }
    if (d->i74 > 0) {
        m->scale += ((IntToFloat(d->i74) * 3.0f + 1.0f) * m->pclass->scale - m->scale) * 0.1f;
    }
    s = m->scale / m->pclass->scale;
    if (t32_CB(&d->i7C) != 0) {
        o = contact_CB(m, -1, 0);
        if (o != 0) {
            sound_CB(1, 0, m);
            d->f78 -= o->scale;
            d->i7C = scale_ticks(0x19);
        }
    }
    m->hit_slot = 0xFF;
    if (d->f78 < 1.0f) {
        if (d->i74 == 0) {
            for (i = 0; i < ftoi_CB(s) * 20; i++) {
                r = random_float_between(0.0f, 0.37f) * s;
                a = random_angle_radians();
                BuildSphericalOffset(&V, r, a, random_angle_radians());
                V.z += random_float_between(0.25f, 0.5f) * s;
                FastVecAdd(&V, &V, &T);
                r = random_float_between(E6C_CB * 0.1f, E6C_CB) * s;
                a = random_angle_radians();
                BuildSphericalOffset(&W, r, a, random_angle_radians());
                spawn_effect_particle_with_target_scalar(&V, &W, 0x3F7F7F7F, 0x2F2F2F, s * 150000.0f,
                                                         random_int_between(scale_ticks(0x1E), scale_ticks(0x39)));
            }
        } else {
            *(VecQuad *)&V = 0;
            V.x = E60_CB * 0.5f;
            V.z = E60_CB * 0.7f;
            V.y = E60_CB * 0.5f;
            *(VecQuad *)&W = *(VecQuad *)&V;
            spawn_explosion(m, &W, 0, 0.0f, 2.0f, 0, 0, 0, 4.0f, 3.0f, 9.0f, 1.0f, -1, 12.0f, 1, 1, -1,
                            (unsigned char)d->i74);
            n = f1F2BE8_CB(&m->pos, 3.0f, 0x10, m, 0);
            *(VecQuad *)&V = *(VecQuad *)&m->pos;
            SendRadialMessageToMobys(m, &V, coll_sphere_moby_hits, n, 0, 0x830000, 2, 1, 1.0f, 1.0f, 1.0f);
        }
        n = random_int_between(5, 8);
        if (0.85f < cpu_frame_load || 0.85f < level_draw_frame_load) {
            if (1.0f < cpu_frame_load || 1.0f < level_draw_frame_load) {
                n = 1;
            } else {
                n /= 2;
            }
        }
        for (j = 0; j < n; j++) {
            r = random_float_between(G1938_CB, G193C_CB) * s;
            a = random_angle_radians();
            BuildSphericalOffset(&V, r, a, random_angle_radians());
            V.z += G1934_CB * s;
            FastVecAdd(&V, &V, &T);
            a = random_float_between(G192C_CB * E6C_CB, G1930_CB * E6C_CB);
            BuildSphericalOffset(&W, a, random_angle_radians(), G1928_CB * 0.017453292f);
            W.z = random_float_between(3.5f, 5.5f) * E6C_CB;
            debris_CB(m, &V, &W);
        }
        sound_CB(1, 0, m);
        if (d->i70 >= 0) {
            del_CB(m);
            return;
        }
        m->collision = 0;
        m->flags = (m->flags & 0xEFFF) | 0x41;
        d->i74 = 0;
        m->state = 6;
        m->command = 0;
        return;
    }
    if (t16_CB(&d->h64) != 0) {
        sound_CB(random_integer_below(100) >= 75, 0, m);
        d->h64 = random_int_between(scale_ticks(0x78), scale_ticks(0x12C));
    }
    if (d->i70 >= 0) {
        speed = (E6C_CB + E6C_CB) * s;
    } else {
        speed = 0.333f;
    }
    switch (m->state) {
        case 0:
            break;
        case 1:
            if (m->anim_flags & 2) {
                MobyBlendAnimation(m, random_integer_below(2) == 0, 0, scale_ticks(5));
            }
            if (t16_CB(&d->h66) != 0 && vec_distance_xy(&m->pos, g_hero_CB + 0x80) < s * 5.0f) {
                v.x = FastCos(m->rot.z) * E6C_CB * s;
                v.y = FastSin(m->rot.z) * E6C_CB * s;
                v.z = 0.0f;
                FastVecAdd(&v, &v, &T);
                hit = move_moby_xy_with_collision_constraints(m, &T, &v, 0, s * 0.2f, s * 0.5f, speed, 0.2617994f);
                if (hit != 0 || (FastDiffRots(m->rot.z, fast_atan2(T.x - m->pos.x, T.y - m->pos.y)) < 1.5707964f &&
                                 fast_vec_dist(&m->pos, &T) > s * 0.01f)) {
                    m->state = 3;
                    MobyBlendAnimation(m, 3, 0, scale_ticks(10));
                } else {
                    d->h66 = random_int_between(scale_ticks(0x3C), scale_ticks(0xB4));
                }
            } else if (vec_distance_xy(&m->pos, g_hero_CB + 0x80) < s * 5.0f &&
                       0.5235988f < FastDiffRots(ang, m->rot.z)) {
                move_moby_xy_with_collision_constraints(m, &T, &T, 0, s * 0.2f, s * 0.5f, speed, 0.2617994f);
                m->state = 2;
                MobyBlendAnimation(m, 3, 0, scale_ticks(10));
            } else {
                move_moby_xy_with_collision_constraints(m, &T, &T, 0, s * 0.2f, s * 0.5f, speed, 0.2617994f);
            }
            break;
        case 2:
            MobySpringTurn(m, &d->f68, ang, E70_CB * 0.7853982f, E70_CB * 1.5707964f, E6C_CB * 0.7853982f);
            move_moby_xy_with_collision_constraints(m, &T, &T, 0, s * 0.2f, s * 0.5f, speed, 0.2617994f);
            if (t16_CB(&d->h66) != 0 && vec_distance_xy(&m->pos, g_hero_CB + 0x80) < s * 5.0f) {
                v.x = FastCos(m->rot.z) * E6C_CB * s;
                v.y = FastSin(m->rot.z) * E6C_CB * s;
                v.z = 0.0f;
                FastVecAdd(&v, &v, &T);
                if (absolute_float(probe_ground_height(&v, 0, 0.5f) - v.z) <= (E6C_CB + E6C_CB) * s) {
                    m->state = 3;
                    MobyBlendAnimation(m, 3, 0, scale_ticks(10));
                }
            } else if (FastDiffRots(ang, m->rot.z) < 0.08726646f ||
                       vec_distance_xy(&m->pos, g_hero_CB + 0x80) > s * 6.0f) {
                m->state = 1;
                d->h66 = random_int_between(scale_ticks(0x3C), scale_ticks(0xB4));
                MobyBlendAnimation(m, random_integer_below(2) == 0, 0, scale_ticks(5));
            }
            break;
        case 3:
            if (vec_distance_xy(&m->pos, g_hero_CB + 0x80) > s * 6.0f) {
                m->state = 1;
                d->h66 = random_int_between(scale_ticks(0x3C), scale_ticks(0xB4));
                MobyBlendAnimation(m, random_integer_below(2) == 0, 0, scale_ticks(5));
                move_moby_xy_with_collision_constraints(m, &T, &T, 0, s * 0.2f, s * 0.5f, speed, 0.2617994f);
            } else if (vec_distance_xy(&m->pos, g_hero_CB + 0x80) < s * 3.0f) {
                MobySpringTurn(m, &d->f68, ang, E70_CB * 0.7853982f, E70_CB * 1.5707964f, E6C_CB * 0.7853982f);
                v.x = FastCos(m->rot.z) * (E6C_CB + E6C_CB);
                v.y = FastSin(m->rot.z) * (E6C_CB + E6C_CB);
                v.z = 0.0f;
                FastVecAdd(&v, &v, &T);
                hit = move_moby_xy_with_collision_constraints(m, &T, &v, 0, s * 0.2f, s * 0.5f, speed, 0.2617994f);
                if (hit == 0 && (1.5707964f < FastDiffRots(m->rot.z, fast_atan2(T.x - m->pos.x, T.y - m->pos.y)) ||
                                 fast_vec_dist(&m->pos, &T) < s * 0.01f)) {
                    m->state = 1;
                    d->h66 = random_int_between(scale_ticks(0x3C), scale_ticks(0xB4));
                    MobyBlendAnimation(m, random_integer_below(2) == 0, 0, scale_ticks(5));
                } else {
                    m->state = 4;
                    MobyBlendAnimation(m, 2, 0, scale_ticks(10));
                }
            } else {
                MobySpringTurn(m, &d->f68, ang, E70_CB * 0.7853982f, E70_CB * 1.5707964f, E6C_CB * 0.7853982f);
                v.x = FastCos(m->rot.z) * E6C_CB * s;
                v.y = FastSin(m->rot.z) * E6C_CB * s;
                v.z = 0.0f;
                FastVecAdd(&v, &v, &T);
                hit = move_moby_xy_with_collision_constraints(m, &T, &v, 0, s * 0.2f, s * 0.5f, speed, 0.2617994f);
                if (hit == 0 && (1.5707964f < FastDiffRots(m->rot.z, fast_atan2(T.x - m->pos.x, T.y - m->pos.y)) ||
                                 fast_vec_dist(&m->pos, &T) < s * 0.01f)) {
                    m->state = 1;
                    d->h66 = random_int_between(scale_ticks(0x3C), scale_ticks(0xB4));
                    MobyBlendAnimation(m, random_integer_below(2) == 0, 0, scale_ticks(5));
                }
            }
            break;
        case 4:
            if (t16_CB(&d->h66) != 0 && vec_distance_xy(&m->pos, g_hero_CB + 0x80) > s * 4.0f) {
                MobySpringTurn(m, &d->f68, ang, E70_CB * 0.7853982f, E70_CB * 1.5707964f, E6C_CB * 0.7853982f);
                v.x = FastCos(m->rot.z) * E6C_CB * s;
                v.y = FastSin(m->rot.z) * E6C_CB * s;
                v.z = 0.0f;
                FastVecAdd(&v, &v, &T);
                hit = move_moby_xy_with_collision_constraints(m, &T, &v, 0, s * 0.2f, s * 0.5f, speed, 0.2617994f);
                if (hit == 0 && (1.5707964f < FastDiffRots(m->rot.z, fast_atan2(T.x - m->pos.x, T.y - m->pos.y)) ||
                                 fast_vec_dist(&m->pos, &T) < s * 0.01f)) {
                    m->state = 1;
                    d->h66 = random_int_between(scale_ticks(0x3C), scale_ticks(0xB4));
                    MobyBlendAnimation(m, random_integer_below(2) == 0, 0, scale_ticks(5));
                } else {
                    m->state = 3;
                    MobyBlendAnimation(m, 3, 0, scale_ticks(10));
                }
            } else {
                MobySpringTurn(m, &d->f68, ang, E70_CB * 0.7853982f, E70_CB * 1.5707964f, E6C_CB * 0.7853982f);
                v.x = FastCos(m->rot.z) * (E6C_CB + E6C_CB) * s;
                v.y = FastSin(m->rot.z) * (E6C_CB + E6C_CB) * s;
                v.z = 0.0f;
                FastVecAdd(&v, &v, &T);
                hit = move_moby_xy_with_collision_constraints(m, &T, &v, 0, s * 0.2f, s * 0.5f, speed, 0.2617994f);
                if (hit == 0 && (1.5707964f < FastDiffRots(m->rot.z, fast_atan2(T.x - m->pos.x, T.y - m->pos.y)) ||
                                 fast_vec_dist(&m->pos, &T) < s * 0.01f)) {
                    m->state = 1;
                    d->h66 = random_int_between(scale_ticks(0x3C), scale_ticks(0xB4));
                    MobyBlendAnimation(m, random_integer_below(2) == 0, 0, scale_ticks(5));
                }
            }
            break;
        case 5:
            if (CarriedMobyUpdate(m, d->p10) != 0) {
                m->state = 0;
                *(short *)(d->p14 + 0x68) = 0;
            }
            if (random_integer_below(4) != 0) {
                return;
            }
            {
                Vec4A *pv;
                Vec4A *pw;
                pv = &V;
                r = random_float_between(G1938_CB, G193C_CB) * s;
                a = random_angle_radians();
                BuildSphericalOffset(pv, r, a, random_angle_radians());
                V.z += G1934_CB * s;
                FastVecAdd(pv, pv, &T);
                pw = &W;
                a = random_float_between(G192C_CB * E6C_CB, G1930_CB * E6C_CB);
                BuildSphericalOffset(pw, a, random_angle_radians(), G1928_CB * 0.017453292f);
                W.z = random_float_between(3.5f, 5.5f) * E6C_CB;
                debris_CB(m, pv, pw);
            }
            return;
    }
    if (hit == 0) {
        float len;
        float max;
        FastVecSub(&W, &T, &m->pos);
        len = FastVecLength(&W);
        max = E6C_CB * s;
        if (max < len) {
            vec_scale_to_length(&W, &W, max);
        }
        FastVecAdd(&T, &m->pos, &W);
        if (absolute_float(probe_ground_height_flag20(&T) - m->pos.z) <= E6C_CB * s) {
            qcopy(&m->pos, &T);
        }
    } else {
        qcopy(&m->pos, &T);
    }
    d->f6C += E70_CB * 9.8f;
    m->pos.z -= d->f6C;
    {
        float g = probe_ground_height(&m->pos, 0, 0.5f);
        if (m->pos.z < g && g < m->pos.z + s * 0.25f) {
            d->f6C = 0.0f;
            m->pos.z = g;
        }
    }
    if (CollType() == 0) {
        d->f78 = 0.0f;
    }
    if (m->state != 5) {
        float pitch;
        float roll;
        m->pos.z = coll_hit_result_CB[10];
        qcopy(&W, &coll_hit_result_CB[16]);
        V.x = W.x * FastCos(m->rot.z) + W.y * FastSin(m->rot.z);
        V.y = W.y * FastCos(m->rot.z) - W.x * FastSin(m->rot.z);
        V.z = W.z;
        pitch = -fast_atan2(square_root(V.x * V.x + V.z * V.z), V.y);
        roll = fast_atan2(V.z, V.x);
        m->rot.x = FastAddRots(m->rot.x, FastSubRots(pitch, m->rot.x) * (E6C_CB * 6.2831855f));
        m->rot.y = FastAddRots(m->rot.y, FastSubRots(roll, m->rot.y) * (E6C_CB * 6.2831855f));
    }
    if (m->pos.z < 0.0f) {
        del_CB(m);
    } else {
        m->anim_speed = save58;
    }
}

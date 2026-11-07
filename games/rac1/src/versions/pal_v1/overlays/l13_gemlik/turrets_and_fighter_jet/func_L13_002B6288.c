extern void InitKnockback(void *, void *, float, int, int, int);
extern float D_E1_L13_00161314 SDATA(D_E1_L13_00161314);
extern float D_E1_L13_00161310 SDATA(D_E1_L13_00161310);
extern float D_E1_L13_00161318 SDATA(D_E1_L13_00161318);
extern float D_E1_L13_0016131C SDATA(D_E1_L13_0016131C);
extern short D_E1_L13_00161320;
extern int D_E1_L13_00161330[] MACRO_ADDR;
void robosquawker_update(Moby *m) {
    RsVars *v;
    RsVec old;
    RsTarget tgt;
    RsVec va;
    RsVec vb;
    RsVec vc;
    int hits;
    float dmg;
    float ang;
    float h;
    float out;
    RsHit *hit;
    RsTargetMoby *tm;

    v = (RsVars *)m->pvars;
    Manipulator_ApplyScale(2.5f, (int)m, 0, v->x230);
    if (m->visible != 0) {
        if (fast_vec_dist(&m->pos, gemlik_camera_pos) < 28.0f) {
            set_ground_height_band(m);
            m->shadow_range = 0x16;
        }
    }
    if (gemlik_game_mode == 2) {
        m->collision = 0;
        m->flags |= 0x41;
        return;
    }
    m->collision = *(int *)(((char *)m->pclass) + 0x10);
    m->flags &= 0xFFBE;
    FindNearestTargetMoby(m, &tgt, v->f1FC + 4.0f);
    tm = tgt.moby;
    if (tm == 0 || (v->h214 == 0 && v->w22C != -1 && is_point_inside_clip_volume(&tgt, v->w22C) == 0)) {
        tgt.moby = 0;
        tm = 0;
        qzero(&tgt);
    }
    qcopy(&old, &m->pos);
    if (v->w38 != 0) {
        v->h214 = scale_ticks(0xF0);
    }
    v->w38 = 0;
    FastDecTimer(&v->h208);
    if (m->state != 0) {
        if (FastDecTimer(&v->h214) != 0) {
            v->f1FC = v->v1D0.f[3];
        } else {
            v->f1FC = v->f200;
        }
    }
    dmg = 0.0f;
    hit = MobyGetHitMessage(m, 0x330000, 0);
    ComputeHitResponse(m, hit, &v->f20, 0, &hits, &dmg, 0, 4);
    FastDecTimer(&v->h26);
    if (v->h26 < scale_ticks(0x2D) && hit != 0 && m->state != 0x63) {
        if ((*(unsigned short *)&hit->src->oclass) >= 0xB0 && (*(unsigned short *)&hit->src->oclass) <= 0xB1 &&
            v->h26 != 0) {
            hit->f2C = 0.0f;
        }
        if (m->group != 0xFF) {
            alert_group_e81e0(m->group, 1);
        }
        if (hit->f2C != 0.0f) {
            ang = fast_atan2(m->pos.x - tgt.pos.f[0], m->pos.y - tgt.pos.f[1]);
            v->f148 = 0.5f;
            v->w140 = 0x202;
            v->f20 -= hit->f2C;
            v->f130 = 0.0055f;
            v->f134 = 0.0011f;
            v->f138 = D_E1_L13_00161314 * g_frame_time;
            v->f13C = D_E1_L13_00161310 * g_frame_time;
            v->w144 = 9;
            v->b15D = 0;
            if (v->f20 <= 0.0f) {
                m->flags &= 0xEFFF;
                v->f138 = g_frame_time * 10.0f;
                v->f13C = g_frame_time * 5.5f;
                va = hit->pos;
                trooper_knockback_arc(&va, &ang, &v->f138, &v->f13C);
                InitKnockback(m, v->x120, ang, (func_001160D8() & 1) + 9, 1, 0);
                v->f170 = 11.0f;
                v->f174 = 18.0f;
                m->state = 0x63;
                release_robosquawker_particle((Level13VendorMoby *)v);
                v->x110[7] = 0x78;
                RerollEffectCounter(m, (short *)v->x110);
                SetDeathBits(m, 0, -1);
            } else {
                va = hit->pos;
                trooper_knockback_arc(&va, &ang, &v->f138, &v->f13C);
                InitKnockback(m, v->x120, ang, (func_001160D8() & 1) + 9, 1, 0);
                v->f170 = 5.0f;
                v->f174 = 10.0f;
                m->state = 6;
                release_robosquawker_particle((Level13VendorMoby *)v);
                *(unsigned char *)&v->x110[7] = 0xFA;
                v->h26 = scale_ticks(0x3C);
                RerollEffectCounter(m, (short *)v->x110);
            }
        }
    }
    m->hit_slot = 0xFF;
    switch (m->state) {
        case 0x0:
            qcopy(&v->v1D0, &m->pos);
            v->v1D0.f[3] = v->f1FC;
            if (v->h20A != 0) {
                m->state = 1;
                if (m->prev_seq != 3) {
                    MobyBlendAnimation(m, 3, 0, 0);
                }
            } else {
                m->state = 0xE;
                m->pos.z += 6.0f;
                if (m->prev_seq != 0) {
                    MobyBlendAnimation(m, 0, 0, 0);
                }
            }
            v->h214 = 0;
            v->h208 = 0;
            v->b29 = 0;
            v->f20 = 1.0f;
            v->h24 = 1;
            v->b58 = 8;
            v->b5A = 4;
            MoveCtl_Init((char *)&v->w180);
            v->w180 = 0x38D;
            v->f18C = 2.0f;
            v->f188 = 2.0f;
            v->f1A4 = v->f204 * g_frame_time;
            v->w1B8 |= 0x30;
            if (func_001160D8() & 1) {
                m->flags |= 0x8000;
            }
            v->pD0 = &D_E1_L13_00161320;
            if (v->w220 != -1) {
                v->w224 = select_point_index_by_distance(&m->pos, walker_paths[v->w220], 0.0f);
            }
            break;
        case 0xE: {
            float d;

            update_robosquawker_particle((unsigned char *)m, (char *)v);
            FastVecSub(&va, &tgt, &m->pos);
            d = probe_ground_height(&m->pos, 0x120, 0.5f);
            if (d > 1.0f && vector_length_xy(&va) < v->f1FC + 4.0f && absolute_float(m->pos.z - tgt.pos.f[2]) < 10.0f &&
                random_integer_below(0x13) == 0) {
                if (m->prev_seq != 0) {
                    MobyBlendAnimation(m, 0, 0, scale_ticks(3));
                }
                m->state = 0x11;
                release_robosquawker_particle((Level13VendorMoby *)v);
            } else if (d > 1.0f && m->command == 1 && random_integer_below(4) == 0) {
                if (random_integer_below(0x100) & 1) {
                    v->f1F8 = -v->f1F8;
                }
                v->f1F4 = v->f1F8 * 0.017453292f;
                *(int *)&v->h210 = scale_ticks((int)random_float_between(30.0f, 90.0f));
                if (m->prev_seq != 0) {
                    MobyBlendAnimation(m, 0, 0, scale_ticks(3));
                }
                release_robosquawker_particle((Level13VendorMoby *)v);
                m->state = 0x11;
                clear_u64_value(&v->v40);
                m->command = 0;
            } else if (v->w220 != -1) {
                char *path;

                path = walker_paths[v->w220];
                if (fast_vec_dist(&m->pos, path + (v->w224 * 16 + 16)) < 0.25f) {
                    v->w224 += 1;
                    if (v->w224 >= *(int *)path) {
                        v->w224 = 0;
                    }
                }
                qcopy(&v->v1E0, path + v->w224 * 16 + 16);
                m->state = 0x12;
            } else if (fast_dec_timer(&v->w20C) != 0) {
                int i;

                v->w20C = scale_ticks(random_int_between(0x28, 0x78));
                for (i = 0; i < 10; i++) {
                    float r;
                    float a;

                    r = random_float_between(0.0f, 4.0f);
                    a = random_angle_radians();
                    vb.f[0] = FastCos(a) * r;
                    vb.f[1] = FastSin(a) * r;
                    vb.f[2] = 0.0f;
                    vb.f[2] = random_float_between(5.0f, 7.0f);
                    FastVecAdd(&v->v1E0, &vb, &v->v1D0);
                    if (probe_ground_height(&v->v1E0, 0x120, 0.5f) > 1.0f) {
                        break;
                    }
                }
                if (m->prev_seq != 0) {
                    MobyBlendAnimation(m, 0, 0, scale_ticks(3));
                }
                m->state = 0xF;
            }
            break;
        }
        case 0xF: {
            float a;

            a = fast_atan2(v->v1E0.f[0] - m->pos.x, v->v1E0.f[1] - m->pos.y);
            MobySpringTurn((char *)m, &v->f1F0, a, 0.03f, 0.6f, 0.09f);
            h = FastVecLength(&v->v40);
            m->anim_speed = h * 20.0f + 1.0f;
            FastVecSub(&v->v40, &v->v1E0, &m->pos);
            if (FastVecLength(&v->v40) > 1.0f) {
                ApproachValue(&h, g_frame_time * 4.0f, g_frame_time_v[1] * 6.0f);
            } else {
                ApproachValue(&h, g_frame_time, g_frame_time_v[1] * 8.0f);
            }
            ClampVectorLength(&v->v40, h);
            FastVecAdd(&va, &v->v40, &m->pos);
            if (coll_sphere(1.2f, &va, 0, m) != 0) {
                clear_u64_value(&v->v40);
                if (m->prev_seq != 0) {
                    MobyBlendAnimation(m, 0, 0, scale_ticks(3));
                }
                m->state = 0xE;
            } else {
                qcopy(&m->pos, &va);
            }
            if (fast_vec_dist(&m->pos, &v->v1E0) < h || h == 0.0f) {
                if (m->prev_seq != 0) {
                    MobyBlendAnimation(m, 0, 0, scale_ticks(3));
                }
                m->state = 0xE;
            }
            update_robosquawker_particle((unsigned char *)m, (char *)v);
            break;
        }
        case 0x12: {
            char *path;
            float a;
            float d;

            path = walker_paths[v->w220];
            a = fast_atan2(v->v1E0.f[0] - m->pos.x, v->v1E0.f[1] - m->pos.y);
            MobySpringTurn((char *)m, &v->f1F0, a, 0.018f, 0.8f, 0.09f);
            FastVecSub(&v->v40, &v->v1E0, &m->pos);
            ClampVectorLength(&v->v40, g_frame_time * 4.0f);
            FastVecAdd(&va, &v->v40, &m->pos);
            m->anim_speed = FastVecLength(&v->v40) * 20.0f + 1.0f;
            if (coll_sphere(1.2f, &va, 0, m) != 0) {
                int i;

                v->w20C = scale_ticks(0x78);
                for (i = 0; i < 10; i++) {
                    float r;
                    float an;

                    r = random_float_between(0.0f, 3.0f);
                    an = random_angle_radians();
                    vb.f[0] = FastCos(an) * r;
                    vb.f[1] = FastSin(an) * r;
                    vb.f[2] = 0.0f;
                    vb.f[2] = random_float_between(5.0f, 7.0f);
                    FastVecAdd(&v->v1E0, &vb, &v->v1D0);
                    if (probe_ground_height(&v->v1E0, 0x120, 0.5f) > 1.0f) {
                        break;
                    }
                }
                if (m->prev_seq != 0) {
                    MobyBlendAnimation(m, 0, 0, scale_ticks(3));
                }
                m->state = 0xF;
            } else {
                qcopy(&m->pos, &va);
            }
            d = fast_vec_dist(&m->pos, &v->v1E0);
            if (d < FastVecLength(&v->v40)) {
                float g;

                v->w224 += 1;
                if (v->w224 >= *(int *)path) {
                    v->w224 = 0;
                }
                qcopy(&v->v1E0, path + v->w224 * 16 + 16);
                v->v1E0.f[2] = v->v1E0.f[2] + random_float_between(-0.2f, 0.2f);
                g = probe_ground_height(&m->pos, 0x120, 0.5f);
                if (g > 1.0f && vector_length_xy(&vb) < v->f1FC + 4.0f &&
                    absolute_float(m->pos.z - tgt.pos.f[2]) < 10.0f && random_integer_below(0x13) == 0) {
                    if (m->prev_seq != 0) {
                        MobyBlendAnimation(m, 0, 0, scale_ticks(3));
                    }
                    qzero(&v->v40);
                    m->state = 0x11;
                    release_robosquawker_particle((Level13VendorMoby *)v);
                } else if (g > 1.0f && m->command == 1 && random_integer_below(4) == 0) {
                    if (random_integer_below(0x100) & 1) {
                        v->f1F8 = -v->f1F8;
                    }
                    v->f1F4 = v->f1F8 * 0.017453292f;
                    *(int *)&v->h210 = scale_ticks((int)random_float_between(30.0f, 90.0f));
                    if (m->prev_seq != 0) {
                        MobyBlendAnimation(m, 0, 0, scale_ticks(3));
                    }
                    release_robosquawker_particle((Level13VendorMoby *)v);
                    qzero(&v->v40);
                    m->state = 0x11;
                    clear_u64_value(&v->v40);
                    m->command = 0;
                }
            }
            update_robosquawker_particle((unsigned char *)m, (char *)v);
            break;
        }
        case 0x11: {
            float g;

            g = probe_ground_height(&m->pos, 0x120, 0.5f);
            ApproachValue(&v->v40.f[2], -(g_frame_time * 8.0f), g_frame_time_v[1] * 10.8f);
            move_4318_v(m, &v->v40, 0.6f, 0.5f, 0.0f, 0);
            if (m->prev_seq != 2 && m->pos.z - g <= g_frame_time * 8.0f * multiply_global_factor(45.0f) &&
                m->prev_seq != 2) {
                MobyBlendAnimation(m, 2, 0, scale_ticks(5));
            }
            if (m->pos.z <= g) {
                m->pos.z = g;
                if (random_integer_below(0x100) & 1) {
                    v->f1F8 = -v->f1F8;
                }
                v->f1F4 = v->f1F8 * 0.017453292f;
                *(int *)&v->h210 = scale_ticks((int)random_float_between(30.0f, 90.0f));
                m->state = 3;
                clear_u64_value(&v->v40);
                if (m->prev_seq != 5) {
                    MobyBlendAnimation(m, 5, 0, scale_ticks(3));
                }
            }
            break;
        }
        case 0x10: {
            float g;

            g = probe_ground_height(&m->pos, 0x120, 0.5f);
            if (m->pos.z - g < 3.0f) {
                ApproachValue(&v->v40.f[2], g_frame_time * 8.0f, g_frame_time_v[1] * 10.8f);
            } else {
                ApproachValue(&v->v40.f[2], g_frame_time, g_frame_time_v[1] * 10.8f);
            }
            move_4318_v(m, &v->v40, 0.6f, 0.5f, 0.0f, 0);
            m->anim_speed = 6.0f - (m->pos.z - g) / 6.0f * 5.0f;
            if (g + 6.0f <= m->pos.z) {
                m->pos.z = g + 6.0f;
                if (m->prev_seq != 0) {
                    MobyBlendAnimation(m, 0, 0, scale_ticks(3));
                }
                m->state = 0xE;
                clear_u64_value(&v->v40);
            }
            update_robosquawker_particle((unsigned char *)m, (char *)v);
            break;
        }
        case 0x1:
            FastVecSub(&va, &tgt, &m->pos);
            if (FastVecLength(&va) < v->f1FC + 4.0f && absolute_float(m->pos.z - tgt.pos.f[2]) < 8.0f &&
                random_integer_below(0x13) == 0) {
                m->state = 2;
                if (m->prev_seq != 4) {
                    MobyBlendAnimation(m, 4, 0, scale_ticks(3));
                }
            } else if (m->command == 1 && random_integer_below(0x13) == 0) {
                if (random_integer_below(0x100) & 1) {
                    v->f1F8 = -v->f1F8;
                }
                v->f1F4 = v->f1F8 * 0.017453292f;
                *(int *)&v->h210 = scale_ticks((int)random_float_between(30.0f, 90.0f));
                m->state = 3;
                if (m->prev_seq != 5) {
                    MobyBlendAnimation(m, 5, 0, scale_ticks(3));
                }
                m->command = 0;
            } else if (FastVecLength(&va) > v->f1FC + 6.0f && v->h20A == 0) {
                qzero(&v->v40);
                m->state = 0x10;
                if (m->prev_seq != 0) {
                    MobyBlendAnimation(m, 0, 0, scale_ticks(3));
                }
            }
            update_robosquawker_ground_height((char *)m, (char *)v);
            break;
        case 0x8:
            FastVecSub(&va, &tgt, &m->pos);
            if (FastVecLength(&va) < v->f1FC - 4.0f && absolute_float(m->pos.z - tgt.pos.f[2]) < 8.0f &&
                random_integer_below(0x13) == 0) {
                m->state = 2;
                m->pos.z += 1.0f;
                if (m->prev_seq != 4) {
                    MobyBlendAnimation(m, 4, 0, scale_ticks(3));
                }
            }
            break;
        case 0xA: {
            float a;

            a = fast_atan2(v->v1D0.f[0] - m->pos.x, v->v1D0.f[1] - m->pos.y);
            MobySpringTurn((char *)m, &v->f1F0, a, 0.05f, 0.3f, 0.2f);
            FastVecSub(&va, &tgt, &v->v1D0);
            if ((FastVecLength(&va) < v->f1FC && absolute_float(v->v1D0.f[2] - tgt.pos.f[2]) < 8.0f) ||
                m->command == 2) {
                if (m->group != 0xFF) {
                    alert_group_e81e0(m->group, 2);
                }
                if (m->prev_seq != (func_001160D8() & 1) + 9) {
                    MobyBlendAnimation(m, (func_001160D8() & 1) + 9, 0, scale_ticks(3));
                }
                v->f134 = 0.0f;
                v->f130 = D_E1_L13_0016131C;
                v->f138 = D_E1_L13_00161318 * g_frame_time;
                v->f13C =
                    jump_speed_4318(&m->pos, &v->v1D0, &out, D_E1_L13_00161318 * g_frame_time, -D_E1_L13_0016131C);
                v->w144 = 0xD;
                v->b15D = 0;
                InitKnockback(m, v->x120, m->rot.z, (func_001160D8() & 1) + 9, 1, 0);
                v->f170 = 5.0f;
                v->f174 = 10.0f;
                update_robosquawker_particle((unsigned char *)m, (char *)v);
                m->state = 0xB;
            }
            break;
        }
        case 0xB:
            if (step_moby_motion(m, v->x120) & 1) {
                release_robosquawker_particle((Level13VendorMoby *)v);
                qzero(v->x120);
                m->state = 3;
                if (random_integer_below(0x100) & 1) {
                    v->f1F8 = -v->f1F8;
                }
                v->f1F4 = v->f1F8 * 0.017453292f;
                *(int *)&v->h210 = scale_ticks((int)random_float_between(30.0f, 90.0f));
                if (m->prev_seq != 5) {
                    MobyBlendAnimation(m, 5, 8, scale_ticks(3));
                }
                return;
            } else if (m->pos.z < 5.0f) {
                SetDeathBits(m, 0, -1);
                DeleteMoby(m);
                return;
            }
            break;
        case 0x2: {
            float a;

            a = fast_atan2(m->pos.x - tgt.pos.f[0], m->pos.y - tgt.pos.f[1]);
            MobySpringTurn((char *)m, &v->f1F0, a, 0.02f, 0.3f, 0.1f);
            FastVecSub(&va, &tgt, &m->pos);
            if (FastVecLength(&va) < v->f1FC && absolute_float(m->pos.z - tgt.pos.f[2]) < 8.0f &&
                random_integer_below(0x13) == 0) {
                if (m->group != 0xFF) {
                    alert_group_e81e0(m->group, 1);
                }
                if (random_integer_below(0x100) & 1) {
                    v->f1F8 = -v->f1F8;
                }
                v->f1F4 = v->f1F8 * 0.017453292f;
                *(int *)&v->h210 = scale_ticks((int)random_float_between(30.0f, 90.0f));
                m->state = 3;
                if (m->prev_seq != 5) {
                    MobyBlendAnimation(m, 5, 0, scale_ticks(3));
                }
                m->command = 0;
            } else if (m->command == 1 && random_integer_below(0x13) == 0) {
                if (random_integer_below(0x100) & 1) {
                    v->f1F8 = -v->f1F8;
                }
                v->f1F4 = v->f1F8 * 0.017453292f;
                *(int *)&v->h210 = scale_ticks((int)random_float_between(30.0f, 90.0f));
                m->state = 3;
                if (m->prev_seq != 5) {
                    MobyBlendAnimation(m, 5, 0, scale_ticks(3));
                }
                m->command = 0;
            }
            if (m->prev_seq == 4 && (m->anim_flags & 2)) {
                MobyBlendAnimation(m, 3, 0, scale_ticks(3));
            }
            update_robosquawker_ground_height((char *)m, (char *)v);
            break;
        }
        case 0x3: {
            float r;
            int res;
            float a;
            float d;

            r = absolute_float(m->pos.z - tgt.pos.f[2]);
            r = r / vec_distance_xy(&m->pos, &tgt);
            if (v->w21C != -1) {
                select_list_point_toward_target(m, walker_paths[v->w21C], &tgt, &vb);
            } else {
                qcopy(&vb, &tgt);
            }
            a = fast_atan2(vb.f[0] - m->pos.x, vb.f[1] - m->pos.y) + v->f1F4;
            MobySpringTurn((char *)m, &v->f1F0, a, 0.05f, 0.3f, 0.2f);
            va.f[0] = FastCos(m->rot.z) * 2.0f;
            va.f[1] = FastSin(m->rot.z) * 2.0f;
            va.f[2] = 0.0f;
            res = MobySteerToward(m, &v->w180, &va, &vb, 1.0f);
            if (fast_dec_timer((int *)&v->h210) != 0) {
                *(int *)&v->h210 = scale_ticks((int)random_float_between(30.0f, 90.0f));
                v->f1F4 = -v->f1F4;
            }
            FastVecSub(&vc, &tgt, &v->v1D0);
            d = vec_distance_xy(&m->pos, &tgt);
            if ((d < 1.5f || (tgt.moby != 0 && tgt.moby->hA6 == 0x10E && tgt.moby->state != 0xFE &&
                              tgt.moby->state != 0xFD && d < 2.3f)) &&
                !(res & 2)) {
                m->state = 4;
                if (m->prev_seq != 6) {
                    MobyBlendAnimation(m, 6, 0, scale_ticks(3));
                }
            } else {
                if (r > 1.0f || (res & 2)) {
                    m->pos.x = old.f[0];
                    m->pos.y = old.f[1];
                }
                if (FastVecLength(&vc) < v->f1FC + 4.0f && absolute_float(m->pos.z - tgt.pos.f[2]) < 8.0f) {
                    if (m->group != 0xFF) {
                        alert_group_e81e0(m->group, 1);
                    }
                } else if (m->command != 1) {
                    m->state = 5;
                }
            }
            m->command = 0;
            update_robosquawker_ground_height((char *)m, (char *)v);
            break;
        }
        case 0x4: {
            float a;
            int same;
            float hh;
            float dz;
            float d;
            float da;
            float g;

            a = fast_atan2(tgt.pos.f[0] - m->pos.x, tgt.pos.f[1] - m->pos.y);
            MobySpringTurn((char *)m, &v->f1F0, a, 0.05f, 0.3f, 0.2f);
            same = m->seq == m->prev_seq;
            hh = compute_interpolated_record_value(m);
            dz = absolute_float(m->pos.z - tgt.pos.f[2]);
            d = vec_distance_xy(&m->pos, &tgt);
            da = FastDiffRots(fast_atan2(tgt.pos.f[0] - m->pos.x, tgt.pos.f[1] - m->pos.y), m->rot.z);
            if (same && absolute_float(hh - 13.0f) < 0.2f && dz < 1.5f &&
                (d < 2.0f || (tgt.moby != 0 && tgt.moby->hA6 == 0x10E && tgt.moby->state != 0xFE &&
                              tgt.moby->state != 0xFD && d < 2.6f)) &&
                da < 0.2617994f) {
                va.f[0] = FastCos(m->rot.z) * 0.2f;
                va.f[1] = FastSin(m->rot.z) * 0.2f;
                va.f[2] = 0.0f;
                qcopy(&vb, &tgt);
                vb.f[2] += 0.75f;
                if (tm != 0) {
                    PostHitMessage(tm, (int)m, 1, &vb, &va, 1.0f);
                }
            }
            if ((m->anim_flags & 2) && vec_distance_xy(&m->pos, &tgt) > 1.5f) {
                m->state = 3;
                if (m->prev_seq != 5) {
                    MobyBlendAnimation(m, 5, 0, scale_ticks(3));
                }
            }
            g = probe_ground_height(&m->pos, 0x120, 0.5f);
            v->f198 -= g_frame_time_v[1] * 9.8f;
            m->pos.z += v->f198;
            if (m->pos.z < g) {
                v->f198 = 0.0f;
                m->pos.z = g;
            }
            update_robosquawker_ground_height((char *)m, (char *)v);
            break;
        }
        case 0x5: {
            int res;
            float a;

            if (v->w21C != -1) {
                select_list_point_toward_target(m, walker_paths[v->w21C], &v->v1D0, &vb);
            } else {
                qcopy(&vb, &v->v1D0);
            }
            a = fast_atan2(vb.f[0] - m->pos.x, vb.f[1] - m->pos.y);
            MobySpringTurn((char *)m, &v->f1F0, a, 0.02f, 0.3f, 0.1f);
            va.f[0] = FastCos(m->rot.z) * 2.0f;
            va.f[1] = FastSin(m->rot.z) * 2.0f;
            va.f[2] = 0.0f;
            res = MobySteerToward(m, &v->w180, &va, &vb, 1.0f);
            if (vec_distance_xy(&m->pos, &v->v1D0) < 2.0f) {
                m->state = 1;
                if (m->prev_seq != 3) {
                    MobyBlendAnimation(m, 3, 0, scale_ticks(3));
                }
            }
            FastVecSub(&vc, &tgt, &v->v1D0);
            if (FastVecLength(&vc) < v->f1FC) {
                if (m->group != 0xFF) {
                    alert_group_e81e0(m->group, 1);
                }
                m->state = 3;
                if (m->prev_seq != 5) {
                    MobyBlendAnimation(m, 5, 0, scale_ticks(3));
                }
            } else if (m->command == 1) {
                m->state = 3;
                if (m->prev_seq != 5) {
                    MobyBlendAnimation(m, 5, 0, scale_ticks(3));
                }
            } else if (res & 2) {
                m->pos.x = old.f[0];
                m->pos.y = old.f[1];
                m->state = 0xC;
                if (m->prev_seq != 4) {
                    MobyBlendAnimation(m, 4, 0, scale_ticks(3));
                }
            }
            update_robosquawker_ground_height((char *)m, (char *)v);
            m->command = 0;
            break;
        }
        case 0x6:
            if (step_moby_motion(m, v->x120) & 1) {
                qzero(v->x120);
                m->state = 3;
                if (m->prev_seq != 5) {
                    MobyBlendAnimation(m, 5, 0, scale_ticks(3));
                }
                return;
            } else if (m->pos.z < 5.0f) {
                SetDeathBits(m, 0, -1);
                DeleteMoby(m);
                return;
            }
            break;
        case 0x7:
            release_robosquawker_particle((Level13VendorMoby *)v);
            if (CarriedMobyUpdate(m, v->x120) != 0) {
                m->state = 0xE;
                if (m->prev_seq != 0) {
                    MobyBlendAnimation(m, 0, 0, 0);
                }
                v->hC8 = 0;
            }
            break;
        case 0x63: {
            int t;

            m->flags &= 0xEFFF;
            if (step_moby_motion(m, v->x120) & 0x140) {
                clear_u64_value(&va);
                t = (int)((float)scale_ticks(1) * 60.0f);
                if (reserve_expired_frame_slot(D_E1_L13_00161330, 3, t) >= 0) {
                    cart_burst(m, &m->pos, 4, 0.5f, 13.0f);
                }
                DeleteMoby(m);
                return;
            } else if (m->pos.z < 0.0f) {
                DeleteMoby(m);
                return;
            }
            break;
        }
        case 0xC: {
            float r;
            int res;
            float a;

            r = absolute_float(m->pos.z - tgt.pos.f[2]);
            r = r / vec_distance_xy(&m->pos, &tgt);
            if (v->w21C != -1) {
                select_list_point_toward_target(m, walker_paths[v->w21C], &v->v1D0, &vb);
            } else {
                qcopy(&vb, &v->v1D0);
            }
            a = fast_atan2(vb.f[0] - m->pos.x, vb.f[1] - m->pos.y);
            MobySpringTurn((char *)m, &v->f1F0, a, 0.05f, 0.3f, 0.2f);
            va.f[0] = FastCos(m->rot.z) * 2.0f;
            va.f[1] = FastSin(m->rot.z) * 2.0f;
            va.f[2] = 0.0f;
            res = MobySteerToward(m, &v->w180, &va, &vb, 1.0f);
            FastVecSub(&vc, &tgt, &v->v1D0);
            if (vec_distance_xy(&m->pos, &tgt) < 1.5f && r <= 1.0f) {
                m->state = 4;
                if (m->prev_seq != 6) {
                    MobyBlendAnimation(m, 6, 0, scale_ticks(3));
                }
            } else if (r <= 1.0f && !(res & 2)) {
                m->state = 5;
                if (m->prev_seq != 5) {
                    MobyBlendAnimation(m, 5, 0, scale_ticks(3));
                }
            } else {
                if (m->anim_flags & 2) {
                    int anim;

                    anim = 4;
                    random_integer_below(4);
                    if (random_integer_below(4) != 0) {
                        anim = 3;
                    }
                    MobyBlendAnimation(m, anim, 0, scale_ticks(3));
                }
                if (random_integer_below(0x63) == 0) {
                    qzero(&v->v40);
                    m->state = 0x10;
                    if (m->prev_seq != 0) {
                        MobyBlendAnimation(m, 0, 0, scale_ticks(3));
                    }
                }
            }
            m->command = 0;
            break;
        }
        case 0xD: {
            RsOther *o;
            float g;

            if (m->seq != 5 && m->prev_seq != 5) {
                MobyBlendAnimation(m, 5, 0, scale_ticks(3));
            }
            va.f[0] = FastCos(m->rot.z) * (v->f204 * g_frame_time);
            va.f[1] = FastSin(m->rot.z) * (v->f204 * g_frame_time);
            va.f[2] = 0.0f;
            FastVecAdd(&vb, &m->pos, &va);
            for (o = (RsOther *)level13_moby_list_head; o != 0; o = o->next) {
                if (o == (RsOther *)m) {
                    continue;
                }
                if (o->state < 0) {
                    continue;
                }
                if (o->hA6 != 0 && o->hA6 != 0x3F) {
                    continue;
                }
                if (vec_distance_xy(&vb, &o->pos) < 2.0f) {
                    float z;
                    float a;
                    float d;

                    z = m->pos.z;
                    a = fast_atan2(o->pos.f[0] - m->pos.x, o->pos.f[1] - m->pos.y);
                    a = FastSubRots(m->rot.z, a);
                    d = g_frame_time * 1.5707964f;
                    if (!(a > 0.0f)) {
                        d = -d;
                    }
                    m->rot.z = FastAddRots(m->rot.z, d);
                    FastVecSub(&vb, &m->pos, &o->pos);
                    vec_scale_to_length(&vb, &vb, 2.0f);
                    FastVecAdd(&vb, &vb, &o->pos);
                    vb.f[2] = z;
                }
            }
            qcopy(&m->pos, &vb);
            g = probe_ground_height(&m->pos, 0x120, 0.5f);
            v->f198 -= g_frame_time_v[1] * 9.8f;
            m->pos.z += v->f198;
            if (m->pos.z < g) {
                v->f198 = 0.0f;
                m->pos.z = g;
            }
            if (v->p218 == 0 || vec_distance_xy(&m->pos, v->p218 + 0x10) > IntToFloat(v->h216)) {
                m->state = 5;
                if (m->prev_seq != 5) {
                    MobyBlendAnimation(m, 5, 0, scale_ticks(3));
                }
            }
            m->command = 0;
            break;
        }
    }
    StepColorFade(m, v->x110);
    if (m->pos.x < 2.0f || m->pos.x > 1021.0f || m->pos.y < 2.0f || m->pos.y > 1021.0f || m->pos.z < 2.0f ||
        m->pos.z > 1021.0f) {
        DeleteMoby(m);
        return;
    }
}

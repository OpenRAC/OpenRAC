void decoy_update(Moby *moby) {
    int active = hero_globals_base[6];
    struct DecoyVars *vars = (struct DecoyVars *)moby->pvars;
    char *target, *found, *ground;
    float v[4] __attribute__((aligned(16)));
    float wv[4] __attribute__((aligned(16)));
    float xv[4] __attribute__((aligned(16)));
    float cv[4] __attribute__((aligned(16)));
    float len, len2, angle, rate, effect, fx;
    int i, ok, h5;
    short *ids;

    if (moby->oclass == MOBY_CLASS_DECOY)
        active = 0;
    effect = IntToFloat(active);
    if (moby->pos.x < 2.0f || 1021.0f < moby->pos.x || moby->pos.y < 2.0f || 1021.0f < moby->pos.y ||
        moby->pos.z < 2.0f || 1021.0f < moby->pos.z) {
        moby_index_list_remove(moby, decoy_list);
        DeleteMoby(moby);
        return;
    }
    {
        if (((char *)g_hero_r4.items[0].moby == 0 || *(short *)((char *)g_hero_r4.items[0].moby + 0xA6) != 0x232) &&
            moby->state == 1) {
            moby_index_list_remove(moby, decoy_list);
            DeleteMoby(moby);
            return;
        }
    }
    {
        if (g_hero_r4.vehicle_mode != 0 && moby->state == 1) {
            moby_index_list_remove(moby, decoy_list);
            DeleteMoby(moby);
            return;
        }
    }
    vars->counter = vars->counter + 1;
    if (g_hero_r4.unk20A5 != 0 && moby->state < 2)
        moby->flags = moby->flags | 0x41;
    else
        moby->flags = moby->flags & 0xFFBE;
    if (*(short *)(vars->target + 0xA6) == 0x232 && vars->flag == 0 && FastVecLength(vars->vel_xy) > 0.0f) {
        ground = (char *)g_hero_r4.ground_moby;
        if (ground == 0 || get_moby_pvars_link(ground) == 0) {
            predict_thrown_moby_landing(((char *)vars), (char *)moby, 1);
        } else if (predict_thrown_moby_landing(((char *)vars), (char *)moby, 0) == 0) {
            predict_thrown_moby_landing(((char *)vars), (char *)moby, 2);
        }
    }
    switch (moby->state) {
        case 0:
            moby->state = 1;
            vars->timer = 0;
            moby->collision_disable = -1;
            vars->hold_timer = 0;
            vars->holding = 0;
            break;
        case 1:
            vars->timer = 0;
            {
                char *t = vars->target;
                if (t != 0 && (*(unsigned char *)(t + 0x20) & 0xF0) != 0xF0 &&
                    (t = (char *)g_hero_r4.items[0].moby) != 0 && *(short *)(t + 0xA6) == 0x232)
                    return;
            }
            moby_index_list_remove(moby, decoy_list);
            DeleteMoby(moby);
            return;
        case 2:
            if (moby->scale < *(float *)(*(char **)&moby->pclass + 0x24))
                moby->scale = moby->scale + frame_scale * 0.02f;
            qcopy(v, &moby->pos);
            vars->fall_speed = vars->fall_speed - g_frame_time_sq * 9.0f;
            for (i = 1; i <= ((short *)decoy_list)[0]; i++) {
                ids = (short *)decoy_list + i;
                target = (char *)moby_array_start + *ids * 0x100;
                if (target != ((char *)moby) && *(unsigned char *)(target + 0x20) > 1 && target != 0 &&
                    *(unsigned char *)(target + 0x20) != 0xFE && *(unsigned char *)(target + 0x20) != 0xFD) {
                    FastVecSub(wv, &moby->pos, target + 0x10);
                    wv[2] = 0.0f;
                    {
                        float d = FastVecLength(wv);
                        if (d < 1.8f) {
                            FastVecScale(wv, wv, (1.8f - d) * 0.015f);
                            FastVecAdd(vars->vel_xy, vars->vel_xy, wv);
                        }
                    }
                }
            }
            if (g_hero_r4.state.current == HERO_STATE_LOOK_RELEASE_WAIT)
                *(int *)((char *)g_hero_r4.moby + 0x94) = 0;
            ok = ((int (*)(void *, void *, float, float, float, int))MoveMobyWithCollision)((char *)moby, vars->vel_xy,
                                                                                            0.2f, 0.2f, 0.0f, 0);
            if (ok & 1) {
                qcopy(wv, &moby->pos);
                wv[2] = wv[2] + 0.2f;
                if (coll_sphere(0.23f, wv, 0, moby) != 0) {
                    if (vars->fall_speed < 0.0f) {
                        FastVecSub(xv, &moby->pos, &g_hero_r4.motion.pos);
                        xv[2] = 0.0f;
                        {
                            float d = FastVecLength(xv);
                            if (d < 1.8f) {
                                FastVecScale(xv, xv, (1.8f - d) * 0.015f);
                                FastVecAdd(vars->vel_xy, vars->vel_xy, xv);
                            }
                        }
                    }
                    reflect_vector(vars->vel_xy, vars->vel_xy, &coll_hit_result_r3.normal.f[0]);
                    if (coll_hit_result_r3.moby != 0 && coll_hit_result_r3.unk1C <= 0)
                        FastVecScale(vars->vel_xy, vars->vel_xy, 1.05f);
                    else
                        FastVecScale(vars->vel_xy, vars->vel_xy, 0.2f);
                    len = FastVecLength(vars->vel_xy);
                    if (len < frame_scale * 0.01f ||
                        (g_hero_r4.ground_moby != 0 && get_moby_pvars_link((int)g_hero_r4.ground_moby) != 0 &&
                         FastVecLength(vars->vel_xy) <
                             frame_scale * 0.01f + FastVecLength(&g_hero_r4.motion.displacement))) {
                        angle = fast_atan2(coll_hit_result_r3.normal.f[2],
                                           vector_length_xy(&coll_hit_result_r3.normal.f[0]));
                        if (angle < 0.61086524f) {
                            int t44 = *(unsigned short *)&vars->timer + 1;
                            vars->timer = t44;
                            if (coll_hit_result_r3.moby == 0 || coll_hit_result_r3.unk1C > 0 ||
                                (short)t44 > scale_ticks(240)) {
                                if (moby->prev_seq != 1)
                                    MobyBlendAnimation(moby, 1, 0, scale_ticks(10));
                                *(int *)&moby->collision = *(int *)(*(char **)&moby->pclass + 0x10);
                                moby->collision_disable = 0;
                                qzero(vars->vel_xy);
                                moby->state = 3;
                                vars->timer = (short)scale_ticks(600);
                                moby->pos.z = coll_hit_result_r3.point.f[2];
                                qcopy(xv, &coll_hit_result_r3.normal.f[0]);
                                {
                                    float cos_a, sin_a;
                                    cos_a = FastCos(moby->rot.z);
                                    sin_a = FastSin(moby->rot.z);
                                    cv[0] = xv[0] * cos_a + xv[1] * sin_a;
                                }
                                {
                                    float cos_a, sin_a;
                                    cos_a = FastCos(moby->rot.z);
                                    sin_a = FastSin(moby->rot.z);
                                    cv[1] = xv[1] * cos_a - xv[0] * sin_a;
                                    cv[2] = xv[2];
                                    vars->pitch = -fast_atan2(square_root(cv[0] * cv[0] + cv[2] * cv[2]), cv[1]);
                                    vars->yaw = fast_atan2(cv[2], cv[0]);
                                }
                                rate = 0.1f;
                                moby->rot.x = FastAddRots(moby->rot.x,
                                                          FastSubRots(vars->pitch, moby->rot.x) * (frame_scale * rate));
                                moby->rot.y = FastAddRots(moby->rot.y,
                                                          FastSubRots(vars->yaw, moby->rot.y) * (frame_scale * rate));
                            }
                        }
                    }
                    destroy_decoy_on_selected_materials(moby);
                    if (fast_subtract_rotations_cf(&vars->spin_timer) != 0) {
                        Moby_PlaySound(0, 0, moby);
                        vars->spin_timer = (unsigned char)scale_ticks(20);
                    } else {
                        add_random_offset_to_vector_xyz_v(g_frame_time, vars->vel_xy);
                    }
                }
            }
            if (g_hero_r4.state.current == HERO_STATE_LOOK_RELEASE_WAIT) {
                char *hm = (char *)g_hero_r4.moby;
                *(int *)(hm + 0x94) = *(int *)(*(char **)(hm + 0x24) + 0x10);
            }
            break;
        case 3:
        case 4:
            found = MobyGetHitMessage(moby, 0x30001, 0);
            moby->rot.x = FastAddRots(moby->rot.x, FastSubRots(vars->pitch, moby->rot.x) * (frame_scale * 0.1f));
            moby->rot.y = FastAddRots(moby->rot.y, FastSubRots(vars->yaw, moby->rot.y) * (frame_scale * 0.1f));
            update_decoy_proximity_motion((char *)moby);
            fast_dec_timer(&vars->found_timer);
            FastDecTimer(&vars->hold_timer);
            if (found != 0) {
                if (*(int *)(found + 0x20) != vars->found_id) {
                    vars->found_id = *(int *)(found + 0x20);
                    vars->found_timer = scale_ticks(30);
                } else if (vars->found_timer <= 0) {
                    vars->found_id = *(int *)(found + 0x20);
                    vars->found_timer = scale_ticks(30);
                } else
                    goto skip;
                {
                    if (vars->hold_timer == 0) {
                        if (*(unsigned short *)(found + 0x28) == 0x101 &&
                            (*(unsigned long *)&vars->unk_68 & 0xFFFFFF00000000UL) == 0) {
                            vars->hold_timer = (short)scale_ticks(100);
                            vars->holding = 1;
                        } else {
                            vars->holding = 0;
                        }
                        Moby_PlaySound(0, 0, moby);
                        fx = *(float *)(found + 0x2C);
                        vars->flash = 0xFA;
                        vars->health = vars->health - fx;
                        RerollEffectCounter(moby, ((char *)vars));
                        if (vars->health < 0.0f) {
                            float a = frame_scale * 0.5f;
                            float b = frame_scale * 0.7f;
                            *(u128 *)wv = 0;
                            wv[0] = a;
                            wv[2] = b;
                            wv[1] = a;
                            *(u128 *)v = *(u128 *)wv;
                            add_random_offset_to_vector_xyz_v(a, v);
                            spawn_explosion_v(moby, v, 0, 0.0f, 0.0f, 0, 0, 0, 4.0f, 3.0f, 9.0f, 2, 1.0f, 12.0f, 1, 1,
                                              -1, active);
                            if (active != 0) {
                                initialize_hit_descriptor(effect + effect, wv, moby, 0x810000);
                                *(unsigned char *)((char *)xv + 8) = 2;
                                *(unsigned char *)((char *)xv + 9) = 1;
                                *(unsigned short *)((char *)xv + 10) = *(unsigned short *)&moby->oclass;
                                coll_sphere_mobys(effect * 3.0f, &moby->pos, 0x10, moby, wv);
                            }
                            moby_index_list_remove(moby, decoy_list);
                            DeleteMoby(moby);
                            return;
                        }
                    }
                }
            }
        skip:
            moby->hit_slot = 0xFF;
            StepColorFade(moby, ((char *)vars));
            if ((moby->anim_flags & 2) != 0) {
                if (moby->prev_seq != 2)
                    MobyBlendAnimation(moby, 2, 0, scale_ticks(10));
                moby->anim_speed = random_float_between(0.7f, 1.05f);
            }
            if (moby->state != 4) {
                len = probe_ground_height(&moby->pos, 0, 0.5f);
                if (len == 0.0f)
                    return;
                destroy_decoy_on_selected_materials(moby);
                if (len + 0.02f < moby->pos.z) {
                    vars->fall_speed = vars->fall_speed - g_frame_time_sq * 9.8f;
                    FastVecAdd(&moby->pos, &moby->pos, vars->vel_xy);
                    len = probe_ground_height(&moby->pos, 0, 0.5f);
                    if (moby->pos.z < len) {
                        moby->pos.z = len;
                        qzero(vars->vel_xy);
                        qcopy(v, &coll_hit_result_r3.normal.f[0]);
                        {
                            float cos_a, sin_a;
                            cos_a = FastCos(moby->rot.z);
                            sin_a = FastSin(moby->rot.z);
                            wv[0] = v[0] * cos_a + v[1] * sin_a;
                            cos_a = FastCos(moby->rot.z);
                            sin_a = FastSin(moby->rot.z);
                            wv[1] = v[1] * cos_a - v[0] * sin_a;
                            wv[2] = v[2];
                            vars->pitch = -fast_atan2(square_root(wv[0] * wv[0] + wv[2] * wv[2]), wv[1]);
                            vars->yaw = fast_atan2(wv[2], wv[0]);
                        }
                    }
                } else {
                    qzero(vars->vel_xy);
                }
                if (coll_hit_result_r3.moby != 0)
                    RotateWithMoby(moby, (char *)coll_hit_result_r3.moby, &moby->pos, &moby->rot, &moby->pos,
                                   &moby->rot);
            } else {
                if (moby->scale < *(float *)(*(char **)&moby->pclass + 0x24))
                    moby->scale = moby->scale + frame_scale * 0.02f;
                if (FastDecTimer(&vars->timer) != 0) {
                    if (moby->prev_seq != 0)
                        MobyBlendAnimation(moby, 0, 0, scale_ticks(60));
                    moby_index_list_remove(moby, decoy_list);
                    moby->state = 5;
                }
            }
            break;
        case 5:
            len = probe_ground_height(&moby->pos, 0, 0.5f);
            if (len != 0.0f && coll_hit_result_r3.moby != 0)
                RotateWithMoby(moby, (char *)coll_hit_result_r3.moby, &moby->pos, &moby->rot, &moby->pos, &moby->rot);
            if ((moby->anim_flags & 2) != 0)
                *(int *)&moby->anim_speed = 0;
            if (FastDecTimer(&vars->timer) != 0 || (moby->anim_flags & 2) != 0)
                moby->state = 6;
            break;
        case 6:
            len = probe_ground_height(&moby->pos, 0, 0.5f);
            if (len != 0.0f && coll_hit_result_r3.moby != 0)
                RotateWithMoby(moby, (char *)coll_hit_result_r3.moby, &moby->pos, &moby->rot, &moby->pos, &moby->rot);
            moby->scale = moby->scale - moby->scale * (frame_scale * 0.02f);
            if (!(moby->scale < *(float *)(*(char **)&moby->pclass + 0x24) * 0.1f))
                return;
            DeleteMoby(moby);
            break;
        case 7:
        default:
            moby->state = 0;
            break;
    }
}

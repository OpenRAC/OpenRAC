void visibomb_missile_update(Moby *m) {
    V_2B7430 v20;
    V_2B7430 k30;
    V_2B7430 k40;
    V_2B7430 v50;
    V_2B7430 v60;
    Ev ev;
    V_2B7430 vA0;
    D_2B7430 *d = (D_2B7430 *)m->pvars;
    int direct = 0;

    *(u128_2B7430 *)&k40 = 0;
    k40.z = g_frame_time_r * 8.0f;
    *(u128_2B7430 *)&k30 = *(u128_2B7430 *)&k40;
    *(u128_2B7430 *)&k40 = 0;
    k40.z = 1.0f;
    visibomb_occlusion_skip_flag_r2 = 1;
    D_L00_0015F558_2B7430 = 1;
    jet_camera_mode();
    visibomb_view_active_flag_r2 = 1;
    force_help_message(6, 0);
    d->f10--;
    if (!(-scale_ticks(100) < d->f10) || ((Hero *)g_hero_r3)->state.current == HERO_STATE_HURT ||
        ((Hero *)g_hero_r3)->state.current == HERO_STATE_DYING || ((Hero *)g_hero_r3)->state.current == 0x72 ||
        ((Hero *)g_hero_r3)->state.control_mode == HERO_MODE_FALL ||
        ((Hero *)g_hero_r3)->state.control_mode == HERO_MODE_JUMP ||
        ((Hero *)g_hero_r3)->state.control_mode == HERO_MODE_LEDGE || ((Hero *)g_hero_r3)->state.control_mode > 12 ||
        ((Hero *)g_hero_r3)->state.control_mode == HERO_MODE_HURT || ((Hero *)g_hero_r3)->state.control_mode == 10 ||
        ((Hero *)g_hero_r3)->state.control_mode == 16 || ((Hero *)g_hero_r3)->state.control_mode == HERO_MODE_LAND ||
        ((Hero *)g_hero_r3)->state.current == HERO_STATE_APPROACH_TARGET_POSE ||
        ((Hero *)g_hero_r3)->grounded_frames == 0 ||
        (((Tgt *)((Hero *)g_hero_r3)->ground_moby) != 0 && (((Tgt *)((Hero *)g_hero_r3)->ground_moby)->hA6 == 0x13E ||
                                                            ((Tgt *)((Hero *)g_hero_r3)->ground_moby)->hA6 == 0x46F))) {
        terminate_visibomb_missile_r(m, d);
        return;
    }
    if (d->f10 < 0) {
        if (d->f14 != 0)
            update_orbit_camera_from_target_moby(d->f14, m);
        Moby_RestoreGlobalsState_r(m, 0);
        update_attached_moby_transform_r(m, d);
        return;
    }
    {
        float t = D_L00_0016156C_2B7430 * g_frame_time_r;
        if (((PadState *)g_controller_state_r)->held & 0x40)
            t = D_L00_00161570_2B7430 * g_frame_time_r;
        d->f48 += (t - d->f48) * D_L00_00161574_2B7430;
        d->f4 += (d->f48 - d->f4) * D_L00_00161578_2B7430;
        if (D_L00_00161570_2B7430 * g_frame_time_r < d->f4)
            d->f4 = D_L00_00161570_2B7430 * g_frame_time_r;
    }
    Sound_SetPitchBend(d->f2C, truncate_float_to_s32_r2((d->f48 - D_L00_0016156C_2B7430 * g_frame_time_r /
                                                                      (D_L00_00161570_2B7430 * g_frame_time_r -
                                                                       D_L00_0016156C_2B7430 * g_frame_time_r)) *
                                                        D_L00_00161568_2B7430));
    if (d->f10 > 0) {
        if (d->f10 < scale_ticks(0xBB8) - scale_ticks(5)) {
            float ang;
            float dx = ((Hero *)g_hero_r3)->stick_x - d->f18;
            float dy = ((Hero *)g_hero_r3)->stick_y - d->f1C;
            float ka = frame_scale_r * 0.1f;
            float kb = frame_scale_r * 0.06f;
            float kc = frame_scale_r * 0.0122718466f;
            d->f18 += dx * ka;
            d->f1C += dy * ka;
            m->rot.x += (d->f18 - m->rot.x) * kb;
            ang = FastSubRots(m->rot.y, d->f1C * kc);
            m->rot.y = ang;
            if (1.57079637f <= ang) {
                m->rot.y = 1.56979632f;
            } else if (ang <= -1.57079637f) {
                m->rot.y = -1.56979632f;
            }
            m->rot.z = FastSubRots(m->rot.z, d->f18 * (frame_scale_r * 0.0122718466f));
            jet_animate(m, 0.2f);
        }
    }
    BuildSphericalOffset(&v50, d->f4, m->rot.z, -m->rot.y);
    qcopy(&v20, &m->pos);
    FastVecAdd(&m->pos, &m->pos, &v50);
    if (scale_ticks(0xBB8) - scale_ticks(0x3C) < d->f10) {
        float t = probe_ground_height(&m->pos, 0, 0.5f) + d->f44;
        if (m->pos.z < t) {
            if (g_coll_hit_moby_r2 == 0 || (g_coll_hit_moby_r2->f24 != 0 && g_coll_hit_moby_r2->f24->h46 == 5)) {
                float s = fast_atan2(d->f4, ApproachValue(&m->pos.z, t, g_frame_time_r * 6.0f));
                if (g_frame_time_r * 1.57079637f < s)
                    s = g_frame_time_r * 1.57079637f;
                m->rot.y -= s;
            }
        }
    }
    if (m->pos.x < 4.5f || 1018.5f < m->pos.x || m->pos.y < 4.5f || 1018.5f < m->pos.y || m->pos.z < 4.5f ||
        1018.5f < m->pos.z) {
        terminate_visibomb_missile_r(m, d);
        return;
    }
    update_attached_moby_transform_r(m, d);
    {
        Moby *skip = m;
        build_axis_rotation_quaternion(
            &d->f24->pos, 1, -(d->f1C * 0.5f + absolute_float(d->f18 / 3.0f) + (d->f18 - m->rot.x) * 1.1f) * 0.7f);
        build_axis_rotation_quaternion(
            &d->f28->pos, 1, -(d->f1C * 0.5f + absolute_float(d->f18 / 3.0f) - (d->f18 - m->rot.x) * 1.1f) * 0.7f);
        ev.moby = m;
        ev.flags = 0x830000;
        ev.f1C = 6.0f;
        ev.f20 = 1;
        *(u128_2B7430 *)&ev.pos = *(u128_2B7430 *)&v50;
        vec_set_length_xy(&ev, &ev, 1.0f);
        ev.b18 = 3;
        ev.pos.w = 5627.9248f;
        ev.pos.z = 1.0f;
        ev.b19 = 2;
        ev.h1A = (*(unsigned short *)&m->oclass);
        if (scale_ticks(0xBB8) - 40 < d->f10)
            skip = ((Hero *)g_hero_r3)->moby;
        if (coll_line_r3(&v20, &m->pos, 0, skip, &ev)) {
            if (((Hit_2B7430 *)coll_hit_result)->f18 != 0) {
                direct = 0;
                if (((Hit_2B7430 *)coll_hit_result)->f18 == ((Hero *)g_hero_r3)->moby)
                    direct = 1;
                m->command = 2;
                qcopy(&m->pos, &((Hit_2B7430 *)coll_hit_result)->v20);
                FastVecSub(&v60, &((Hit_2B7430 *)coll_hit_result)->v50, &((Hit_2B7430 *)coll_hit_result)->v60);
                vec_scale_to_length(&v60, &v60, 1.0f);
                reflect_vector(&k30, &v50, &((Hit_2B7430 *)coll_hit_result)->v40);
                vec_scale_to_length(&k30, &k30, g_frame_time_r + g_frame_time_r);
                vec_scale_to_length(&k40, &((Hit_2B7430 *)coll_hit_result)->v40, 1.0f);
                if (d->f2C != -1) {
                    VoicePoolRow *e = (VoicePoolRow *)(voice_pool_r + d->f2C * 0x70);
                    if (e->owner == m) {
                        if (e->state != 0)
                            Sound_StopSlot(d->f2C);
                    }
                }
                d->f2C = -1;
            } else if (((Hit_2B7430 *)coll_hit_result)->f1C > 0) {
                m->command = 1;
                qcopy(&m->pos, &((Hit_2B7430 *)coll_hit_result)->v20);
                FastVecSub(&v60, &((Hit_2B7430 *)coll_hit_result)->v50, &((Hit_2B7430 *)coll_hit_result)->v60);
                vec_scale_to_length(&v60, &v60, 1.0f);
                reflect_vector(&k30, &v50, &((Hit_2B7430 *)coll_hit_result)->v40);
                vec_scale_to_length(&k30, &k30, g_frame_time_r + g_frame_time_r);
                vec_scale_to_length(&k40, &((Hit_2B7430 *)coll_hit_result)->v40, 1.0f);
                if (d->f2C != -1) {
                    VoicePoolRow *e = (VoicePoolRow *)(voice_pool_r + d->f2C * 0x70);
                    if (e->owner == m) {
                        if (e->state != 0)
                            Sound_StopSlot(d->f2C);
                    }
                }
                d->f2C = -1;
            }
        } else if (d->f10 == 0) {
            terminate_visibomb_missile_r(m, d);
            return;
        }
    }
    if (d->f10 < scale_ticks(0xBB8) - scale_ticks(0x3C)) {
        if (((PadState *)g_controller_state_r)->pressed & 0x20) {
            m->command = 1;
            if (d->f2C != -1) {
                VoicePoolRow *e = (VoicePoolRow *)(voice_pool_r + d->f2C * 0x70);
                if (e->owner == m) {
                    if (e->state != 0)
                        Sound_StopSlot(d->f2C);
                }
            }
            d->f2C = -1;
        }
    }
    if (m->command != 0) {
        int k;
        int rr;
        if (direct == 0) {
            spawn_explosion(m, &k30, 0, 0.0f, 0.0f, 10, 3, 16, 4.0f, 2.0f, 0.0f, 1.0f, 1, 20.0f, 1, 1, -1, 0);
            rr = coll_sphere_mobys_r2(&m->pos, 4.0f, 16, m, 0);
            *(u128_2B7430 *)&vA0 = *(u128_2B7430 *)((V_2B7430 *)&m->pos);
            SendRadialMessageToMobys(m, &vA0, coll_sphere_moby_hits, rr, 0, 0x830000, 3, 2, 6.0f, 1.0f, 1.0f);
        }
        m->flags |= 1;
        d->f4 = frame_scale_r * 0.0199999996f;
        d->f18 = d->f18 * 0.00999999978f;
        if (scale_ticks(0xBB8) - scale_ticks(0x1E) < d->f10) {
            d->f10 = -scale_ticks(100);
        } else {
            d->f10 = -1;
        }
        qcopy(&vA0, &m->pos);
        for (k = 4; k >= 0; k--) {
            qcopy(&m->pos, &vA0);
            add_random_offset_to_vector_xyz(&m->pos, 1.5f);
            spawn_moby_particle_and_glow_burst_r(m, 1);
            qcopy(&m->pos, &vA0);
        }
        v50.x = 0.0f;
        m->command = 0;
    }
    if (d->f14 != 0)
        update_orbit_camera_from_target_moby(d->f14, m);
}

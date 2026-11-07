void follow_camera_apply_overrides(O_y *o) {
    S220_y *s;
    S40_y *q;
    S1D0_y *t;
    M_y *m;
    A_y *a;
    A_y *x;
    f32 v[4];
    f32 f, sum;
    s32 flag = 0;
    s32 n, i;
    s16 k;
    s32 *lp;
    u8 *e;

    s = (S220_y *)((u8 *)o->m + 0x220);
    if (hero74.unk2284 == 0xD && s->i10 != 0x14D) {
        f = FastAddRots(hero74.ledge_normal_yaw, 3.14159265f);
        v[0] = FastCos(f);
        v[1] = FastSin(f);
        v[2] = 0.0f;
        set_follow_camera_angle_blend_from_direction(v, 0.20943952f, v[2]);
        follow_camera_start_slide();
    }
    if (hero74.state.control_mode == HERO_MODE_SWIM) {
        o->m->fF0 = 0.25f;
        follow_camera_set_height(0.5f, 0.003f);
    }
    if (hero74.state.control_mode == HERO_MODE_LAND) {
        flag = 1;
        {
            S40_y *q5 = (S40_y *)((u8 *)o->m + 0x40);
            if (hero74.vehicle_mode == 1) {
                q5->fB0 = 0.25f;
                follow_camera_set_height(1.5f, 0.003f);
            } else {
                q5->fB0 = 0.25f;
                follow_camera_set_height(2.5f, 0.003f);
            }
        }
    }
    if (hero74.state.control_mode == 0x10)
        o->m->fF0 = 0.25f;
    if (hero74.vehicle_mode == 1) {
        if (s->i0 == 0) {
            q = (S40_y *)((u8 *)o->m + 0x40);
            follow_camera_set_distance(3.0f, 0.003f, 1);
            if (flag == 0) {
                q->fB0 = 1.0f;
                follow_camera_set_height(1.0f, 0.003f);
            }
        }
        *(f32 *)((u8 *)o->m + 0x20C) = 0.6f;
    }
    if (hero74.vehicle_mode == 2) {
        if (s->i10 < 2) {
            follow_camera_set_offset_scale(0, g68, 0.005f);
            follow_camera_set_distance(g6C, 0.003f, 1);
            follow_camera_set_height(g70, 0.003f);
        }
        set_follow_camera_flag_226();
        t = (S1D0_y *)((u8 *)o->m + 0x1D0);
        {
            f32 u = g78, w = g74;
            t->f3C = u;
            t->f44 = w;
        }
    }
    if (hero74.unk20AA != 0 && hero74.state.control_mode != HERO_MODE_GRIND) {
        set_follow_camera_orbit_clamp_mode(0);
        follow_camera_start_slide();
        follow_camera_set_lateral_spring(0.04f, 0.2f);
    }
    x = &player_camera_y[1];
    if (x->fn != 0)
        x->fn();
    a = &player_camera_y[0];
    if (a->p25C != 0 && s->i10 < 2) {
        if ((s8)a->p25C[0x20] < 0) {
            a->p25C = 0;
        } else {
            if (0.3f <= absolute_float(D_0013C940_y.f100) || 0.3f <= absolute_float(D_0013C940_y.f104)) {
                a->i260 = 0;
            }
            x = &player_camera_y[0];
            x->i260++;
            if (scale_ticks(400) < x->i260)
                x->i260 = scale_ticks(400);
            f = IntToFloat(x->i260) / IntToFloat(scale_ticks(400));
            set_follow_camera_angle_blend_from_position((s32)(x->p25C + 0x10), f * 0.20943952f, 0.0f);
        }
    }
    s->i10 = 0;
    n = coll_sphere_mobys(15.0f, &D_0013F3D0_y, 1, (void *)D_0013F3D0_y.i2000, 0);
    if (n > 0) {
        lp = (s32 *)coll_sphere_moby_hits;
        for (i = 0; i < n; lp++, i++) {
            e = GetFlaggedPvarWord(*lp);
            if (e != 0 && (f32)e[0xD] != 0.0f) {
                s->i10 = 1;
                break;
            }
        }
    }
    if (hero74.state.current == HERO_STATE_DELAYED_JUMP_OR_R1_REPEAT_EXIT_WAIT && (D_0013CAE0_y[0] & 3)) {
        set_follow_camera_orbit_clamp_mode(0);
        follow_camera_start_slide();
        follow_camera_set_lateral_spring(0.04f, 0.2f);
        {
            M_y *mm = o->m;
            mm->h16 = 0;
            mm->h12 = 0;
            mm->h14 = 0;
        }
        return;
    }
    if (g64 == 0)
        return;
    m = o->m;
    sum = 0.0f;
    {
        s32 j = 0, next;
        f32 *qq = v;
        char *gc = camera_view;
        f32 *gq = (f32 *)gc + 44;
        do {
            next = j + 1;
            *qq = FastDiffRots(gq[j - 1], gq[j]);
            sum += *qq;
            qq++;
            j = next;
        } while (j < 4);
    }

    sum *= 0.25f;
    {
        f32 c = sum * 5.0f;
        if (1.0f < c)
            c = 1.0f;
        c = 1.0f - c;
        if (c < g54)
            m->h12 = scale_ticks(0x20);
    }
    if (m->h12 != 0) {
        if (FastDecTimer(&m->h12))
            m->h14 = scale_ticks(0x2D);
        follow_camera_start_slide();
        k = ++m->h16;
        if (scale_ticks(0x1E) < k)
            m->h16 = scale_ticks(0x1E);
        f = IntToFloat(m->h16) / IntToFloat(scale_ticks(0x1E));
        follow_camera_set_lateral_spring((g58 - 0.015f) * f + 0.015f, 0.2f);
        g40 = 0.0f;
    } else if (m->h14 != 0) {
        FastDecTimer(&m->h14);
        f = IntToFloat(m->h14) / IntToFloat(scale_ticks(0x2D));
        f *= -0.7f;
        f += 0.7f;
        g40 = f;
        m->h16 = 0;
    } else {
        m->h16 = 0;
        g40 = 0.7f;
    }
}

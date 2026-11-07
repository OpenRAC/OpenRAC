void update_hero_and_equipped_item_glows(void) {
    Vec4 v;
    Moby *m;
    int i;
    int c, g;

    if (((Hero *)D_0013F450_2086C8)->items[2].item_id == 5) {
        Moby *w = ((Hero *)D_0013F450_2086C8)->items[2].moby;
        if (w != 0 && ((w->flags ^ 1) & 1)) {
            for (i = 0; i < 2; i++) {
                moby_point_position(w, i, &v);
                HeroDrawQueue_Add_r(&v, 0x301EAA1E, 0.057f, 0.15f);
            }
        }
    }

    ((Hero *)D_0013F450_2086C8)->glow_phase =
        FastAddRots(((Hero *)D_0013F450_2086C8)->glow_phase, g_frame_time_r * 2.26892805f);
    ((Hero *)D_0013F450_2086C8)->moby->glow_color =
        FastTweenColor(0x806E8C6E, 0x805A3232, FastSin(((Hero *)D_0013F450_2086C8)->glow_phase) * 0.5f + 0.5f);

    m = ((Hero *)D_0013F450_2086C8)->items[0].moby;
    if (m == 0)
        return;
    if (!((m->flags ^ 1) & 1))
        return;

    switch (((Hero *)D_0013F450_2086C8)->items[0].item_id) {
        case 17: {
            int c5;
            int x1, x2, col;
            float fr;
            float f = FastAddRots(((Hero *)D_0013F450_2086C8)->items[0].glow_phase, g_frame_time_r * 4.36332321f);
            c5 = 0x28C8C8C8;
            ((Hero *)D_0013F450_2086C8)->items[0].glow_phase = f;
            if (0.0f < f && f < 2.26892805f)
                c5 = 0x280A0A0A;
            m->glow_color = FastTweenColor(m->glow_color, c5, 0.07f);
            v.q = 0;
            v.f[0] = 0.0173452497f;
            v.f[1] = -0.0391772911f;
            v.f[2] = 0.0962345004f;
            transform_vector_by_basis(&v, &v, &m->rot_row0);
            FastVecAdd(&v, &v, &m->pos);
            c = (*(unsigned char *)&m->glow_color);
            g = ((unsigned char *)&m->glow_color)[1];
            fr = (float)c;
            c = ((unsigned char *)&m->glow_color)[2];
            x1 = truncate_float_to_s32_r(fr * 0.2f);
            x2 = truncate_float_to_s32_r((float)c * 0.2f);
            col = ((g << 5) / 200) << 24;
            col |= x2 << 16;
            g <<= 8;
            col |= g;
            col |= x1;
            HeroDrawQueue_Add_r(&v, col, 0.15f, 0.1f);
            break;
        }
        case 16:
            if (scale_ticks(50) < ((Hero *)D_0013F450_2086C8)->items[0].ready_frames) {
                float k, f;
                int r, g, b, a, col, col1, bs, gs;
                k = g_frame_time_r * 3.14159274f;
                if (((Hero *)D_0013F450_2086C8)->item_anim_active != 0)
                    k = g_frame_time_r * 4.71238899f;
                f = FastAddRots(((Hero *)D_0013F450_2086C8)->items[0].glow_phase, k);
                ((Hero *)D_0013F450_2086C8)->items[0].glow_phase = f;
                r = truncate_float_to_s32_r(FastSin(f) * 20.0f) + 30;
                g = truncate_float_to_s32_r(FastSin(f) * 90.0f) + 150;
                b = truncate_float_to_s32_r(FastSin(f) * 20.0f) + 30;
                if (((Hero *)D_0013F450_2086C8)->item_anim_active != 0) {
                    int tmp = r;
                    r = g;
                    g = tmp;
                }
                bs = b << 16;
                col1 = bs | 0x20000000;
                gs = g << 8;
                col1 |= gs;
                col1 |= r;
                m->glow_color = FastTweenColor(m->glow_color, col1, 0.13f);
                a = ((Hero *)D_0013F450_2086C8)->items[0].ready_frames - scale_ticks(50);
                if (a > 32)
                    a = 32;
                col = (a << 24) | bs | gs | r;
                moby_point_position(m, 1, &v);
                HeroDrawQueue_Add_r(&v, col, 0.15f, 0.04f);
            }
            break;
        case 19: {
            float k, f;
            int r, g, b, col;
            k = g_frame_time_r * 2.26892805f;
            if (((Hero *)D_0013F450_2086C8)->item_anim_active != 0)
                k = g_frame_time_r * 7.33038282f;
            f = FastAddRots(((Hero *)D_0013F450_2086C8)->items[0].glow_phase, k);
            ((Hero *)D_0013F450_2086C8)->items[0].glow_phase = f;
            if (((Hero *)D_0013F450_2086C8)->item_anim_active != 0) {
                r = truncate_float_to_s32_r(FastSin(f) * 30.0f) + 50;
                g = truncate_float_to_s32_r(FastSin(f) * 30.0f) + 50;
                b = truncate_float_to_s32_r(FastSin(f) * 90.0f) + 170;
            } else {
                r = truncate_float_to_s32_r(FastSin(f) * 70.0f) + 100;
                g = truncate_float_to_s32_r(FastSin(f) * 40.0f) + 70;
                b = truncate_float_to_s32_r(FastSin(f) * 40.0f) + 70;
            }
            col = b << 16;
            col |= 0x80000000;
            col |= g << 8;
            col |= r;
            m->glow_color = FastTweenColor(m->glow_color, col, 0.15f);
            break;
        }
        case 20: {
            int c5;
            int x1, x2, col;
            float fr;
            float f = FastAddRots(((Hero *)D_0013F450_2086C8)->items[0].glow_phase, g_frame_time_r * 4.36332321f);
            c5 = 0xDCDCDC;
            ((Hero *)D_0013F450_2086C8)->items[0].glow_phase = f;
            if (0.0f < f && f < 2.44346094f)
                c5 = 0xA0A0A;
            m->glow_color = FastTweenColor(m->glow_color, c5, 0.07f);
            v.q = 0;
            v.f[0] = 0.0171042103f;
            v.f[1] = -0.0939194188f;
            v.f[2] = 0.140954122f;
            transform_vector_by_basis(&v, &v, &m->rot_row0);
            FastVecAdd(&v, &v, &m->pos);
            c = (*(unsigned char *)&m->glow_color);
            g = ((unsigned char *)&m->glow_color)[1];
            fr = (float)c;
            c = ((unsigned char *)&m->glow_color)[2];
            x1 = truncate_float_to_s32_r(fr * 0.2f);
            x2 = truncate_float_to_s32_r((float)c * 0.2f);
            col = ((g << 5) / 200) << 24;
            col |= x2 << 16;
            g <<= 8;
            col |= g;
            col |= x1;
            HeroDrawQueue_Add_r(&v, col, 0.17f, 0.1f);
            break;
        }
        case 13: {
            int c5;
            float f = FastAddRots(((Hero *)D_0013F450_2086C8)->items[0].glow_phase, g_frame_time_r * 4.71238899f);
            c5 = 0x2814D214;
            ((Hero *)D_0013F450_2086C8)->items[0].glow_phase = f;
            if (0.0f < f && f < 2.09439516f)
                c5 = 0x281414D2;
            m->glow_color = FastTweenColor(m->glow_color, c5, frame_scale_r * 0.05f);
            moby_point_position(m, 0, &v);
            HeroDrawQueue_Add_r(&v, m->glow_color, 0.1f, 0.03f);
            break;
        }
    }
}

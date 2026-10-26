void update_path_following_moby(char *m) {
    float a[4];
    float b[4];
    char *d = *(char **)(m + 0x78);
    char *pos = m + 0x10;
    int *t;
    qcopy(a, pos);
    t = (int *)batalia_path_table[*(int *)d];
    switch (*(unsigned char *)(m + 0x20)) {
    case 0:
        if (*(int *)d == -1) {
            DeleteMoby(m);
            return;
        }
        {
            float l0 = FastVecDist((char *)t + 0x10, (char *)t + 0x20);
            FastVecDist((char *)t + 0x10, (char *)t + (*t << 4));
            *(float *)(d + 8) = *(float *)&batalia_path_mover_speed * g_frame_time / l0;
            *(float *)(d + 4) = *(float *)(d + 4) * (float)*t;
            *(float *)(d + 0xC) = randf_sym(0.0f, 5.0f);
        }
        m[0x20] = 1;
        *(unsigned char *)(m + 0x30) = 0xFF;
    case 1: {
        float f;
        SamplePathAt(t, 1, m + 0x10, (float *)(m + 0x40), 0, *(float *)(d + 4));
        *(float *)(m + 0x44) = *(float *)(m + 0x44) * 0.5f;
        *(float *)(m + 0x18) = *(float *)(m + 0x18) + *(float *)(d + 0xC);
        *(float *)(d + 4) = *(float *)(d + 4) + *(float *)(d + 8);
        f = (float)*t;
        if (f < *(float *)(d + 4)) {
            *(float *)(d + 4) = *(float *)(d + 4) - f;
        }
        break;
    }
    case 2: {
        float c15 = 15.0f;
        float c10 = 10.0f;
        float dd;
        float g;
        char *v = d + 0x10;
        g = g_frame_time * 7.3303828f;
        *(float *)(d + 0x18) = *(float *)(d + 0x18) - g_frame_time_sq * c15;
        dd = FastAddRots(*(float *)(m + 0x40), g);
        *(float *)(m + 0x40) = *(float *)(m + 0x40) + dd;
        *(float *)(m + 0x44) = -fast_atan2(vec_length_xy(v), *(float *)(d + 0x18));
        FastVecAdd(pos, pos, v);
        if (*(float *)(m + 0x18) < c10) {
            DeleteMoby(m);
            return;
        }
        if (coll_sphere(1.25f, pos, 0, m)) {
            spawn_explosion(m, v, pos, 0.0f, 0.0f, 0x14, 0x28, 0x10, c10, 5.0f, 9.0f, 1.0f, -1, c15, 1, 1, -1, 0);
            DeleteMoby(m);
            return;
        }
        break;
    }
    default:
        break;
    }
    if (*(float *)(m + 0x10) < 4.0f || *(float *)(m + 0x14) < 4.0f || *(float *)(m + 0x18) < 4.0f) {
        *(int *)(m + 0x94) = 0;
    } else {
        *(int *)(m + 0x94) = *(int *)(*(char **)(m + 0x24) + 0x10);
    }
    if (MobyGetHitMessage(m, 0x10000, 0) != 0 && *(unsigned char *)(m + 0x20) != 2) {
        if (hero_current_state == 0x32 && gSkillPoints[0xC] == 0) {
            gSkillPoints[0xC] = 1;
            PlayBankSound(1, 0, 0);
            ShowBanner(0x542D, -1);
        }
        FastVecSub(b, m + 0x10, a);
        qcopy(d + 0x10, b);
        *(float *)(d + 0x18) = *(float *)(d + 0x18) + (g_frame_time + g_frame_time);
        spawn_explosion(m, b, m + 0x10, 0.0f, 0.0f, 0x14, 0x28, 0x10, 10.0f, 5.0f, 9.0f, 1.0f, -1, 15.0f, 1, 1, -1, 0);
        m[0x20] = 2;
    }
    *(unsigned char *)(m + 0xA4) = 0xFF;
    if (*(unsigned char *)(m + 0x31) != 0) {
        spawn_moby_motion_spark(m, (char *)a);
    }
}

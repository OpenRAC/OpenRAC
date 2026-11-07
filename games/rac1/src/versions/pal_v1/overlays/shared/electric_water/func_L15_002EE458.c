extern int D_E1_L15_0015ED84 MACRO_ADDR;
extern int D_E1_0015F5A4 MACRO_ADDR;
extern Moby *D_E1_L15_0015FF58 MACRO_ADDR;
extern Moby *pads_gp SDATA(D_E1_L15_0015FF58);
extern int D_E1_L15_0015F574 SDATA(D_E1_L15_0015F574);
void giant_clank_pad_update(char *m) {
    struct GiantClankPadVars *data = (struct GiantClankPadVars *)((Moby *)m)->pvars;
    Moby *other;
    Moby *p0;
    int ns;
    char *o3;
    unsigned char *x;
    unsigned char *b;
    float V[4];
    float f;
    int st;
    int idx;
    int v;
    int gm;

    st = ((unsigned char *)m)[0x20];
    switch (st) {
        case 0:
            idx = data->target_idx;
            if (idx == -1) {
                DeleteMoby(m);
                return;
            }
            p0 = &pads_gp[idx];
            p0->flags = p0->flags | 1;
            p0->collision = 0;
            p0->visible = 0;
            if (D_E1_L15_0015ED84 != LEVEL_VELDIN2 ||
                (b = saved_mission_state, x = b + ((unsigned char *)m)[0xB0], x[0x120] != 0xFF)) {
                ns = 1;
                m[0x20] = ns;
            } else {
                ns = 3;
                m[0x20] = ns;
            }
            data->unk_68 = -1;
            return;
        case 1:
            m[0x20] = 2;
            return;
        case 2:
            if ((char *)g_hero_r.ground_moby != m)
                break;
            if (g_hero_r.air_frames.s != 0)
                break;
            if (D_E1_0015F5A4 == st)
                break;
            if (g_hero_r.state.current == 0x1D)
                break;
            v = g_hero_r.vehicle_mode;
            if (v == 0) {
                try_set_help_message(4, 0x3AA4);
            } else if (v == st) {
                if (D_E1_L15_0015ED84 != LEVEL_VELDIN2)
                    try_set_help_message(4, 0x3AA5);
            }
            if ((g_controller_state.pressed & PAD_TRIANGLE) == 0)
                return;
            if (D_E1_L15_0015F574 != 4)
                return;
            if (g_hero_r.vehicle_mode == 2 && (gm = D_E1_L15_0015ED84) == 0xF) {
                other = D_E1_L15_0015FF58;
                other += data->target_idx;
                other->flags = other->flags | 1;
                other->collision = 0;
                other->visible = 0;
                restore_saved_hero_moby_and_clear_vehicle_mode();
                MobySetAnimation(other, 0, 0);
                if (D_E1_L15_0015ED84 == gm) {
                    start_scene(9);
                    request_music_track(0, 5);
                }
                return;
            }
            if (g_hero_r.vehicle_mode != 0)
                return;

            other = D_E1_L15_0015FF58;
            other += data->target_idx;
            *(u128_2EE458 *)V = *(u128_2EE458 *)(m + 0x10);
            f = probe_ground_height(V, 0, 0.5f);
            V[2] = f + 0.1f;
            set_race_camera(V, m + 0x40, 0, 1);
            o3 = (char *)other->pclass;
            other->collision = *(int *)(o3 + 0x10);
            other->visible = 1;
            other->flags = other->flags & 0xFFFE;
            enter_hero_vehicle_mode(2, 0x5A, (char *)other);
            if (D_E1_L15_0015ED84 == LEVEL_QUARTU) {
                start_scene(8);
                *(u128_2EE458 *)g_hero_r.saved_pos_rot = *(u128_2EE458 *)&g_hero_r.motion.pos;
                *(u128_2EE458 *)(g_hero_r.saved_pos_rot + 0x10) = *(u128_2EE458 *)&g_hero_r.motion.rot;
                request_music_track(2, 4);
                return;
            }
            if (D_E1_L15_0015ED84 == LEVEL_VELDIN2) {
                start_scene(6);
                *(u128_2EE458 *)g_hero_r.saved_pos_rot = *(u128_2EE458 *)&g_hero_r.motion.pos;
                *(u128_2EE458 *)(g_hero_r.saved_pos_rot + 0x10) = *(u128_2EE458 *)&g_hero_r.motion.rot;
                m[0x20] = 3;
            }
            return;
        default:
            return;
    }
    data->unk_68 = -1;
}

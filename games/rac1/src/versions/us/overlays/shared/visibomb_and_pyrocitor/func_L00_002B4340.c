void visibomb_update(char *m) {
    int *p;
    struct Hero *g;
    char *pad;
    char *g2;
    float t0[4];
    float t1[4];
    float f20, f21;
    if (m == 0)
        return;
    p = ((int *)((Moby *)m)->pvars);
    fast_dec_timer(p);
    g = &g_hero;
    pad = (char *)&g_controller_state;
    if ((*(int *)(pad + 0x1A4) & g->items[0].button_mask) != 0 && p[0] == 0 && p[1] == 0 && g->item_use_locked == 0) {
        int st = g->state.control_mode;
        if (st != 7 && st != 2 && st != 3 && st != 4 && st != 5 && st != 6 && st != 0x10 && st != 0xE && st != 9 &&
            st != 0xA && st != 0xB && st < 0xD) {
            int w = g->state.current;
            if (w != 0x72 && w != 0x65) {
                int x;
                g2 = gCameraLvl;
                x = *(int *)(g2 + 0x180);
                if (x == 0 || *(short *)(x + 0x86) != 5) {
                    if (g->grounded_frames != 0) {
                        int y = *(int *)&g->ground_moby;
                        if (y == 0 || (*(short *)(y + 0xA6) != 0x13E && *(short *)(y + 0xA6) != 0x46F)) {
                            if (use_ammo(-1, 1) != 0) {
                                f21 = 0;
                                f20 = g->held_pose_yaw;
                                if (g->state.current == HERO_STATE_LOOK &&
                                    (*(unsigned short *)(*(int *)&g->moby + 0x34) & 1)) {
                                    char *v = g2 + 0x350;
                                    f20 = fast_atan2(*(float *)(g2 + 0x350), *(float *)(g2 + 0x354));
                                    f21 = -fast_atan2(vector_length_xy(v), *(float *)(g2 + 0x358));
                                    vec_scale_to_length(t0, v, 0.3f);
                                    vec_scale_to_length(t1, g2 + 0x370, -0.35f);
                                    FastVecAdd(t0, t0, t1);
                                    FastVecAdd(t0, t0, g2 + 0x140);
                                } else {
                                    struct Hero *q = &g_hero;
                                    if (q->unk22C8 == 1) {
                                        f21 = fast_atan2(vector_length_xy(&((Moby *)m)->rot_row1),
                                                         ((Moby *)m)->rot_row1.z);
                                        BuildSphericalOffset(t0, 0.6f, f20, -f21);
                                        FastVecAdd(t0, t0, &((Moby *)m)->pos);
                                    } else {
                                        BuildSphericalOffset(t0, 0.6f, f20, -f21);
                                        FastVecAdd(t0, t0, &((Moby *)m)->pos);
                                    }
                                }
                                if (((Moby *)m)->prev_seq != 3)
                                    MobyBlendAnimation(m, 3, 0, 0);
                                jet_camera_mode();
                                p[1] = spawn_visibomb_missile(m, t0, f20, f21);
                                {
                                    char *h = help_stats_base + 0x65;
                                    if (*(unsigned short *)(h + 0x68) <= 0xFFFE) {
                                        *(unsigned short *)(h + 0x68) = *(unsigned short *)(h + 0x68) + 1;
                                    }
                                    if (scale_ticks(gameplay_event_frame_count) / 600 > *(unsigned short *)(h + 0x6A)) {
                                        *(unsigned short *)(h + 0x6A) = scale_ticks(gameplay_event_frame_count) / 600;
                                    }
                                    *(unsigned int *)(h + 0x6C) =
                                        *(unsigned int *)(h + 0x6C) | (1 << g_current_level_index) | 0x80000000;
                                    p[0] = scale_ticks(0x3C);
                                }
                            } else if ((*(int *)(pad + 0x1A4) & g->items[0].button_mask) != 0) {
                                Moby_PlaySound(0, 0, (int)m);
                            }
                        }
                    }
                }
            }
        }
    }
    if ((((Moby *)m)->anim_flags & 2) != 0 && ((Moby *)m)->prev_seq != 1)
        MobyBlendAnimation(m, 1, 0, 0);
    ((Moby *)m)->flags |= 4;
}

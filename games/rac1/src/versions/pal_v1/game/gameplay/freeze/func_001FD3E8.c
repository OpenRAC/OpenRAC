void update_mode_freeze(void) {
    Freeze *q;
    Freeze *r5;
    char *dp1, *dp2;
    char *h1, *h2, *h3, *h4;
    int t;
    int p1, p2, p3, p4, p5, p6, p7, p8, p9, pa;
    char *m, *m2;
    char *c;
    int v;

    ((int (*)(void))sound_update)();
    q = &R;
    if (q->x4 != 0) {
        q->x4--;
    }
    switch (R.kind) {
        case 5:
            R.x20++;
            t = scale_game_frame_count(0x5A);
            if (t < R.x20 && R.x24 != 0) {
                R.x24--;
            }
            r5 = &R;
            t = scale_game_frame_count(0x78);
            if (t < r5->x20 && (PAD & 0x40)) {
                g_game_mode = 0;
                if (g_current_level_index == LEVEL_NOVALIS) {
                    memcard_Save(0, -1);
                }
            }
            break;
        case 4:
            p1 = PAD;
            if (p1 & 0x10) {
                h1 = g_hero;
                m = *(char **)&((struct Hero *)h1)->moby;
                m[0x31] = 0;
                *(int *)(m + 0x94) = 0;
                *(unsigned short *)(m + 0x34) |= 1;
                Profile_Mark_v();
                noop_callback_k(((struct Hero *)h1)->saved_pos_rot, h1 + 0x1D10, 0, 1);
                g_game_mode = 0;
            } else if (p1 & 0x40) {
                g_game_mode = 0;
            }
            break;
        case 6:
            switch (R.x1C) {
                case 0:
                    if (R.x4 == 0 || --R.x4 == 0) {
                        R.x1C = 1;
                    }
                    break;
                case 1:
                    p2 = PAD;
                    if (p2 & 0x10) {
                        g_game_mode = R.x14;
                    } else if (p2 & 0x40) {
                        FadeToBlack(4);
                        requested_video_mode[0] = 0;
                        R.x1C = 2;
                        R.x20 = scale_game_frame_count(0x258);
                    }
                    break;
                case 2:
                    if (R.x20 == 0 || --R.x20 == 0) {
                        R.x1C = 3;
                        requested_video_mode[0] = 1;
                        break;
                    }
                    p3 = PAD;
                    if (p3 & 0x10) {
                        FadeToBlack(4);
                        g_game_mode = R.x14;
                        requested_video_mode[0] = 1;
                    } else if (p3 & 0x40) {
                        g_game_mode = R.x14;
                        requested_video_mode[0] = 0;
                    }
                    break;
                case 3:
                    if (PAD & 0x40) {
                        g_game_mode = R.x14;
                    }
                    break;
                default:
                    g_game_mode = R.x14;
                    break;
            }
            break;
        case 1:
            p4 = PAD;
            occl_fallback_mode = 2;
            if (p4 & 0x10) {
                g_game_mode = 0;
                h2 = g_hero;
                h2[0x160F] |= 1;
            } else if (p4 & 0x40) {
                g_game_mode = 0;
            }
            break;
        case 2:
            p5 = PAD;
            occl_fallback_mode = 2;
            if (p5 & 0x40) {
                g_game_mode = 0;
            }
            break;
        case 0:
            switch (R.x1C) {
                case 0:
                    if (R.x20 < 8) {
                        R.x20++;
                    } else if (R.x24 < 8) {
                        R.x24++;
                    } else {
                        R.x1C = 1;
                    }
                    break;
                case 1:
                    p6 = PAD;
                    if (p6 & 0x40) {
                        R.x1C = 2;
                    } else if (p6 & 0x820) {
                        R.x1C = 3;
                    }
                    break;
                case 2:
                case 3:
                    if (R.x24 != 0) {
                        R.x24--;
                        break;
                    }
                    if (R.x20 != 0) {
                        R.x20--;
                        break;
                    }
                    v = R.x1C;
                    if (v == 2) {
                        h3 = g_hero;
                        p7 = *(int *)(h3 + 0x880);
                        if (p7 != -1) {
                            set_animation_parameter(p7, 0);
                            *(int *)(h3 + 0x880) = -1;
                        }
                        decrement_resource_counter();
                        t = scale_game_frame_count(0x1068);
                        if (t < *(int *)(h3 + 0x19C)) {
                            if (g_current_level_index == LEVEL_RILGAR) {
                                rilgar_hoverboard_races_finished = rilgar_hoverboard_races_finished + 1;
                            } else if (g_current_level_index == LEVEL_KALEBO3) {
                                kalebo3_hoverboard_races_finished = kalebo3_hoverboard_races_finished + 1;
                            }
                        }
                        g_game_mode = 0;
                        noop_callback_k(D_00141150, D_00141150 + 0x10, 0, 1);
                        break;
                    }
                    g_game_mode = 0;
                    h4 = g_hero;
                    if (*(short *)(h4 + 0x89A) >= 3) {
                        *(short *)(h4 + 0x89A) = *(unsigned short *)(h4 + 0x89A) - 1;
                        m2 = *(char **)(h4 + 0x890);
                        m2[0xBC] = 3;
                    }
                    break;
            }
            break;
        case 3:
            r5 = &R;
            r5->x20++;
            t = scale_game_frame_count(0x1E);
            if (t < r5->x20 && r5->x24 != 0) {
                r5->x24--;
            }
            switch (mode_freeze_state) {
                case 2:
                    if (R.x4 == 0 && (PAD & 0x40)) {
                        mode_freeze_flags &= ~1;
                        g_game_mode = R.x14;
                    }
                    break;
                case 6:
                    if (PAD & 0x20) {
                        mode_freeze_flags |= 8;
                        break;
                    }
                    if (PAD & 0x10) {
                        {
                            int fl = mode_freeze_flags;
                            fl |= 0x20;
                            fl &= ~2;
                            fl &= ~4;
                            mode_freeze_flags = fl;
                        }
                        v = boot_level_active;
                        if (v == 0) {
                            g_game_mode = R.x14;
                        }
                    }
                    break;
                case 13:
                    if (PAD & 0x20) {
                        mode_freeze_flags |= 0x10;
                        break;
                    }
                    if (PAD & 0x10) {
                        {
                            int fl = mode_freeze_flags;
                            fl |= 0x20;
                            fl &= ~2;
                            fl &= ~4;
                            mode_freeze_flags = fl;
                        }
                        if (boot_level_active == 0) {
                            g_game_mode = R.x14;
                        }
                    }
                    break;
                case 17:
                case 18:
                case 20:
                case 21:
                    if (PAD & 0x40) {
                        g_game_mode = R.x14;
                        {
                            int fl = mode_freeze_flags;
                            fl ^= 0x40;
                            fl &= ~2;
                            fl &= ~4;
                            mode_freeze_flags = fl;
                        }
                    }
                    break;
                case 16:
                    g_game_mode = R.x14;
                    pause_current_action_list[0] = R.x18;
                    break;
                case 12:
                    if (mode_freeze_flags & 2) {
                        break;
                    }
                    /* fall through */
                case 3:
                case 5:
                    if (PAD & 0x10) {
                        {
                            int fl = mode_freeze_flags;
                            fl &= ~2;
                            fl &= ~4;
                            mode_freeze_flags = fl;
                        }
                        g_game_mode = R.x14;
                    }
                    break;
                case 19:
                    if (boot_level_active != 0 && (mode_freeze_flags & 2)) {
                        p9 = PAD;
                        if (p9 & 0x40) {
                            goto restart;
                        }
                        if (p9 & 0x10) {
                            {
                                int fl = mode_freeze_flags;
                                fl &= ~2;
                                fl &= ~4;
                                mode_freeze_flags = fl;
                            }
                            g_game_mode = R.x14;
                        }
                        break;
                    }
                    if (PAD & 0x10) {
                        {
                            int fl = mode_freeze_flags;
                            fl &= ~2;
                            fl &= ~4;
                            mode_freeze_flags = fl;
                        }
                        g_game_mode = R.x14;
                    }
                    break;
                case 23:
                case 24:
                    pa = PAD;
                    if (pa & 0x40) {
                    restart:
                        load_and_initialize_level_chunk();
                        {
                            int fl = mode_freeze_flags;
                            fl &= ~2;
                            fl &= ~4;
                            fl |= 0x20;
                            mode_freeze_flags = fl;
                        }
                        request_space_load(0);
                        screen_globals_base[0] = 1;
                        break;
                    }
                    if (pa & 0x10) {
                        mode_freeze_flags |= 0x20;
                        g_game_mode = R.x14;
                    }
                    break;
                case 4:
                    if (boot_level_active != 0) {
                        if (mode_freeze_flags & 2) {
                            c = g_controller_state;
                            if (*(int *)(c + 0x1A4) & 0x40) {
                                load_and_initialize_level_chunk();
                                {
                                    int fl = mode_freeze_flags;
                                    fl &= ~2;
                                    fl &= ~4;
                                    mode_freeze_flags = fl;
                                }
                                request_space_load(0);
                                screen_globals_base[0] = 1;
                            }
                            if (*(int *)(c + 0x1A4) & 0x10) {
                                {
                                    int fl = mode_freeze_flags;
                                    fl |= 0x20;
                                    fl &= ~2;
                                    fl &= ~4;
                                    mode_freeze_flags = fl;
                                }
                                g_game_mode = R.x14;
                            }
                        } else if (mode_freeze_flags & 4) {
                            if (PAD & 0x10) {
                                {
                                    int fl = mode_freeze_flags;
                                    fl |= 0x20;
                                    fl &= ~2;
                                    fl &= ~4;
                                    mode_freeze_flags = fl;
                                }
                                g_game_mode = R.x14;
                            }
                        } else {
                            g_game_mode = R.x14;
                        }
                        break;
                    }
                    if (PAD & 0x10) {
                        {
                            int fl = mode_freeze_flags;
                            fl |= 0x20;
                            fl &= ~2;
                            fl &= ~4;
                            mode_freeze_flags = fl;
                        }
                        g_game_mode = R.x14;
                    }
                    break;
                case 9:
                    if (boot_level_active == 0 || (mode_freeze_flags & 6) != 0) {
                        break;
                    }
                    g_game_mode = R.x14;
                    break;
            }
            break;
    }
    if ((unsigned int)(g_game_mode - 3) >= 2) {
        snd_ContinueAllSoundsInGroup(0x1D);
        music_Unpause();
        snd_FlushSoundCommands();
    }
}

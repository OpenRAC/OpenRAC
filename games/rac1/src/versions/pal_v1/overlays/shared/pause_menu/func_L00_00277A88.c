void update_pause_menu(void) {
    Own *u8p;
    Mth *m;
    NO *obj;
    int v;
    int i, tmp;
    int flag;
    int k;
    int j;
    NO **objs;
    struct MemoryCardState *q;

    GS.progress = GS.progress + 1;
    switch (GS.state) {
        case 0x2:
        case 0x1A:
        case 0x1E:
        case 0x1F:
            level_game_clock_paused = 1;
            break;
    }
    if (GS.state == 0x14) {
        if (GS.unk14 != 0) {
            GS.unk14 = GS.unk14 - 1;
            if (GS.unk14 != 0) {
                return;
            }
        }
        if (*(short *)(cd_read_state + 8) != 0) {
            return;
        }
        level_vu1_chain_reserved_bytes = 0x1E000;
        select_level_vu_chain();
        qcopy(g_debug_camera, &GS.v40);
        qcopy(g_debug_camera + 4, &GS.v50);
        render_view_tan_half_fov = GS.unkE8;
        select_moby_context(0);
        if (GS.unk140 != 0) {
            loaded_swap_moby_class_set = -1;
            select_world_object_resource_tables(GS.unk140, GS.unk144);
        }
        UpdateFog();
        skip_frame = 1;
        GS.unk108 = 0;
        GS.unk10C = 0;
        GS.unk104 = 0;
        GS.unk10 = 0;
        if (GS.unkC == 2) {
            char *w = map_menu_state;
            STUB_printf(level_transition_target_fmt, *(int *)(w + 0x224));
            prepare_target_level_transition(*(int *)(w + 0x224));
        } else if (GS.unkC == 3) {
            tmp = decode_mode;
            decode_mode = 2;
            GS.unk130 = tmp;
            FadeToBlack(scale_ticks(0x10));
            cinematic_mode = 0;
            PlayMovieEntryIfIdle((int)GS.unkE4);
        } else if (GS.unkC == 4) {
            tmp = decode_mode;
            decode_mode = 1;
            GS.unk130 = tmp;
            FadeToBlack(scale_ticks(0x10));
            cinematic_mode = 0;
            queue_movie_with_game_language((int)GS.unkE4);
        } else if (GS.unkC == 6) {
            tmp = decode_mode;
            decode_mode = 1;
            GS.unk130 = tmp;
            FadeToBlack(scale_ticks(0x10));
            cinematic_mode = 0;
            queue_language_zero_movie((int)GS.unkE4);
        } else if (GS.unkC == 5) {
            tmp = decode_mode;
            decode_mode = 1;
            GS.unk130 = tmp;
            FadeToBlack(scale_ticks(0x10));
            cinematic_mode = 0;
            start_scene((int)GS.unkE4);
        } else if (GS.unkC == 7) {
            tmp = decode_mode;
            GS.unk130 = tmp;
            FadeToBlack(scale_ticks(0x10));
            cinematic_mode = 0;
            start_slideshow();
        } else {
            cinematic_mode = 0;
        }
        snd_ContinueAllSoundsInGroup(0x1D);
        if (GS.unkC != 2) {
            music_Unpause();
        }
        snd_FlushSoundCommands();
        sound_update();
        return;
    }
    v = GS.state;
    if (v == 0 || v == 0xA || v == 0xE || v == 0x11 || v == 0x21 || v == 0x2D || v == 0x23) {
        initialize_pause_menu();
    }
    if ((mode_freeze_flags & 1) && !(cinematic_mode_frames < 8)) {
        ((void (*)(int, int))mode_freeze_init)(3, GS.owner);
        return;
    }
    process_unlock_code_input();
    if (GS.state == 1) {
        GS.unk14 = (GS.unk14 < 1) ? 0 : GS.unk14 - 1;
        if (GS.unk14 == 0) {
            u8p = GS.unk8;
            GS.unk8 = 0;
            GS.owner = u8p;
            GS.state = u8p->unk3C;
            for (i = 0; i < 14; i++) {
                obj = GS.owner->objs[i];
                if (obj != 0 && obj->f8 != 0) {
                    obj->f8(obj, 0);
                }
            }
            goto L2;
        }
    } else if (GS.unk8 != 0) {
        if (GS.owner == GS.unk8) {
            Moby_PlaySound(3, 0x11, l00_pause_menu_page_mobys[0]);
        } else {
            Moby_PlaySound(4, 0x11, l00_pause_menu_page_mobys[0]);
        }
        for (i = 0; i < 14; i++) {
            obj = GS.owner->objs[i];
            if (obj != 0 && obj->fC != 0) {
                obj->fC(obj, 0);
            }
        }
        flag = (GS.unk8 == GS.owner->unk38);
        if (GS.owner == GS.unk8 && GS.owner != &pause_menu_state_21_page && GS.owner != &pause_menu_no_toggle_page_4 &&
            GS.owner != &pause_menu_no_toggle_page_5 &&
            GS.owner != &l00_pause_menu_self_transition_animation_guard_page_owner &&
            GS.owner != &pause_menu_no_toggle_page_3 && GS.owner != &pause_menu_no_toggle_page_2) {
            if (GS.state != 0x23) {
                flag = flag ^ 1;
            }
        }
        for (j = 0; j < 14; j++) {
            if (GS.unk8->objs[j] != 0) {
                GS.unk8->objs[j]->f14 = l00_pause_menu_page_mobys[j];
            }
            if (flag) {
                int n = GS.owner->ids[j];
                HEnt *e = l00_pause_menu_page_mobys[j];
                MobySetAnimation(e, n, e->unk24->unk48[n]->unk10 - 1);
                l00_pause_menu_page_mobys[j]->unk58 = -1.0f;
            } else {
                MobySetAnimation(l00_pause_menu_page_mobys[j], GS.unk8->ids[j], 0);
                l00_pause_menu_page_mobys[j]->unk58 = 1.0f;
            }
        }
        GS.state = 1;
        GS.unk14 = 12;
        GS.unkD0 = GS.owner;
        GS.owner = 0;
    }
L2:
    if (GS.owner != 0) {
        objs = GS.owner->objs;
        for (k = 0; k < 14; k++) {
            if (objs != 0 && objs[k] != 0 && objs[k]->f0 != 0) {
                if (objs[k]->f0(objs[k]) != 0) {
                    GS.unkC = ((unsigned int)(GS.state - 15) < 2) ? 2 : 1;
                }
            }
        }
        if (GS.owner != 0 && GS.owner->unk80 != 0) {
            m = GS.owner->unk40;
            if (m->fC != 0) {
                m->fC(m, 1);
            }
            GS.owner->unk40 = GS.owner->unk80;
            GS.owner->unk80 = 0;
            m = GS.owner->unk40;
            if (m->f8 != 0) {
                m->f8(m, 1);
            }
        }
    }
    update_visible_resident_objects();
    sound_update();
    if (GS.unkC != 0) {
        close_pause_menu();
    }
}

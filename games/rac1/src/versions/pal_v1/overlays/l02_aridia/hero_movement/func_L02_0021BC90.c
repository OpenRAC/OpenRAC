void process_hero_ground_material_effects(void) {
    struct Hero *g = &g_hero;
    int keep = g->ground_sound_class;
    int st = g->ground_type;
    FastMemZero16(&g->ground_type, 0x10);
    g->ground_sound_class = keep;
    g->ground_type = -1;
    g->wading = 0;
    g->unk308 = 0;
    if (st == -1)
        return;
    if (st == 2) {
        if (g->frames_off_ground.s == 0 || g->ground_distance < 0.3f) {
            g->on_magnet_ground = 1;
            if (g->vehicle_mode == 0) {
                char *p = *(char **)&g->items[1].moby;
                if (p != 0 && *(short *)(p + 0xA6) == 0xAD) {
                    g->unk308 = 1;
                }
            }
        }
    }
    if (st == 3) {
        struct Hero *q = &g_hero;
        q->selector_3 = 1;
    }
    if (g_current_level_index == LEVEL_GEMLIK) {
        struct Hero *q = &g_hero;
        if (q->state.current != HERO_STATE_HEIGHT_OR_TIMEOUT_RESPAWN_WAIT) {
            if (coll_sphere(q->capsule_radius + 0.03f, &q->motion.body_center, 2, 0) != 0) {
                if (CollType() == 0xB) {
                    HeroL02_SetMoveState(HERO_STATE_HEIGHT_OR_TIMEOUT_RESPAWN_WAIT, 1);
                    return;
                }
            }
        }
    }
    if (st == 0xB) {
        struct Hero *q = &g_hero;
        q->selector_11 = 1;
    }
    if (st == 2) {
        struct Hero *q = &g_hero;
        q->on_magnet_ground = 1;
    }
    if (st == 0xD) {
        struct Hero *q = &g_hero;
        q->selector_13 = 1;
    }
    if (st == 8) {
        struct Hero *q = &g_hero;
        q->on_slip_ground = 1;
    }
    if (st == 9) {
        struct Hero *q = &g_hero;
        q->unk12EE = 1;
    }
    if (st == 0xC) {
        struct Hero *q = &g_hero;
        q->on_slip_ground = 1;
    }
    {
        struct Hero *q = &g_hero;
        if (q->selector_13 != 0 && q->state.current != HERO_STATE_SINK) {
            if (absolute_float(q->water_surface_z - (q->motion.pos.f[2] + 0.25f)) < 1.0f) {
                if (q->water_surface_z - q->motion.pos.f[2] > 0.0f) {
                    if (q->motion.displacement.f[2] < 0.0f) {
                        exit_hero_vehicle_mode_if_mode_three();
                        HeroL02_SetMoveState(HERO_STATE_SINK, 1);
                        return;
                    }
                }
            }
        }
    }
    {
        struct Hero *h = &g_hero;
        if (h->selector_3 != 0 && h->state.current != HERO_STATE_WAIT_FOR_JUMP_OR_RESPAWN &&
            h->state.current != HERO_STATE_HEIGHT_OR_TIMEOUT_RESPAWN_WAIT) {
            if (absolute_float(h->sink_surface_z - (h->motion.pos.f[2] + 0.25f)) < 1.0f) {
                if (h->sink_surface_z - h->motion.pos.f[2] > 0.0f) {
                    if (h->state.current != HERO_STATE_JUMP_FROM_RESPAWN_WAIT || h->past_jump_apex != 0) {
                        if (h->motion.displacement.f[2] < 0.0f) {
                            exit_hero_vehicle_mode_if_mode_three();
                            if (h->health.hp == 0)
                                HeroL02_SetMoveState(HERO_STATE_HEIGHT_OR_TIMEOUT_RESPAWN_WAIT, 1);
                            else
                                HeroL02_SetMoveState(HERO_STATE_WAIT_FOR_JUMP_OR_RESPAWN, 1);
                            return;
                        }
                    }
                }
            }
        }
    }
    {
        struct Hero *h = &g_hero;
        if (h->selector_11 != 0 && h->state.current != HERO_STATE_HEIGHT_OR_TIMEOUT_RESPAWN_WAIT) {
            if (absolute_float(h->sink_surface_z - (h->motion.pos.f[2] + 0.25f)) < 1.0f) {
                if (h->sink_surface_z - h->motion.pos.f[2] > 0.0f) {
                    if (h->motion.displacement.f[2] < 0.0f) {
                        exit_hero_vehicle_mode_if_mode_three();
                        HeroL02_SetMoveState(HERO_STATE_HEIGHT_OR_TIMEOUT_RESPAWN_WAIT, 1);
                    }
                }
            }
        }
    }
}

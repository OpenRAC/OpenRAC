void Hero_UpdateAimFlags(void) {
    struct Hero *g = &g_hero;
    int c = g->ground_sound_class;
    int s = g->ground_type;
    int t;
    FastMemZero16(&g->ground_type, 0x10);
    g->ground_sound_class = c;
    g->ground_type = -1;
    g->wading = 0;
    g->unk308 = 0;
    if (s == -1)
        return;
    if (s == 2) {
        if (g->frames_off_ground.s == 0 || g->ground_distance < 0.3f) {
            g->on_magnet_ground = 1;
            if (g->vehicle_mode == 0) {
                struct Moby *m = (struct Moby *)(*(char **)&g->items[1].moby);
                if (m != 0 && m->oclass == 0xAD) {
                    g->unk308 = 1;
                }
            }
        }
    }
    if (s == 0xE) {
        struct Hero *p = &g_hero;
        p->base_condition = 1;
        p->water_depth = 0.2f;
        p->water_surface_z = p->ground_z.f + 0.2f;
    }
    if (s == 0) {
        struct Hero *p = &g_hero;
        float d = p->water_surface_z - p->ground_z.f;
        p->water_depth = d;
        if (d < 0.85f && 0.25f < d) {
            p->wading = 1;
        }
        p = &g_hero;
        p->base_condition = 1;
    }
    if (s == 3) {
        struct Hero *p = &g_hero;
        p->selector_3 = 1;
    }
    if (g_current_level_index == LEVEL_GEMLIK) {
        struct Hero *p = &g_hero;
        if (p->state.current != HERO_STATE_HEIGHT_OR_TIMEOUT_RESPAWN_WAIT) {
            if (coll_sphere(p->capsule_radius + 0.03f, &p->motion.body_center, 2, 0) != 0) {
                if (CollType() == 0xB) {
                    HeroQuartu_TrySetMoveState(HERO_STATE_HEIGHT_OR_TIMEOUT_RESPAWN_WAIT, 1);
                    return;
                }
            }
        }
    }
    if (s == 0xB) {
        struct Hero *p = &g_hero;
        p->selector_11 = 1;
    }
    if (s == 4) {
        struct Hero *p = &g_hero;
        p->selector_4 = 1;
    }
    if (s == 8) {
        struct Hero *p = &g_hero;
        p->on_slip_ground = 1;
    }
    if (s == 9) {
        struct Hero *p = &g_hero;
        p->unk12EE = 1;
    }
    if (s == 0xC) {
        struct Hero *p = &g_hero;
        p->on_slip_ground = 1;
    }
    {
        struct Hero *p = &g_hero;
        if (p->selector_4 != 0 && p->grounded_frames != 0) {
            t = p->state.control_mode;
            if (t != 0x10 && t != 0x14 && t != 7) {
                HeroQuartu_TrySetMoveState(HERO_STATE_VOICE_SLOT_CLEANUP_IDLE_EXIT, 1);
                return;
            }
        }
    }
    {
        struct Hero *p = &g_hero;
        if (p->selector_3 != 0) {
            t = p->state.current;
            if (t != 0x68 && t != 0x7B) {
                if (absolute_float(p->sink_surface_z - (p->motion.pos.f[2] + 0.25f)) < 1.0f) {
                    if (0.0f < p->sink_surface_z - p->motion.pos.f[2]) {
                        if (p->state.current != HERO_STATE_JUMP_FROM_RESPAWN_WAIT || p->past_jump_apex != 0) {
                            if (p->motion.displacement.f[2] < 0.0f) {
                                exit_hero_vehicle_mode_if_mode_three();
                                if (p->health.hp != 0) {
                                    HeroQuartu_TrySetMoveState(HERO_STATE_WAIT_FOR_JUMP_OR_RESPAWN, 1);
                                } else {
                                    HeroQuartu_TrySetMoveState(HERO_STATE_HEIGHT_OR_TIMEOUT_RESPAWN_WAIT, 1);
                                }
                                return;
                            }
                        }
                    }
                }
            }
        }
    }
    {
        struct Hero *p = &g_hero;
        if (p->selector_11 != 0) {
            if (p->state.current != HERO_STATE_HEIGHT_OR_TIMEOUT_RESPAWN_WAIT) {
                if (absolute_float(p->sink_surface_z - (p->motion.pos.f[2] + 0.25f)) < 1.0f) {
                    if (0.0f < p->sink_surface_z - p->motion.pos.f[2]) {
                        if (p->motion.displacement.f[2] < 0.0f) {
                            exit_hero_vehicle_mode_if_mode_three();
                            HeroQuartu_TrySetMoveState(HERO_STATE_HEIGHT_OR_TIMEOUT_RESPAWN_WAIT, 1);
                        }
                    }
                }
            }
        }
    }
}

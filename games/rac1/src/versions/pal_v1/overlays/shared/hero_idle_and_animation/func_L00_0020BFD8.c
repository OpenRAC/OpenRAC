void apply_hero_selected_flags_and_state_transitions(void) {
    char *g = (char *)hero_globals_base + 0xE1D;
    unsigned char keep = *(unsigned char *)(g + 0x12ED);
    int s = *(short *)(g + 0x12E0);
    FastMemZero16(g + 0x12E0, 0x10);
    *(unsigned char *)(g + 0x12ED) = keep;
    *(short *)(g + 0x12E0) = -1;
    *(unsigned char *)(g + 0x20A9) = 0;
    *(short *)(g + 0x308) = 0;
    if (s == -1)
        return;
    if (s == 2) {
        if (*(short *)(g + 0x30C) == 0 || *(float *)(g + 0x2DC) < 0.3f) {
            *(unsigned char *)(g + 0x12E7) = 1;
            if (*(unsigned char *)(g + 0x20A4) == 0) {
                int p = *(int *)(g + 0x10E0);
                if (p != 0 && *(short *)(p + 0xA6) == 0xAD) {
                    *(short *)(g + 0x308) = 1;
                }
            }
        }
    }
    if (s == 0xE) {
        struct Hero *g = (struct Hero *)((char *)hero_globals_base + 0xE1D);
        g->base_condition = 1;
        g->water_depth = 0.2f;
        g->water_surface_z = g->ground_z.f + 0.2f;
    }
    if (s == 0) {
        struct Hero *g = (struct Hero *)((char *)hero_globals_base + 0xE1D);
        float d = g->water_surface_z - g->ground_z.f;
        g->water_depth = d;
        if (d < 0.85f && 0.25f < d) {
            g->wading = 1;
        }
        g = (struct Hero *)((char *)hero_globals_base + 0xE1D);
        g->base_condition = 1;
    }
    if (s == 3) {
        struct Hero *g = (struct Hero *)((char *)hero_globals_base + 0xE1D);
        g->selector_3 = 1;
    }
    if (g_current_level_index_m == LEVEL_GEMLIK) {
        struct Hero *g = (struct Hero *)((char *)hero_globals_base + 0xE1D);
        if (g->state.current != HERO_STATE_HEIGHT_OR_TIMEOUT_RESPAWN_WAIT) {
            if (coll_sphere(g->capsule_radius + 0.03f, &g->motion.body_center, 2, 0)) {
                if (CollType() == 0xB) {
                    HeroEnterMoveState(HERO_STATE_HEIGHT_OR_TIMEOUT_RESPAWN_WAIT, 1);
                    return;
                }
            }
        }
    }
    if (s == 0xB) {
        struct Hero *g = (struct Hero *)((char *)hero_globals_base + 0xE1D);
        g->selector_11 = 1;
    }
    if (s == 2) {
        struct Hero *g = (struct Hero *)((char *)hero_globals_base + 0xE1D);
        g->on_magnet_ground = 1;
    }
    if (s == 4) {
        struct Hero *g = (struct Hero *)((char *)hero_globals_base + 0xE1D);
        g->selector_4 = 1;
    }
    if (s == 7) {
        struct Hero *g = (struct Hero *)((char *)hero_globals_base + 0xE1D);
        g->on_ice_ground = 1;
    }
    if (s == 0xD) {
        struct Hero *g = (struct Hero *)((char *)hero_globals_base + 0xE1D);
        g->selector_13 = 1;
    }
    if (s == 1) {
        struct Hero *g = (struct Hero *)((char *)hero_globals_base + 0xE1D);
        g->selector_1 = 1;
    }
    if (s == 8) {
        struct Hero *g = (struct Hero *)((char *)hero_globals_base + 0xE1D);
        g->on_slip_ground = 1;
    }
    if (s == 9) {
        struct Hero *g = (struct Hero *)((char *)hero_globals_base + 0xE1D);
        g->unk12EE = 1;
    }
    if (s == 0xC) {
        struct Hero *g = (struct Hero *)((char *)hero_globals_base + 0xE1D);
        g->on_slip_ground = 1;
    }
    {
        struct Hero *g = (struct Hero *)((char *)hero_globals_base + 0xE1D);
        if (g->selector_4 != 0 && g->grounded_frames != 0) {
            int v = g->state.control_mode;
            if (v != 0x10 && v != 0x14 && v != 7) {
                HeroEnterMoveState(HERO_STATE_VOICE_SLOT_CLEANUP_IDLE_EXIT, 1);
                return;
            }
        }
    }
    {
        char *g = (char *)hero_globals_base + 0xE1D;
        if (*(unsigned char *)(g + 0x12E5) != 0) {
            if (*(int *)(g + 0x300) != 0 && *(unsigned char *)(g + 0x20A4) == 1 && *(int *)(g + 0x2084) != 0x7D) {
                HeroEnterMoveState(0x7D, 1);
                return;
            }
            {
                struct Hero *g = (struct Hero *)((char *)hero_globals_base + 0xE1D);
                if (g->selector_1 != 0) {
                    if (g->grounded_frames != 0) {
                        if (!(g->state.current == 0x3C && g->past_jump_apex == 0)) {
                            exit_hero_vehicle_mode_if_mode_three();
                            if (g->health.hp != 0) {
                                HeroEnterMoveState(0x3C, 1);
                                return;
                            }
                            HeroEnterMoveState(HERO_STATE_DEATH_DESCENT, 1);
                            return;
                        }
                    }
                }
            }
        }
    }
    {
        struct Hero *g = (struct Hero *)((char *)hero_globals_base + 0xE1D);
        if (g->selector_13 != 0 && g->state.current != HERO_STATE_SINK) {
            if (absolute_float(g->water_surface_z - (g->motion.pos.f[2] + 0.25f)) < 1.0f &&
                g->water_surface_z - g->motion.pos.f[2] > 0.0f && g->motion.displacement.f[2] < 0.0f) {
                exit_hero_vehicle_mode_if_mode_three();
                HeroEnterMoveState(HERO_STATE_SINK, 1);
                return;
            }
        }
    }
    {
        struct Hero *g = (struct Hero *)((char *)hero_globals_base + 0xE1D);
        if (g->selector_3 != 0 && g->state.current != HERO_STATE_WAIT_FOR_JUMP_OR_RESPAWN &&
            g->state.current != HERO_STATE_HEIGHT_OR_TIMEOUT_RESPAWN_WAIT) {
            if (absolute_float(g->sink_surface_z - (g->motion.pos.f[2] + 0.25f)) < 1.0f &&
                g->sink_surface_z - g->motion.pos.f[2] > 0.0f) {
                if ((g->state.current != HERO_STATE_JUMP_FROM_RESPAWN_WAIT || g->past_jump_apex != 0) &&
                    g->motion.displacement.f[2] < 0.0f) {
                    exit_hero_vehicle_mode_if_mode_three();
                    if (g->health.hp != 0) {
                        HeroEnterMoveState(HERO_STATE_WAIT_FOR_JUMP_OR_RESPAWN, 1);
                        return;
                    }
                    HeroEnterMoveState(HERO_STATE_HEIGHT_OR_TIMEOUT_RESPAWN_WAIT, 1);
                    return;
                }
            }
        }
    }
    {
        struct Hero *g = (struct Hero *)((char *)hero_globals_base + 0xE1D);
        if (g->selector_11 != 0 && g->state.current != HERO_STATE_HEIGHT_OR_TIMEOUT_RESPAWN_WAIT) {
            if (absolute_float(g->sink_surface_z - (g->motion.pos.f[2] + 0.25f)) < 1.0f &&
                g->sink_surface_z - g->motion.pos.f[2] > 0.0f && g->motion.displacement.f[2] < 0.0f) {
                exit_hero_vehicle_mode_if_mode_three();
                HeroEnterMoveState(HERO_STATE_HEIGHT_OR_TIMEOUT_RESPAWN_WAIT, 1);
                return;
            }
        }
    }
    return;
}

void HeroL05_ProcessContactType(void) {
    char *g = g_hero;
    unsigned char keep = GB(0x12ED);
    int s = GH(0x12E0);
    FastMemZero16(g + 0x12E0, 0x10);
    GB(0x12ED) = keep;
    GH(0x12E0) = -1;
    GB(0x20A9) = 0;
    GH(0x308) = 0;
    if (s == -1)
        return;
    if (s == 2) {
        if (GH(0x30C) == 0 || GF(0x2DC) < 0.3f) {
            GB(0x12E7) = 1;
            if (GB(0x20A4) == 0) {
                int p = GW(0x10E0);
                if (p != 0 && *(short *)(p + 0xA6) == 0xAD) {
                    GH(0x308) = 1;
                }
            }
        }
    }
    if (s == 0xE) {
        char *g = g_hero;
        GB(0x12E4) = 1;
        GF(0x22A4) = 0.2f;
        GF(0x2F0) = GF(0x2D8) + 0.2f;
    }
    if (s == 0) {
        char *g = g_hero;
        float d = GF(0x2F0) - GF(0x2D8);
        GF(0x22A4) = d;
        if (d < 0.85f && 0.25f < d) {
            GB(0x20A9) = 1;
        }
        g = g_hero;
        GB(0x12E4) = 1;
    }
    if (s == 3) {
        char *g = g_hero;
        GB(0x12E6) = 1;
    }
    if (g_current_level_index_m == LEVEL_GEMLIK) {
        char *g = g_hero;
        if (GW(0x2084) != 0x7B) {
            if (coll_sphere(GF(0x234) + 0.03f, g + 0xD0, 2, 0)) {
                if (CollType() == 0xB) {
                    Hero_TryMoveStateL05(HERO_STATE_HEIGHT_OR_TIMEOUT_RESPAWN_WAIT, 1);
                    return;
                }
            }
        }
    }
    if (s == 0xB) {
        char *g = g_hero;
        GB(0x12EB) = 1;
    }
    if (s == 5) {
        char *g = g_hero;
        GB(0x12E8) = 1;
    }
    if (s == 6) {
        char *g = g_hero;
        GB(0x12E9) = 1;
    }
    if (s == 4) {
        char *g = g_hero;
        GB(0x12E3) = 1;
    }
    if (s == 0xD) {
        char *g = g_hero;
        GB(0x12EC) = 1;
    }
    if (s == 1) {
        char *g = g_hero;
        GB(0x12E5) = 1;
    }
    if (s == 8) {
        char *g = g_hero;
        GB(0x12EA) = 1;
    }
    if (s == 9) {
        char *g = g_hero;
        GB(0x12EE) = 1;
    }
    if (s == 0xC) {
        char *g = g_hero;
        GB(0x12EA) = 1;
    }
    {
        char *g = g_hero;
        if (GB(0x12E3) != 0 && GW(0x300) != 0) {
            int v = GW(0x208C);
            if (v != 0x10 && v != 0x14 && v != 7) {
                Hero_TryMoveStateL05(HERO_STATE_VOICE_SLOT_CLEANUP_IDLE_EXIT, 1);
                return;
            }
        }
    }
    {
        char *g = g_hero;
        if (GB(0x12E5) != 0) {
            if (GW(0x300) != 0) {
                if (!(GW(0x2084) == 0x3C && GH(0x41E) == 0)) {
                    exit_hero_vehicle_mode_if_mode_three();
                    if (GW(0x22A8) != 0) {
                        Hero_TryMoveStateL05(0x3C, 1);
                        return;
                    }
                    Hero_TryMoveStateL05(HERO_STATE_DEATH_DESCENT, 1);
                    return;
                }
            }
        }
    }
    {
        char *g = g_hero;
        if (GB(0x12EC) != 0 && GW(0x2084) != 0x7F) {
            if (absolute_float(GF(0x2F0) - (GF(0x88) + 0.25f)) < 1.0f && GF(0x2F0) - GF(0x88) > 0.0f &&
                GF(0x108) < 0.0f) {
                exit_hero_vehicle_mode_if_mode_three();
                Hero_TryMoveStateL05(HERO_STATE_SINK, 1);
                return;
            }
        }
    }
    {
        char *g = g_hero;
        if (GB(0x12E6) != 0 && GW(0x2084) != 0x68 && GW(0x2084) != 0x7B) {
            if (absolute_float(GF(0x2F4) - (GF(0x88) + 0.25f)) < 1.0f && GF(0x2F4) - GF(0x88) > 0.0f) {
                if ((GW(0x2084) != 0x69 || GH(0x41E) != 0) && GF(0x108) < 0.0f) {
                    exit_hero_vehicle_mode_if_mode_three();
                    if (GW(0x22A8) != 0) {
                        Hero_TryMoveStateL05(HERO_STATE_WAIT_FOR_JUMP_OR_RESPAWN, 1);
                        return;
                    }
                    Hero_TryMoveStateL05(HERO_STATE_HEIGHT_OR_TIMEOUT_RESPAWN_WAIT, 1);
                    return;
                }
            }
        }
    }
    {
        char *g = g_hero;
        if (GB(0x12EB) != 0 && GW(0x2084) != 0x7B) {
            if (absolute_float(GF(0x2F4) - (GF(0x88) + 0.25f)) < 1.0f && GF(0x2F4) - GF(0x88) > 0.0f &&
                GF(0x108) < 0.0f) {
                exit_hero_vehicle_mode_if_mode_three();
                Hero_TryMoveStateL05(HERO_STATE_HEIGHT_OR_TIMEOUT_RESPAWN_WAIT, 1);
                return;
            }
        }
    }
    return;
}

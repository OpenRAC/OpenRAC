extern short D_E1_L11_00161F88;
extern short D_E1_L11_00161F8C;
extern float D_E1_L11_00161FA4 SDATA(D_E1_L11_00161FA4);
extern float D_E1_L11_00161FA8 SDATA(D_E1_L11_00161FA8);
void jet_fighter_update(Moby *moby) {
    State_3146E0 *d;
    struct HitMessage *r;
    float v[4];
    int st;
    int s2;

    if (moby != 0) {
        d = *(State_3146E0 **)&moby->pvars;
        fast_dec_timer(&d->tDC);
        st = moby->state;
        if (st != 7 && st != 1 && st != 2) {
            r = (struct HitMessage *)MobyGetHitMessage(moby, 0x30001, 0);
            if (r != 0 && moby->state == 4) {
                Glob *g = (Glob *)g_hero;
                if (r->damage < g->t) {
                    g->t = g->t - r->damage;
                    if (r->unk_30 & 1) {
                        char *p;
                        float f;
                        p = (char *)&r->unk_10;
                        f = r->damage * ship_hit_deflect_scale;
                        build_spherical_offset(v, d->f64, d->v30[2], -d->v30[1]);
                        FastVecAdd(((float *)&moby->pos), ((float *)&moby->pos), p);
                        reflect_vector(v, v, p);
                        d->v30[2] = FastAddRots(d->v30[2], FastSubRots(fast_atan2(v[0], v[1]), d->v30[2]) *
                                                               *(float *)&D_E1_L11_00161F88 * f);
                        d->v30[1] =
                            FastAddRots(d->v30[1], FastSubRots(fast_atan2(vector_length_xy(v), v[2]), d->v30[1]) *
                                                       *(float *)&D_E1_L11_00161F8C * f);
                    }
                } else {
                    build_spherical_offset(v, d->f64, d->v30[2], -d->v30[1]);
                    g->t = 0.0f;
                    moby->state = 7;
                    d->t62 = 0xB4;
                    g->f88 = g->f88 + 0.2f;
                    g->p2080->motion.rot.i[2] = 0;
                    Hero_TrySetMoveStateL11(HERO_STATE_IDLE, 1);
                    g->cur = 0;
                    g->id = -1;
                    spawn_explosion_v(moby, v, 0, 0.0f, 0.0f, 0xA, 3, 0x10, 4.0f, 2.0f, 9.0f, -1, 1.0f, 15.0f, 1, 1, -1,
                                      0);
                }
            }
        }
        moby->hit_slot = 0xFF;
        {
            Glob *g = (Glob *)g_hero;
            if (g->cur == (int)moby) {
                g->b160F &= ~2;
                g->i1608 = (int)(g->t / ship_max_hp * 200.0f);
            }
        }
        switch (moby->state) {
            case 0:
                moby->state = 1;
                d->b60 = 0;
                moby->scale = *(float *)(*(char **)&moby->pclass + 0x24);
                d->v20[0] = d->v30[0];
                d->v20[1] = d->v30[1];
                {
                    float t = fast_atan2(1.0f, pokitaru_view_cot_half_fov_a[0]);
                    t = t + t;
                    d->iC0 = 0;
                    d->fAC = t;
                    d->fB0 = t;
                }
                qcopy(d->v30, ((float *)&moby->rot));
                qcopy(d->v50, ((float *)&moby->rot));
                qcopy(d->v40, ((float *)&moby->pos));
                *(char **)&ship_hud_clut = ship_hp_bar_palette_address_base + (pokitaru_texture_table + 0x3C)->h * 16;
                if (ship_hud_clut_saved == 0) {
                    *(Blk400_3146E0 *)ship_hud_clut_backup = **(Blk400_3146E0 **)&ship_hud_clut;
                }
                ship_hud_clut_saved = 1;
                d->tF8 = 1;
                d->hE8 = 1;
                d->f130 = 0.0f;
                if (moby->mission != 0xFF && saved_mission_state[g_current_level_index][moby->mission] == 0xFF) {
                    moby->state = 9;
                    *(int *)&moby->collision = 0;
                    moby->flags |= 1;
                    moby->visible = 0;
                }
                break;
            case 1: {
                Glob *g = (Glob *)g_hero;
                if (jet_distance(moby, g->p2080) < ship_board_radius) {
                    if (item_available[7] != 0) {
                        try_set_help_message(7, 0x2B09);
                    } else {
                        int elapsed, period;
                        HelpTab *statistics;
                        elapsed = scale_ticks(gameplay_event_frame_count);
                        period = 600;
                        statistics = (HelpTab *)help_records;
                        elapsed -= statistics->r[0x3C].best * period;
                        if (elapsed > (int)((float)scale_ticks(18) * 60.0f) || statistics->r[0x3C].best * period == 0) {
                            help_request(0x2AFA, 0x3C);
                        } else if (scale_ticks(gameplay_event_frame_count) / 600 > statistics->r[0x3C].best) {
                            statistics->r[0x3C].best = scale_ticks(gameplay_event_frame_count) / 600;
                        }
                    }
                    if (item_available[7] != 0 && (pad0_pressed & 0x10)) {
                        moby->state = 2;
                        d->i118 = -1;
                    }
                } else {
                    d->i118 = -1;
                    if (d->f130 != 0.0f) {
                        ApproachValue(&d->f130, 0.0f, g_frame_time * 4.0f);
                        pokitaru_fade_level = d->f130;
                    }
                }
                break;
            }
            case 2: {
                float f;
                ApproachValue(&d->f130, 1.0f, g_frame_time * 4.0f);
                f = d->f130;
                pokitaru_fade_level = f;
                if (f == 1.0f) {
                    Hero_TrySetMoveStateL11(0x32, 1);
                    moby->pos.z = moby->pos.z + 30.0f;
                    request_music_track(2, 6);
                    clear_u64_value(d->v30);
                    clear_u64_value(d->v20);
                    d->v20[2] = moby->rot.z;
                    d->v30[2] = d->v20[2];
                    d->t114 = scale_ticks(0x708);
                    if (*(unsigned short *)(help_stats_base + 0x5F5) == 0) {
                        help_request(0x2AFC, 0x55);
                    }
                    moby->state = 4;
                    {
                        Glob *g = (Glob *)g_hero;
                        g->cur = (int)moby;
                        g->id = *(unsigned short *)&moby->oclass;
                        g->n6 = ship_start_missiles;
                        g->b160E = 1;
                        g->n7 = ship_max_missiles;
                        g->n8 = ship_max_missiles;
                        g->t = ship_start_hp;
                        d->iF0 = 0;
                        d->iF4 = 0xFF;
                        g->f1600 = ship_max_hp;
                        g->f1604 = 100.0f;
                        g->n9 = 3;
                        g->nA = 3;
                        g->nB = 3;
                        g->b160F = 0;
                        jet_mount(camera_pos_b358, camera_pos_b358 + 4, 1, 0, 0);
                        d->f64 = ship_start_speed;
                        d->h68 = 0;
                        d->i6C = 0;
                        d->i70 = 0;
                        d->i84 = 0;
                        d->i80 = 0;
                        d->iD4 = 0;
                        d->iD8 = 0;
                        moby->draw_dist = 0xFF;
                        moby->update_dist = 0xFF;
                        d->fB4 = D_E1_L11_00161FA4 * ship_camera_offset_scale;
                        d->fBC = D_E1_L11_00161FA8 * ship_camera_offset_scale;
                        d->iB8 = 0;
                        g->p2080->motion.rot.i[2] = -1;
                        moby->scale = *(float *)(*(char **)&moby->pclass + 0x24) * ship_camera_offset_scale;
                        g->b20A5 = 1;
                    }
                }
                break;
            }
            case 4: {
                int n;
                if (d->f130 != 0.0f) {
                    ApproachValue(&d->f130, 0.0f, g_frame_time * 4.0f);
                    pokitaru_fade_level = d->f130;
                }
                pokitaru_play_mode = 2;
                update_player_ship_flight(moby, d);
                n = count_listed_blarg_ships_in_state_5(d->i10C);
                if (d->hEA != 0 && n < 5 - d->hE8 && fast_dec_timer(&d->t114) != 0 &&
                    activate_listed_blarg_ship(d->i10C) != 0) {
                    d->t114 = scale_ticks(0xF0);
                }
                if (d->hE8 <= 0) {
                    d->tF8 = ship_mission_end_ticks;
                    moby->state = 8;
                }
                {
                    Glob *g = (Glob *)g_hero;
                    if (g->b160F & 1) {
                        moby->state = 5;
                    }
                }
                break;
            }
            case 5: {
                Glob *g = (Glob *)g_hero;
                char *e;
                g->b20A5 = 0;
                g->p2080->motion.rot.i[2] = 0;
                Hero_TrySetMoveStateL11(HERO_STATE_IDLE, 1);
                jet_end_mount(0);
                set_animation_parameter(g->h160C, 0);
                request_music_track(0, 8);
                qcopy(((float *)&moby->pos), d->v40);
                qcopy(((float *)&moby->rot), d->v50);
                moby->scale = *(float *)(*(char **)&moby->pclass + 0x24);
                refresh_moby_spatial_bounds_from_basis(moby);
                e = path_points_14b0 + d->iFC * 128;
                set_race_camera(e + 0x30, e + 0x70, 0, 1);
                if (d->h12C != -1) {
                    char *e = voice_pool + d->h12C * 0x70;
                    if (*(void **)(e + 0x88) == moby && *(unsigned char *)(e + 0x74) != 0) {
                        Sound_StopSlot(d->h12C);
                    }
                }
                d->h12C = -1;
                if (d->h12E != -1) {
                    char *e = voice_pool + d->h12E * 0x70;
                    if (*(void **)(e + 0x88) == moby && *(unsigned char *)(e + 0x74) != 0) {
                        Sound_StopSlot(d->h12E);
                    }
                }
                d->h12E = -1;
                moby->state = 1;
                d->f130 = 0.99f;
                pokitaru_fade_level = 0.99f;
                break;
            }
            case 7:
                pokitaru_play_mode = 2;
                if (fast_dec_timer(&d->tF8) != 0) {
                    hero_respawn();
                }
                break;
            case 8:
                pokitaru_play_mode = 2;
                update_player_ship_flight(moby, d);
                if (fast_dec_timer(&d->tF8) != 0) {
                    float f;
                    ApproachValue(&d->f130, 1.0f, g_frame_time * 4.0f);
                    f = d->f130;
                    pokitaru_fade_level = f;
                    if (f >= 1.0f) {
                        Glob *g = (Glob *)g_hero;
                        int i;
                        g->b20A5 = 0;
                        g->p2080->motion.rot.i[2] = 0;
                        Hero_TrySetMoveStateL11(HERO_STATE_IDLE, 1);
                        jet_end_mount(0);
                        request_music_track(0, 8);
                        moby->visible = 0;
                        moby->flags |= 1;
                        *(int *)&moby->collision = 0;
                        i = d->i100;
                        if (i == -1) {
                            STUB_printf(jet_fighter_missing_park_point_format);
                            qcopy(((float *)&moby->pos), d->v40);
                            qcopy(((float *)&moby->rot), d->v50);
                        } else {
                            char *b2 = path_points_14b0;
                            qcopy(((float *)&moby->pos), b2 + i * 128 + 0x30);
                            qcopy(((float *)&moby->rot), b2 + i * 128 + 0x70);
                        }
                        refresh_moby_spatial_bounds_from_basis(moby);
                        if (d->iFC == -1) {
                            STUB_printf(jet_fighter_missing_hero_point_format);
                        } else {
                            char *e;
                            e = path_points_14b0 + d->iFC * 128;
                            set_race_camera(e + 0x30, e + 0x70, 0, 1);
                        }
                        if (moby->mission != 0xFF) {
                            jet_clear_persistent(moby->mission);
                        }
                        moby->state = 9;
                    }
                }
                break;
            case 9:
                if (d->f130 != 0.0f) {
                    ApproachValue(&d->f130, 0.0f, g_frame_time * 4.0f);
                    pokitaru_fade_level = d->f130;
                }
                break;
            case 3:
            case 6:
            case 10:
                break;
        }
        if (moby->pos.x < 15.0f)
            moby->pos.x = 15.0f;
        if (moby->pos.x > 1008.0f)
            moby->pos.x = 1008.0f;
        if (moby->pos.y < 15.0f)
            moby->pos.y = 15.0f;
        if (moby->pos.y > 1008.0f)
            moby->pos.y = 1008.0f;
        if (moby->pos.z < 15.0f)
            moby->pos.z = 15.0f;
        if (moby->pos.z > 1008.0f)
            moby->pos.z = 1008.0f;
        s2 = moby->state;
        if (s2 == 4) {
            qcopy(hero_pos, ((float *)&moby->pos));
            qcopy(hero_pos + 0x10, d->v30);
        }
        if (s2 >= 3 && s2 <= 4) {
            emit_type_74_particles_from_moby_joints(moby, d);
            if (voice_is_playing(moby, d->h12C) == 0) {
                d->h12C = Moby_PlaySound(0, 4, (int)moby);
            }
        } else {
            if (d->h12C != -1) {
                char *e = voice_pool + d->h12C * 0x70;
                if (*(void **)(e + 0x88) == moby && *(unsigned char *)(e + 0x74) != 0) {
                    Sound_StopSlot(d->h12C);
                }
            }
            d->h12C = -1;
        }
    }
}

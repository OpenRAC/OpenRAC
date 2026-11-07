void WaterSurfaceUpdate(Moby *moby) {
    Data_0030DED8 *d = *(Data_0030DED8 **)&moby->pvars;

    switch (moby->state) {
        case 0:
            if (d->f14 == 0) {
                int i;
                int j;
                float a;

                rilgar_flood_water_z = moby->pos.z;
                rilgar_flood_stage = 0;
                rilgar_flood_checkpoint_reached = 0;
                pool_init_tiles(water_records_r, 0x30);
                pool_set_scale(1.0f);
                g_light.f0 = 16.0f;
                g_light.f4 = 16.0f;
                g_light.f8 = -8.0f;
                g_light.fC = -8.0f;
                g_light.f10 = 1.0f;
                g_light.f14 = 1.0f;
                g_light.f18 = 0.9f;
                g_light.f20 = 0.1f;
                a = fast_atan2(l05_water_light_heading_source.f10, l05_water_light_heading_source.f14);
                g_light.f30 = FastCos(a) * 0.57735002f;
                g_light.f34 = FastSin(a) * 0.57735002f;
                g_light.f38 = -0.57735002f;
                g_light.f28 = 0x29;
                g_light.f2C = 0x2A;
                g_light.b3C = 0x1C;
                g_light.b3D = 0x40;
                rilgar_water_patches = water_records_r;
                rilgar_water_patch_position_table = rilgar_water_patch_positions;
                rilgar_water_patch_count = 0x30;
                rilgar_zone_tint2_red[0] = 0;
                rilgar_zone_tint2_green[0] = 0xB;
                rilgar_zone_tint2_blue[0] = 0x3E;
                rilgar_water_ripple_params_0 = 0;
                rilgar_water_ripple_params_1 = 20480.0f;
                rilgar_water_ripple_params_2 = 255.0f;
                rilgar_water_ripple_params_3 = 26.0f;
                for (i = 0; i < 0x30; i++) {
                    pool_bind_tile(&water_records_r[i].x, rilgar_water_patch_positions[i]);
                    water_records_r[d->f14].h1E = rilgar_water_patch_h1e[d->f14];
                }
                for (j = 0; j < 0x30; j++) {
                    record_grid_entry_neighbor_indices(j);
                }
            }
            moby->draw_dist = 0;
            moby->flags |= 1;
            if (d->fC != 0) {
                if (d->f10 >= 0) {
                    moby->update_dist = 0xFF;
                    if (rilgar_flood_slowdown_count >= 2) {
                        int k;
                        float s = (float)(rilgar_flood_slowdown_count - 1) * 0.014285714f + 1.0f;
                        if (s > 1.4f) {
                            s = 1.4f;
                        }
                        for (k = 0; k < 6; k++) {
                            rilgar_flood_stage_ticks[k] =
                                truncate_float_to_s32((float)rilgar_flood_stage_base_ticks[k] * s);
                            rilgar_flood_stage_speeds[k] = rilgar_flood_stage_base_speeds[k] / s;
                        }
                    }
                }
                if ((help_stats_base + 0xAA35)[moby->mission + (g_current_level_index << 4)] == 0xFF) {
                    moby->state = 7;
                    moby->pos.z = 44.0f;
                } else {
                    moby->state = 3;
                    d->f18 = scale_ticks(0x3C);
                    if (moby->pos.z < d->f4) {
                        water_records_r[d->f14].h1E = 0;
                    }
                }
            } else {
                if (d->f10 < 0) {
                    float z = moby->pos.z;
                    d->f4 = z;
                    d->f0 = z + 4.5f;
                    if (d->f8 != 0) {
                        moby->state = 1;
                        moby->pos.z = d->f0;
                    } else {
                        moby->state = 2;
                        moby->pos.z = d->f4;
                    }
                } else {
                    moby->state = 2;
                    moby->pos.z = d->f4;
                }
            }
            if (d->f14 == 0x2F) {
                *(int *)&moby->collision = 0;
            }
            break;
        case 2:
            if (moby->command & 4) {
                d->f24 = 1;
                set_moby_list_state(moby->group, 1);
            } else if (d->f10 >= 0) {
                if ((help_stats_base + 0xAA35)[moby->mission + (g_current_level_index << 4)] == 0xFF) {
                    moby->state = 1;
                    moby->pos.z = d->f0;
                }
            }
            if (d->f24 != 0) {
                ApproachWithVelocity(&moby->pos.z, &d->f20, d->f4, rilgar_water_gate_accel * g_frame_time_sq,
                                     rilgar_water_gate_accel * g_frame_time_sq,
                                     rilgar_water_gate_max_speed * g_frame_time);
                synchronize_group_water_heights((char *)moby);
            }
            moby->command = 1;
            if (random_integer_below(0xC8) == 0) {
                float x = water_records_r[d->f14].x + random_float_between(-4.0f, 4.0f);
                float r = random_float_between(-4.0f, 4.0f);
                Rec_0030DED8 *e = &water_records_r[d->f14];
                pool_splash(e, 1, 1, x, e->y + r, 1.0f, -0.05f);
            }
            break;
        case 1:
            if (moby->command & 8) {
                d->f24 = 1;
                set_moby_list_state(moby->group, 2);
            } else {
                moby->command = 2;
            }
            if (d->f24 != 0) {
                ApproachWithVelocity(&moby->pos.z, &d->f20, d->f0, rilgar_water_gate_accel * g_frame_time_sq,
                                     rilgar_water_gate_accel * g_frame_time_sq,
                                     rilgar_water_gate_max_speed * g_frame_time);
                synchronize_group_water_heights((char *)moby);
            }
            if (random_integer_below(0xC8) == 0) {
                float x = water_records_r[d->f14].x + random_float_between(-4.0f, 4.0f);
                float r = random_float_between(-4.0f, 4.0f);
                Rec_0030DED8 *e = &water_records_r[d->f14];
                pool_splash(e, 1, 1, x, e->y + r, 1.0f, -0.05f);
            }
            break;
        case 3:
            if (vec_distance_xy(((float *)&moby->pos), ((Hero *)(hero_globals_base + 0xE1D))->motion.pos.f) < 16.0f) {
                Hero *h = (Hero *)(hero_globals_base + 0xE1D);
                if (h->motion.pos.f[2] < moby->pos.z) {
                    if (h->state.control_mode == HERO_MODE_SWIM) {
                        if (h->water_surface_z < moby->pos.z) {
                            h->water_surface_z = moby->pos.z;
                        }
                    }
                }
            }
            if (d->f10 >= 0) {
                if (vec_distance_xy(((float *)&moby->pos), ((Hero *)(hero_globals_base + 0xE1D))->motion.pos.f) <
                    10.0f) {
                    Hero *h = (Hero *)(hero_globals_base + 0xE1D);
                    if (h->ground_moby != 0 && h->air_frames.s == 0 &&
                        h->ground_moby->oclass == MOBY_CLASS_FLOOR_BUTTON) {
                        d->f1C = 0;
                        moby->state = 8;
                        rilgar_flood_stage = 0;
                        d->f18 = scale_ticks(rilgar_flood_stage_ticks[0]);
                    }
                }
            }
            if (random_integer_below(0x32) == 0) {
                float x = water_records_r[d->f14].x + random_float_between(-4.0f, 4.0f);
                float r = random_float_between(-4.0f, 4.0f);
                Rec_0030DED8 *e = &water_records_r[d->f14];
                pool_splash(e, 1, 1, x, e->y + r, 1.0f, -0.01f);
            }
            break;
        case 7:
            if (d->f10 >= 0) {
                Hero *h = (Hero *)(hero_globals_base + 0xE1D);
                if (h->state.current == HERO_STATE_DROWN && h->state_timer == 2) {
                    rilgar_flood_slowdown_count++;
                }
            } else if (d->f0 < moby->pos.z) {
                water_records_r[d->f14].h1E = 0;
                DeleteMoby(moby);
                return;
            }
            break;
        case 8:
            if (fast_dec_timer(&d->f18) != 0) {
                if (rilgar_flood_stage < 5) {
                    rilgar_flood_stage++;
                    d->f18 = scale_ticks(rilgar_flood_stage_ticks[rilgar_flood_stage]);
                }
            }
            moby->pos.z += rilgar_flood_stage_speeds[rilgar_flood_stage] * g_frame_time;
            synchronize_group_water_heights((char *)moby);
            rilgar_flood_water_z = moby->pos.z;
            {
                Vec4 v;

                v.q = 0;
                v.f[0] = 211.0f;
                v.f[1] = 338.0f;
                v.f[2] = 25.0f;
                if (fast_vec_dist(v.f, &((struct Hero *)((char *)&hero_globals_base + 0xE1D))->motion.pos) < 6.0f) {
                    if (rilgar_flood_checkpoint_reached == 0) {
                        rilgar_flood_checkpoint_reached = 1;
                        if (moby->pos.z > rilgar_flood_skip_height) {
                            rilgar_flood_stage = 5;
                        }
                    }
                }
            }
            if (d->f10 >= 0) {
                Hero *h = (Hero *)(hero_globals_base + 0xE1D);
                if (h->state.current == HERO_STATE_DROWN && h->state_timer == 2) {
                    rilgar_flood_slowdown_count++;
                }
            }
            if (moby->pos.z >= 44.0f) {
                set_moby_list_state(moby->group, 7);
            }
            break;
    }
    if (d->f14 >= 0) {
        Rec_0030DED8 *e = &water_records_r[d->f14];
        float t;

        e->z = moby->pos.z;
        t = 0.0f;
        if (check_scaled_vector_bsphere((char *)moby, 48.0f) != -1 &&
            (!(moby->state >= 3 && moby->state <= 7) || (moby->pos.z > d->f4 && moby->pos.z < d->f0))) {
            t = 1.0f;
        }
        if (t == 0.0f) {
            water_records_r[d->f14].h1E = 0;
        } else {
            water_records_r[d->f14].h1E = rilgar_water_patch_h1e[d->f14];
        }
        if (d->f14 == 0) {
            pool_flush((char *)water_records_r, 0x30);
            AddDrawCallback(DrawRipplePatches, moby);
        }
        if (d->f14 == rilgar_water_override_patch) {
            water_records_r[d->f14].h1E = rilgar_water_override_value;
            rilgar_water_patch_h1e[d->f14] = rilgar_water_override_value;
        }
    }
}

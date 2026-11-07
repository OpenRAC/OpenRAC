void elevator_685_update(char *moby) {
    struct ElevatorVars *data;
    char *tbl;
    float f1v;
    float f0;
    float ft;
    int v;
    int s;
    int r;
    char v0[16];
    char v10[16];
    char v20[16];
    float t30;

    data = (struct ElevatorVars *)((Moby *)moby)->pvars;
    if (((unsigned char *)moby)[0x20] != 0) {
        if (((unsigned char *)moby)[0xBC] == 2) {
            if (data->dir < 0.0f) {
                tbl = oltanis_paths_r2[data->path_to];
                data->dir = -data->dir;
                data->point = *(int *)tbl - 2;
                data->point_dist = *(float *)(tbl + (data->point << 4) + 0x1C);
                ((unsigned char *)moby)[0xBC] = 1;
            }
        }
    }
    qcopy(v0, moby + 0x10);
    qcopy(v10, moby + 0x40);

    switch (((unsigned char *)moby)[0x20]) {
        case 0:
            ((unsigned char *)moby)[0x20] = 1;
            ((unsigned char *)moby)[0xBC] = 0;
            initialize_elevator_path_state_r(moby);
            qcopy(moby + 0x10, oltanis_paths_r2[data->path_from] + 0x10);
            qcopy(moby + 0x40, &data->rot_start_x);
            {
                unsigned short hw = ((Moby *)moby)->save_id;
                int sh = (short)hw;
                if (((Tab *)oltanis_collected_flags_base_r)->b[sh]) {
                    data->locked = 0;
                } else if (((*(int *)(level_moby_killed_r + ((sh >> 5) * 4 + (g_current_level_index_m << 8))) >>
                             (hw & 0x1F)) ^
                            1) &
                           1) {
                    if (saved_mission_state_r[data->mission + (g_current_level_index_m << 4)] != 0xFF) {
                        data->locked = 0;
                    } else {
                        data->locked = 1;
                    }
                } else {
                    data->locked = 0;
                }
            }
            ((unsigned char *)moby)[0x30] = 0xFF;
            ((Moby *)moby)->glow_color = 0x80303030;
            data->voice = -1;
            data->pitch_base = -1;
            data->pitch_alt = -1;
            *(int *)&data->glow = 0;
            break;
        case 1:
            update_elevator_idle_glow_r(moby);
            v = data->call_a;
            if (v >= 0) {
                if (is_point_inside_clip_volume(hero_pos, v) != 0) {
                    ((unsigned char *)moby)[0xBC] = 0;
                    data->dir = -1.0f;
                    data->point = 0;
                    data->point_dist = 0.0f;
                }
            }
            v = data->call_b;
            if (v >= 0 && is_point_inside_clip_volume(hero_pos, v) != 0) {
                ((unsigned char *)moby)[0xBC] = 0;
                data->point = *(int *)oltanis_paths_r2[data->path_from] - 2;
                data->point_dist = *(float *)(oltanis_paths_r2[data->path_from] + (data->point << 4) + 0x1C);
                data->dir = 1.0f;
            }
            v = data->call_c;
            if (v >= 0 && is_point_inside_clip_volume(hero_pos, v) != 0) {
                ((unsigned char *)moby)[0xBC] = 1;
                data->point = *(int *)oltanis_paths_r2[data->path_to] - 2;
                data->point_dist = *(float *)(oltanis_paths_r2[data->path_to] + (data->point << 4) + 0x1C);
                data->dir = 1.0f;
            }
            if (((char *)((Hero *)g_hero_r2)->ground_moby) == moby && ((Hero *)g_hero_r2)->frames_off_ground.s == 0) {
                {
                    f0 = vec_distance_xy(((Hero *)g_hero_r2)->motion.pos.f, moby + 0x10);
                    if (f0 < 1.0f) {
                        r = Moby_PlaySound(1, 0, (int)moby);
                        if (r >= 0) {
                            if (data->pitch_alt < 0) {
                                data->pitch_alt = path_mover_ride_sound_default_pitch;
                            }
                            Sound_SetPitchBend(r, data->pitch_alt);
                        }
                        f1v = data->dir;
                        if (f1v == -1.0f) {
                            if (data->locked != 0) {
                                ((unsigned char *)moby)[0xBC] = 2;
                            } else {
                                ((unsigned char *)moby)[0xBC] = 0;
                            }
                        }
                        ((unsigned char *)moby)[0x20] = 2;
                        *(int *)&data->travelled = 0;
                        data->dir = -data->dir;
                    }
                }
            }
            break;
        case 2:
            if (voice_is_playing(moby, data->voice) == 0) {
                data->voice = Moby_PlaySound(0, 4, (int)moby);
                if (data->voice != 0) {
                    if (data->pitch_base < 0) {
                        data->pitch_base = path_mover_loop_sound_default_pitch;
                    }
                    Sound_SetPitchBend(data->voice, data->pitch_base);
                }
            }
            ((Moby *)moby)->glow_color = FastTweenColor(((Moby *)moby)->glow_color, 0x80808040, 0.1f);
            if (advance_elevator_along_path_r(moby) != 0) {
                ((unsigned char *)moby)[0x20] = 3;
                data->pitch = *(unsigned short *)&data->pitch_base;
            }
            rebuild_elevator_world_path_r(moby);
            FastVecSub(v20, moby + 0x10, v0);
            update_entity_matrix_r(data->entity, v20, v10, moby + 0x40);
            break;
        case 3:
            ft = FastVecLength(&data->pos_vel_x);
            if (0.001f <= ft) {
                if (voice_is_playing(moby, data->voice) == 0) {
                    data->voice = Moby_PlaySound(0, 4, (int)moby);
                }
                t30 = IntToFloat(data->pitch);
                {
                    float *pt = &t30;
                    f0 = IntToFloat(data->pitch_base);
                    Spring_r(pt, &data->pitch_spring, f0 * 0.25f, 0.003f, 0.2f, 0.0f);
                }
                data->pitch = truncate_float_to_s32_r2(t30);
                if (data->voice >= 0) {
                    Sound_SetPitchBend(data->voice, data->pitch);
                }
                rebuild_elevator_world_path_r(moby);
            } else {
                update_elevator_idle_glow_r(moby);
                v = data->voice;
                if (v != -1 && ((VoicePoolRow *)((Voice *)voice_pool_r + v))->owner == moby &&
                    ((VoicePoolRow *)((Voice *)voice_pool_r + v))->state != 0) {
                    Sound_StopSlot(v);
                }
                data->voice = -1;
                s = Moby_PlaySound(1, 0, (int)moby);
                if (s >= 0) {
                    Sound_SetPitchBend(s, data->pitch_alt / 3);
                }
                ((unsigned char *)moby)[0x20] = 4;
                if (g_hero_ground_moby != moby) {
                    ((unsigned char *)moby)[0x20] = 1;
                }
            }
            break;
        case 4:
            update_elevator_idle_glow_r(moby);
            if (g_hero_ground_moby != moby) {
                ((unsigned char *)moby)[0x20] = 1;
            }
            break;
        default:
            break;
    }

    if (((unsigned char *)moby)[0x20] != 2) {
        if (((unsigned char *)moby)[0x20] != 0) {
            settle_elevator_at_path_endpoint_r(moby);
            if (g_hero_ground_moby == moby) {
                FastVecSub(v20, moby + 0x10, v0);
                update_entity_matrix_r(data->entity, v20, v10, moby + 0x40);
            }
        }
    }
}

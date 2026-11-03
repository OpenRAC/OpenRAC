void update_enemy_fighter_921(Moby *m) {
    Vec_2FE038 vt;
    Vec_2FE038 vb;
    Jet *d = (struct Jet *)m->pvars;
    unsigned short id;

    if (m->state == 0 &&
        (oltanis_collected_flags_base_r3.collected[(short)(id = m->save_id)] != 0 ||
         (*(int *)(level_moby_killed_r3 + (((short)id >> 5) * 4 + (g_current_level_index_m << 8))) >> (id & 0x1F)) &
             1)) {
        mark_moby_and_linked_moby_for_removal((Moby *)(oltanis_mobys_r + (d->f8C << 8)));
        DeleteMoby(m);
        return;
    }
    {
        void *r = MobyGetHitMessage(m, 0x800000, 0);
        m->hit_slot = 0xFF;
        if (r != 0 && d->f8C == -1 && m->state != 3) {
            if (g_current_level_index_m == LEVEL_OLTANIS) {
                fighter_skill_hit_count_r2 = fighter_skill_hit_count_r2 + 1;
                if (fighter_skill_hit_count_r2 >= 3 && ((Skill_2FE038 *)gSkillPoints_r)->f18 == 0) {
                    ((Skill_2FE038 *)gSkillPoints_r)->f18 = 1;
                    PlayBankSound(1, 0, 0);
                    ShowBanner(0x53D6, -1);
                }
            }
            m->state = 3;
            SpawnExplosionFx(m, (*(Vec_2FE038 *)&m->pos), -1, 5.0f, 13.0f);
            m->flags |= 0x41;
            m->collision = 0;
            if (d->f6C != -1 && ((VoicePoolRow *)(voice_pool_r + d->f6C * 0x70))->owner == m &&
                ((VoicePoolRow *)(voice_pool_r + d->f6C * 0x70))->state != 0) {
                Sound_StopSlot(d->f6C);
            }
            d->f6C = -1;
        }
    }
    switch (m->state) {
        case 0:
            if (d->f68 == -1) {
                STUB_printf_r3(enemy_fighter_921_no_path_msg_r, (short)m->save_id);
                DeleteMoby(m);
                return;
            }
            if (*(int *)oltanis_paths_r4[d->f68] == 0) {
                STUB_printf_r3(enemy_fighter_921_path_unset_msg_r, (short)m->save_id);
                DeleteMoby(m);
                return;
            }
            if (d->f98 < 0 || d->f9C < 0 || d->fA0 < 0) {
                DeleteMoby(m);
                return;
            }
            initialize_flight_path_state_r2(m);
            if (d->f8C != 0) {
                m->state = 1;
                m->update_dist = 0xFF;
                m->flags |= 0x41;
                m->collision = 0;
                ((Moby *)(oltanis_mobys_r + (d->f8C << 8)))->flags |= 0x41;
                d->f94 = 0;
            } else {
                m->state = 2;
                m->update_dist = 0x80;
            }
            m->draw_dist = 0x80;
            d->f6C = -1;
            d->f7C = scale_ticks(D_L14_0016205C_2FE038);
            break;
        case 1: {
            unsigned char a, b, c;
            a = saved_mission_state_r[d->f98 + (g_current_level_index_m << 4)];
            if (a == 0xFF) {
                b = saved_mission_state_r[d->f9C + (g_current_level_index_m << 4)];
                if (b == a) {
                    c = saved_mission_state_r[d->fA0 + (g_current_level_index_m << 4)];
                    if (c == b && g_hero.frames_off_ground.s == 0 && ((int)g_hero.ground_moby) == 0 &&
                        is_point_inside_clip_volume((*(Vec_2FE038 *)&g_hero.motion.pos), d->fA4) != 0) {
                        Moby *jet = (Moby *)(oltanis_mobys_r + (d->f8C << 8));
                        Moby *seat = (Moby *)(oltanis_mobys_r + (((struct Jet *)jet->pvars)->f4 << 8));
                        float pitch, yaw;
                        qcopy(vt, (*(Vec_2FE038 *)&seat->pos));
                        vt[2] = vt[2] + 4.0f;
                        pitch = FastDiffRots(((Cam *)oltanis_player_camera_r)->f154,
                                             -fast_atan2(vec_distance_xy(((Cam *)oltanis_player_camera_r)->pos, vt),
                                                         vt[2] - ((Cam *)oltanis_player_camera_r)->pos[2]));
                        yaw = FastDiffRots(
                            ((Cam *)oltanis_player_camera_r)->f158,
                            fast_atan2((*(Vec_2FE038 *)&seat->pos)[0] - ((Cam *)oltanis_player_camera_r)->pos[0],
                                       (*(Vec_2FE038 *)&seat->pos)[1] - ((Cam *)oltanis_player_camera_r)->pos[1]));
                        if (yaw < 0.48869219f && pitch < 0.34906584f &&
                            coll_line_r(((Cam *)oltanis_player_camera_r)->pos, vt, 0x12, seat, 0) == 0) {
                            *(int *)(level_moby_killed_r3 +
                                     (((short)m->save_id >> 5) * 4 + (g_current_level_index_m << 8))) |=
                                1 << (m->save_id & 0x1F);
                            oltanis_moby_killed_bits_r[(short)m->save_id >> 5] |= 1 << (m->save_id & 0x1F);
                            m->flags &= 0xFFBE;
                            m->collision = *(int *)((char *)((struct Moby *)m->pclass) + 0x10);
                            jet->flags &= 0xFFBE;
                            m->state = 2;
                            activate_descriptor(d->f90);
                            seat->draw_dist = 0xFF;
                            seat->update_dist = 0xFF;
                        }
                    }
                }
            }
            break;
        }
        case 2:
            if (advance_moby_along_flight_path_r(m) != 0) {
                return;
            }
            if (d->f8C != 0 && d->f94 < 2) {
                if (d->f94 == 0 && d->f60 >= 0x38) {
                    d->f94 = 1;
                }
                FastVecScale(vb, (*(Vec_2FE038 *)&m->rot_row0), 0.21f);
                FastVecScale(vt, (*(Vec_2FE038 *)&m->rot_row2), -1.24f);
                FastVecAdd(vb, vb, vt);
                FastVecAdd(vb, (*(Vec_2FE038 *)&m->pos), vb);
                {
                    Moby *jet = (Moby *)(oltanis_mobys_r + (d->f8C << 8));
                    qcopy((*(Vec_2FE038 *)&jet->pos), vb);
                    qcopy((*(Vec_2FE038 *)&jet->rot), (*(Vec_2FE038 *)&m->rot));
                    if (d->f94 == 1) {
                        set_moby_state_two(jet);
                        d->f94 = 2;
                    }
                }
            }
            break;
        case 3:
            if (fast_dec_timer_r2(&d->f7C) != 0) {
                Moby *p = oltanis_paths_r4[d->f68];
                m->flags &= 0xFFBE;
                m->collision = *(int *)((char *)((struct Moby *)m->pclass) + 0x10);
                d->f7C = scale_ticks(D_L14_0016205C_2FE038);
                m->state = 2;
                d->f60 = 0;
                d->f64 = 0;
                qcopy((*(Vec_2FE038 *)&m->pos), (*(Vec_2FE038 *)&p->pos));
                (*(Vec_2FE038 *)&m->rot)[2] = fast_atan2(*(float *)((char *)p + 0x20) - (*(Vec_2FE038 *)&p->pos)[0],
                                                         *(float *)((char *)p + 0x24) - (*(Vec_2FE038 *)&p->pos)[1]);
                d->f80 = d->f84 = d->f88 = d->f70 = d->f74 = d->f78 = 0.0f;
            }
            break;
    }
    spawn_moby_exhaust_puffs_r(m);
    if (m->state != 0 && m->state != 3 && m->state != 1 && Sound_IsPlaying(m, d->f6C) == 0) {
        d->f6C = Moby_PlaySound_r4(0, 4, m);
    }
}

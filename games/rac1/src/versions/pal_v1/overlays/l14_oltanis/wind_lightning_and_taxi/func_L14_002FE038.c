void update_enemy_fighter_921(Moby *m) {
    Vec_2FE038 vt;
    Vec_2FE038 vb;
    Jet_2FE038 *d = (struct Jet_2FE038 *)m->pvars;
    unsigned short id;

    if (m->state == 0
        && (oltanis_collected_flags_base_r3.collected[(short)(id = m->save_id)] != 0
            || (*(int *)(level_moby_killed_r3 + (((short)id >> 5) * 4 + (g_current_level_index_m << 8))) >> (id & 0x1F)) & 1)) {
        mark_moby_and_linked_moby_for_removal_r((Moby *)(oltanis_mobys_r + (d->f8C << 8)));
        DeleteMoby_r3(m);
        return;
    }
    {
        void *r = MobyGetHitMessage_r2(m, 0x800000, 0);
        m->hit_slot = 0xFF;
        if (r != 0 && d->f8C == -1 && m->state != 3) {
            if (g_current_level_index_m == 0xE) {
                fighter_skill_hit_count_r2 = fighter_skill_hit_count_r2 + 1;
                if (fighter_skill_hit_count_r2 >= 3 && ((Skill_2FE038 *)gSkillPoints_r)->f18 == 0) {
                    ((Skill_2FE038 *)gSkillPoints_r)->f18 = 1;
                    PlayBankSound_r2(1, 0, 0);
                    ShowBanner_r2(0x53D6, -1);
                }
            }
            m->state = 3;
            SpawnExplosionFx_r2(m, (*(Vec_2FE038 *)&m->pos), -1, 5.0f, 13.0f);
            m->flags |= 0x41;
            m->collision = 0;
            if (d->f6C != -1 && ((Voice_2FE038 *)(voice_pool_r + d->f6C * 0x70))->owner == m
                && ((Voice_2FE038 *)(voice_pool_r + d->f6C * 0x70))->on != 0) {
                Sound_StopSlot_r3(d->f6C);
            }
            d->f6C = -1;
        }
    }
    switch (m->state) {
    case 0:
        if (d->f68 == -1) {
            STUB_printf_r3(enemy_fighter_921_no_path_msg_r, (short)m->save_id);
            DeleteMoby_r3(m);
            return;
        }
        if (*(int *)oltanis_paths_r4[d->f68] == 0) {
            STUB_printf_r3(enemy_fighter_921_path_unset_msg_r, (short)m->save_id);
            DeleteMoby_r3(m);
            return;
        }
        if (d->f98 < 0 || d->f9C < 0 || d->fA0 < 0) {
            DeleteMoby_r3(m);
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
        d->f7C = scale_ticks_r3(D_L14_0016205C_2FE038);
        break;
    case 1: {
        unsigned char a, b, c;
        a = saved_mission_state_r[d->f98 + (g_current_level_index_m << 4)];
        if (a == 0xFF) {
            b = saved_mission_state_r[d->f9C + (g_current_level_index_m << 4)];
            if (b == a) {
                c = saved_mission_state_r[d->fA0 + (g_current_level_index_m << 4)];
                if (c == b && ((Hero *)(hero_globals_base_r + 0xE1D))->frames_off_ground.s == 0
                    && ((int)((Hero *)(hero_globals_base_r + 0xE1D))->ground_moby) == 0
                    && is_point_inside_clip_volume_r3((*(Vec_2FE038 *)&((Hero *)(hero_globals_base_r + 0xE1D))->motion.pos), d->fA4) != 0) {
                    Moby *jet = (Moby *)(oltanis_mobys_r + (d->f8C << 8));
                    Moby *seat = (Moby *)(oltanis_mobys_r + (((struct Jet_2FE038 *)jet->pvars)->f4 << 8));
                    float pitch, yaw;
                    qcopy(vt, (*(Vec_2FE038 *)&seat->pos));
                    vt[2] = vt[2] + 4.0f;
                    pitch = FastDiffRots_r(((Cam_2FE038 *)oltanis_player_camera_r)->f154,
                                                 -fast_atan2_r2(vec_distance_xy_r2(((Cam_2FE038 *)oltanis_player_camera_r)->pos, vt),
                                                                           vt[2] - ((Cam_2FE038 *)oltanis_player_camera_r)->pos[2]));
                    yaw = FastDiffRots_r(((Cam_2FE038 *)oltanis_player_camera_r)->f158,
                                               fast_atan2_r2((*(Vec_2FE038 *)&seat->pos)[0] - ((Cam_2FE038 *)oltanis_player_camera_r)->pos[0],
                                                                        (*(Vec_2FE038 *)&seat->pos)[1] - ((Cam_2FE038 *)oltanis_player_camera_r)->pos[1]));
                    if (yaw < 0.48869219f && pitch < 0.34906584f
                        && coll_line_r(((Cam_2FE038 *)oltanis_player_camera_r)->pos, vt, 0x12, seat, 0) == 0) {
                        *(int *)(level_moby_killed_r3 + (((short)m->save_id >> 5) * 4 + (g_current_level_index_m << 8))) |= 1 << (m->save_id & 0x1F);
                        oltanis_moby_killed_bits_r[(short)m->save_id >> 5] |= 1 << (m->save_id & 0x1F);
                        m->flags &= 0xFFBE;
                        m->collision = *(int *)((char *)((struct Moby *)m->pclass) + 0x10);
                        jet->flags &= 0xFFBE;
                        m->state = 2;
                        activate_descriptor_r(d->f90);
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
            FastVecScale_r(vb, (*(Vec_2FE038 *)&m->rot_row0), 0.21f);
            FastVecScale_r(vt, (*(Vec_2FE038 *)&m->rot_row2), -1.24f);
            FastVecAdd_r2(vb, vb, vt);
            FastVecAdd_r2(vb, (*(Vec_2FE038 *)&m->pos), vb);
            {
            Moby *jet = (Moby *)(oltanis_mobys_r + (d->f8C << 8));
            qcopy((*(Vec_2FE038 *)&jet->pos), vb);
            qcopy((*(Vec_2FE038 *)&jet->rot), (*(Vec_2FE038 *)&m->rot));
            if (d->f94 == 1) {
                set_moby_state_two_r(jet);
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
            d->f7C = scale_ticks_r3(D_L14_0016205C_2FE038);
            m->state = 2;
            d->f60 = 0;
            d->f64 = 0;
            qcopy((*(Vec_2FE038 *)&m->pos), (*(Vec_2FE038 *)&p->pos));
            (*(Vec_2FE038 *)&m->rot)[2] = fast_atan2_r2(*(float *)((char *)p + 0x20) - (*(Vec_2FE038 *)&p->pos)[0], *(float *)((char *)p + 0x24) - (*(Vec_2FE038 *)&p->pos)[1]);
            d->f80 = d->f84 = d->f88 = d->f70 = d->f74 = d->f78 = 0.0f;
        }
        break;
    }
    spawn_moby_exhaust_puffs_r(m);
    if (m->state != 0 && m->state != 3 && m->state != 1 && Sound_IsPlaying_r3(m, d->f6C) == 0) {
        d->f6C = Moby_PlaySound_r4(0, 4, m);
    }
}

void shuttle_1448_update(unsigned char *m) {
    Vars1448 *v = ((Vars1448 *)((Moby *)m)->pvars);
    float old_pos[4];
    float old_rot[4];
    float delta[4];
    float b[4];
    float rot[4];
    float a[4];
    int t;
    int idx;
    char *p;
    struct Hero *pl;

    qcopy(old_pos, m + 0x10);
    qcopy(old_rot, m + 0x40);
    ((Moby *)m)->scale = *(float *)(*(char **)(m + 0x24) + 0x24) * shuttle_scale_near;
    switch (((Moby *)m)->state) {
        case 0:
            if (l17_save_id_collection_state.collected[(short)((Moby *)m)->save_id] != 0 ||
                (*(int *)(level_moby_killed +
                          (((short)((Moby *)m)->save_id >> 5) * 4 + (g_current_level_index_m << 8))) >>
                 (((Moby *)m)->save_id & 0x1F)) &
                    1) {
                v->sel = 0;
                v->dir = 0;
                v->side = 0;
            }
            ((Moby *)m)->update_dist = 0xFF;
            ((Moby *)m)->draw_dist = 0x80;
            *(long *)(m + 0x38) = *(long *)(*(char **)&g_hero.moby + 0x38);
            ((Moby *)m)->state = 1;
            if (((Moby *)m)->prev_seq != 2) {
                MobyBlendAnimation(m, 2, 0, 0);
            }
            {
                int base = (int)fleet_camera_points;
                v->sel = 1;
                v->side = 1;
                v->dir = 1;
                *(u128_2F3450 *)(m + 0x10) = *(u128_2F3450 *)((v->cam[1] << 7) + base + 0x30);
                *(u128_2F3450 *)(m + 0x40) = *(u128_2F3450 *)((v->cam[1] << 7) + base + 0x70);
            }
            break;
        case 1:
            if (((Moby *)m)->prev_seq == 2 && (((Moby *)m)->anim_flags & 2) && v->sfx == 0) {
                Moby_PlaySound(1, 0, (int)m);
                v->sfx = 0;
            }
            pl = &g_hero;
            if (*(unsigned char **)&pl->ground_moby == m && pl->air_frames.s == 0) {
                update_shuttle_help_message(m);
                if ((pad0_pressed & 0x10) && fleet_shown_help_message == 10) {
                    int s = v->dir;
                    if (s >= 0) {
                        v->go = 1;
                        switch (s) {
                            case 0: {
                                float x = shuttle_scale_near;
                                float y = shuttle_scale_far;
                                v->sel = 1;
                                v->f9C = x;
                                v->fA0 = y;
                                v->side = 0;
                                break;
                            }
                            case 1: {
                                float x = shuttle_scale_far;
                                float y = shuttle_scale_near;
                                v->side = s;
                                v->f9C = x;
                                v->fA0 = y;
                                v->sel = 0;
                                break;
                            }
                        }
                    }
                }
            }
            if (v->go != 0) {
                v->go = 0;
                remove_help_message_for_selector(10);
                ((Moby *)m)->state = 2;
                Moby_PlaySound(2, 0, (int)m);
                if (((Moby *)m)->prev_seq != 0) {
                    MobyBlendAnimation(m, 0, 0, 0);
                }
                FadeToBlack(scale_ticks(10));
                v->w88 = 0;
                idx = v->side;
                qcopy(a, ((char **)fleet_paths)[v->a[idx]] + 0x10);
                qcopy(b, ((char **)fleet_paths)[v->b[idx]] + 0x10);
                clear_u64_value(rot);
                rot[2] = fast_atan2(b[0] - a[0], b[1] - a[1]);
                rot[1] = -fast_atan2(vec_distance_xy(a, b), b[2] - a[2]);
                jet_mount(a, rot, 2, scale_ticks(300), 0);
                set_selected_object_float_triplets(0.001f, 1.0f, 1.0f, 0.001f, 1.0f, 1.0f);
                set_camera_position(a);
                set_camera_rotation(rot);
                set_race_camera(fleet_camera_points + (v->cut << 7) + 0x30, fleet_camera_points + (v->cut << 7) + 0x70,
                                0x72, 0);
            } else if (l17_save_id_collection_state.collected[(short)((Moby *)m)->save_id] != 0 ||
                       (*(int *)(level_moby_killed +
                                 (((short)((Moby *)m)->save_id >> 5) * 4 + (g_current_level_index_m << 8))) >>
                        (((Moby *)m)->save_id & 0x1F)) &
                           1) {
                float d = vec_distance_xy(&g_hero.motion.pos, fleet_camera_points + (v->cam[v->sel] << 7) + 0x30);
                if (vec_distance_xy(&g_hero.motion.pos, fleet_camera_points + (v->cam[v->sel ^ 1] << 7) + 0x30) < d) {
                    int base;
                    int ofs;
                    t = v->sel ^ 1;
                    v->dir ^= 1;
                    v->side ^= 1;
                    v->sel = t;
                    base = (int)fleet_camera_points;
                    ofs = v->cam[t] << 7;
                    qcopy(m + 0x10, (char *)(ofs + base + 0x30));
                    qcopy(m + 0x40, (char *)(ofs + base + 0x70));
                    euler_rotation_basis(m + 0xC0, m + 0x40);
                    ((Moby *)m)->scale = *(float *)(*(char **)(m + 0x24) + 0x24);
                }
            }
            break;
        case 2:
            if (update_shuttle_travel_and_camera(m, v->side)) {
                int ofs;
                int base;
                ((Moby *)m)->state = 3;
                v->wA4 = 0;
                ofs = v->cam[v->sel] << 7;
                base = (int)fleet_camera_points;
                qcopy(m + 0x10, (char *)(ofs + base + 0x30));
                qcopy(m + 0x40, (char *)(ofs + base + 0x70));
                euler_rotation_basis(m + 0xC0, m + 0x40);
                ((Moby *)m)->scale = *(float *)(*(char **)(m + 0x24) + 0x24);
                qcopy(b, m + 0x10);
                vec_scale_to_length(a, m + 0xC0, -2.0f);
                FastVecAdd(a, a, m + 0xE0);
                FastVecAdd(b, b, a);
                clear_u64_value(rot);
                rot[2] = FastAddRots(((Moby *)m)->rot.z, 3.1415927f);
                set_race_camera(b, rot, 0, 0);
                jet_end_mount(0);
                *(int *)(level_moby_killed + (((short)((Moby *)m)->save_id >> 5) * 4 +
                                              (g_current_level_index_m << 8))) |= 1 << (((Moby *)m)->save_id & 0x1F);
                shuttle_travel_completion_bits[(short)((Moby *)m)->save_id >> 5] |= 1 << (((Moby *)m)->save_id & 0x1F);
            }
            break;
        case 3:
            ((Moby *)m)->state = 1;
            if (((Moby *)m)->prev_seq != 2) {
                MobyBlendAnimation(m, 2, 0, scale_ticks(30));
            }
            Moby_PlaySound(0, 0, (int)m);
            v->sfx = 1;
            break;
    }
    FastVecSub(delta, m + 0x10, old_pos);
    update_entity_matrix(v->area, delta, old_rot, m + 0x40);
}

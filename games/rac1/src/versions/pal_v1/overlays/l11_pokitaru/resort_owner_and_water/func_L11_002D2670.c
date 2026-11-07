typedef struct {
    float a[4];
    float b[4];
    char pad20[0x16];
    short f36;
} Shared36;
void update_resort_owner_state_transitions(void *mp) {
    Moby *moby = mp;
    Data_2D2670 *d = (Data_2D2670 *)moby->pvars;
    float v[4];
    float w[4];
    int r = classify_resort_owner_target_proximity_r(moby);

    if (moby->state >= 3 && moby->state <= 5 && r == 0) {
        set_resort_owner_state_r(moby, 6);
        return;
    }
    if (moby->state >= 4 && moby->state <= 6) {
        vec_scale_to_length(v, &moby->rot_row0, 3.0f);
        FastVecAdd(v, v, &moby->pos);
        if (try_find_resort_owner_jump_target_r(moby, v)) {
            qcopy(d->target, v);
            d->target[2] = probe_ground_height_flag20(d->target);
            d->next = 3;
            set_resort_owner_state_r(moby, 7);
        }
    }
    switch (moby->state) {
        case 0:
            DialogRequest_Init(moby, d->f20);
            moby->update_dist = 0xFF;
            build_symmetric_adjacency_masks(&d->a[0], 1, d->b[0]);
            build_symmetric_adjacency_masks(&d->a[1], 1, d->b[1]);
            build_symmetric_adjacency_masks(&d->a[2], 1, d->b[2]);
            clear_u64_value(d->f130);
            clear_u64_value(d->f140);
            if (gHavePersuader[0xB] != 0) {
                if (saved_mission_state[d->f10C + (g_current_level_index << 4)] == 0xFF) {
                    BossL11_SetPartFlag_r((char *)pokitaru_mobys_d + (d->fF0[0] << 8));
                    BossL11_SetPartFlag_r((char *)pokitaru_mobys_d + (d->fF0[1] << 8));
                    trigger_pokitaru_paired_rotation_r((char *)pokitaru_mobys_d + (d->f104 << 8));
                    DeleteMoby(moby);
                    return;
                }
            } else if (saved_mission_state[d->f10C + (g_current_level_index << 4)] == 0xFF) {
                int ofs;
                int base;
                d->idx = 3;
                ofs = d->fE0[3] << 7;
                base = (int)path_points_14b0_r;
                qcopy(&moby->pos, (char *)(ofs + base + 0x30));
                qcopy(&moby->rot, (char *)(ofs + base + 0x70));
                trigger_pokitaru_paired_rotation_r((char *)pokitaru_mobys_d + (d->f104 << 8));
                set_resort_owner_state_r(moby, 0xE);
                d->f2C = 10.0f;
                BossL11_SetPartFlag_r((char *)pokitaru_mobys_d + (d->fF0[0] << 8));
                BossL11_SetPartFlag_r((char *)pokitaru_mobys_d + (d->fF0[1] << 8));
                return;
            }
            if (saved_mission_state[d->f108 + (g_current_level_index << 4)] == 0xFF &&
                fast_vec_dist(&g_hero.motion.pos, path_points_14b0_r[d->fE0[1]].pos) < 12.0f) {
                int ofs;
                int base;
                d->idx = 1;
                ofs = d->fE0[1] << 7;
                base = (int)path_points_14b0_r;
                qcopy(&moby->pos, (char *)(ofs + base + 0x30));
                qcopy(&moby->rot, (char *)(ofs + base + 0x70));
                set_resort_owner_state_r(moby, 6);
                path_points_14b0_r[d->f114].pos[2] -= 500.0f;
                BossL11_SetPartFlag_r((char *)pokitaru_mobys_d + (d->fF0[0] << 8));
            } else {
                path_points_14b0_r[d->f110].pos[2] -= 500.0f;
                path_points_14b0_r[d->f114].pos[2] -= 500.0f;
                d->idx = 0;
                d->f2C = 5.0f;
                set_resort_owner_state_r(moby, 1);
            }
            break;
        case 1:
            if (d->f56 == 0) {
                if (g_hero.air_frames.s == 0 && Npc_TryStartDialog(moby, d->f20)) {
                    begin_dialogue_camera_transition(2.5f, moby);
                    set_resort_owner_state_r(moby, 2);
                }
            } else if (vec_distance_xy(&moby->pos, &g_hero.motion.pos) < 6.0f) {
                set_resort_owner_state_r(moby, 3);
            }
            break;
        case 2:
            if (pokitaru_cinematic_mode != 2) {
                set_resort_owner_state_r(moby, 3);
            }
            break;
        case 3:
            if (vec_distance_xy(&moby->pos, &g_hero.motion.pos) > 6.0f &&
                try_update_resort_owner_path_target_r(moby, &g_hero.motion.pos) && r == 1) {
                set_resort_owner_state_r(moby, 4);
            }
            break;
        case 4:
            if (vec_distance_xy(&moby->pos, &g_hero.motion.pos) < 3.0f ||
                !try_update_resort_owner_path_target_r(moby, &g_hero.motion.pos) || r == 2) {
                set_resort_owner_state_r(moby, 3);
            } else if (vec_distance_xy(&moby->pos, &g_hero.motion.pos) > 8.0f) {
                set_resort_owner_state_r(moby, 5);
            }
            break;
        case 5:
            if (vec_distance_xy(&moby->pos, &g_hero.motion.pos) < 3.0f ||
                !try_update_resort_owner_path_target_r(moby, &g_hero.motion.pos) || r == 2) {
                set_resort_owner_state_r(moby, 3);
            } else if (vec_distance_xy(&moby->pos, &g_hero.motion.pos) < 4.0f) {
                set_resort_owner_state_r(moby, 4);
            }
            break;
        case 6:
            try_update_resort_owner_path_target_r(moby, path_points_14b0_r[d->fE0[d->idx]].pos);
            if (vec_distance_xy(&moby->pos, d->target) < 1.0f) {
                switch (d->idx) {
                    case 0: {
                        Moby *o = (Moby *)((char *)pokitaru_mobys_d + (d->fF0[0] << 8));
                        qcopy(d->target, &o->pos);
                        vec_scale_to_length(v, &o->rot_row0, resort_owner_approach_offset_forward);
                        FastVecAdd(d->target, d->target, v);
                        vec_scale_to_length(v, &o->rot_row1, resort_owner_approach_offset_side);
                        FastVecAdd(d->target, d->target, v);
                        vec_scale_to_length(v, &o->rot_row2, resort_owner_approach_offset_up);
                        FastVecAdd(d->target, d->target, v);
                        d->next = 8;
                        set_resort_owner_state_r(moby, 7);
                        break;
                    }
                    case 1: {
                        Moby *o = (Moby *)((char *)pokitaru_mobys_d + (d->fF0[1] << 8));
                        qcopy(d->target, &o->pos);
                        vec_scale_to_length(v, &o->rot_row0, resort_owner_approach_offset_forward);
                        FastVecAdd(d->target, d->target, v);
                        vec_scale_to_length(v, &o->rot_row1, -resort_owner_approach_offset_side);
                        FastVecAdd(d->target, d->target, v);
                        vec_scale_to_length(v, &o->rot_row2, resort_owner_approach_offset_up);
                        FastVecAdd(d->target, d->target, v);
                        path_points_14b0_r[d->f110].pos[2] += 500.0f;
                        d->next = 8;
                        set_resort_owner_state_r(moby, 7);
                        break;
                    }
                    case 2:
                        set_resort_owner_state_r(moby, 0xB);
                        break;
                    case 3:
                        path_points_14b0_r[d->f114].pos[2] += 500.0f;
                        set_resort_owner_state_r(moby, 0xC);
                        break;
                }
            }
            break;
        case 12:
            d->f2C = 255.0f;
            if (Npc_TryStartDialog(moby, d->f20)) {
                int ofs;
                int base;
                jet_clear_persistent(d->f10C);
                menu_item_enabled_69[3] = 1;
                set_moby_dialogue_record_value(moby, 2);
                ofs = d->fE0[d->idx] << 7;
                base = (int)path_points_14b0_r;
                qcopy(&moby->pos, (char *)(ofs + base + 0x30));
                qcopy(&moby->rot, (char *)(ofs + base + 0x70));
                euler_rotation_basis(&moby->rot_row0, &moby->rot);
                vec_scale_to_length(v, &moby->rot_row0, 2.0f);
                FastVecAdd(v, v, &moby->pos);
                qcopy(w, &moby->rot);
                w[2] = FastAddRots(w[2], 3.14159f);
                qcopy(pokitaru_cutscene_pose_request.a, v);
                qcopy(pokitaru_cutscene_pose_request.b, w);
                ((Shared36 *)&pokitaru_cutscene_pose_request)->f36 = 1;
                begin_dialogue_camera_transition(2.5f, moby);
                set_resort_owner_state_r(moby, 0xD);
            }
            break;
        case 13:
            if (pokitaru_cinematic_mode != 2) {
                trigger_pokitaru_paired_rotation_r((char *)pokitaru_mobys_d + (d->f104 << 8));
                set_resort_owner_state_r(moby, 0xE);
            }
            break;
        case 7:
            if (moby->anim_flags & 2) {
                set_resort_owner_state_r(moby, d->next);
            }
            break;
        case 8:
            if (vec_distance_xy(&moby->pos, d->target) < 0.5f) {
                set_resort_owner_state_r(moby, 9);
            }
            break;
        case 9: {
            int k = d->idx != 0;
            if (is_moby_in_state_five_r((char *)pokitaru_mobys_d + (d->fF0[k] << 8))) {
                set_resort_owner_state_r(moby, 0xA);
            }
            break;
        }
        case 10:
            if (vec_distance_xy(&moby->pos, d->target) < 0.5f) {
                d->idx++;
                qcopy(d->target, path_points_14b0_r[d->fD0[d->idx]].pos);
                d->next = 4;
                set_resort_owner_state_r(moby, 7);
            }
            break;
        case 11:
            if (is_moby_in_state_five_r((char *)pokitaru_mobys_d + (d->f100 << 8))) {
                d->idx++;
                set_resort_owner_state_r(moby, 3);
            }
            break;
        case 14:
            if (saved_mission_state[d->f118 + (g_current_level_index << 4)] == 0xFF && g_hero.state.current != 0x32 &&
                Npc_TryStartDialog(moby, d->f20)) {
                begin_dialogue_camera_transition(2.5f, moby);
                set_resort_owner_state_r(moby, 0xF);
            }
            break;
        case 15:
            if (pokitaru_cinematic_mode != 2) {
                grant_inventory_item(6, 1);
                memcard_Save(0, -1);
                help_request(0x2AF8, 0x3A);
                DeleteMoby(moby);
            }
            break;
    }
}

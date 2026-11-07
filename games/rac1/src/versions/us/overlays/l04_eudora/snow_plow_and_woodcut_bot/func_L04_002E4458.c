void lieutenant_update(char *m) {
    char *data = ((char *)((Moby *)m)->pvars);
    float a[4];
    float b[4];
    int g;
    int c;
    if (((Moby *)m)->visible != 0) {
        if (fast_vec_dist(m + 0x10, eudora_camera_pos) < 30.0f) {
            set_ground_height_band(m);
            ((Moby *)m)->shadow_range = 0x18;
        }
    }
    attach_big_head_cheat_manipulator_to_matching_moby(m);
    switch (((Moby *)m)->state) {
        case 0:
            c = ((Moby *)m)->mission;
            g = g_current_level_index;
            if (saved_mission_state[c + (g << 4)] == 0xFF)
                goto done;
            ((Moby *)m)->state = 1;
            DialogRequest_Init(m, data);
            break;
        case 1:
            if (is_point_inside_clip_volume(&g_hero.motion.pos, ((struct LieutenantUpdateVars *)data)->unk_140) != 0) {
                ((struct LieutenantUpdateVars *)data)->unk_0C = 255.0f;
            } else {
                ((struct LieutenantUpdateVars *)data)->unk_0C = 1.0f;
            }
            if (Npc_TryStartDialog(m, data) != 0) {
                ((Moby *)m)->state = 2;
            }
            break;
        case 2:
            if (((struct LieutenantUpdateVars *)data)->unk_04 == 2) {
                unlock_planet(6);
                jet_clear_persistent(((Moby *)m)->mission);
                vec_scale_to_length(a, m + 0xC0, 2.0f);
                FastVecAdd(a, a, m + 0x10);
                qcopy(b, m + 0x40);
                b[2] = FastAddRots(b[2], 3.14159f);
                qcopy(eudora_lieutenant_respawn_point, a);
                qcopy(eudora_lieutenant_respawn_point + 0x10, b);
                *(short *)(eudora_lieutenant_respawn_point + 0x36) = 1;
                memcard_Save(0, -1);
            done:
                ((Moby *)m)->state = 3;
                ((Moby *)m)->flags |= 0x41;
                *(int *)&((Moby *)m)->collision = 0;
            } else if (*(int *)&eudora_game_mode != 2) {
                Npc_TryStartDialog(m, data);
            }
            break;
        case 3:
            *(int *)&((Moby *)m)->collision = 0;
            ((Moby *)m)->flags |= 0x41;
            break;
    }
}

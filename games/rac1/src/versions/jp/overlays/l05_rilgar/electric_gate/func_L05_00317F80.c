void update_hoverboard_girl(Moby *moby) {
    HGState *state = (HGState *)moby->pvars;
    HGVec target_position;
    HGVec head_position;
    HGVec direction;
    int animation;
    int look_enabled;
    float rotation_step;
    float rotation_limit;
    float heading;
    float yaw;
    float pitch;
    float delta;

    update_hoverboard_girl_cutscene_instances_r(moby);
    switch (moby->state) {
    case 0:
        if (moby->prev_seq != 0) {
            blend_moby_animation(moby, 0, 0, scale_frame_count(20));
        }
        moby->pos.z = find_ground_height(0.5f, (HGVec *)&moby->pos, 0);
        if ((*(HGGates *)(race_gates_base + 0xB3)).first_gate == 0 || (*(HGGates *)(race_gates_base + 0xB3)).second_gate == 0) {
            HGRaceEntry **entries = race_entries_r;
            int entry = get_dialogue_entry_r(moby);
            entries[entry]->state = 13;
        }
        state->dialogue_name = race_girl_name;
        moby->state = 1;
        initialize_npc_dialogue(moby, state);
        initialize_moby_grounding_r(moby);
        break;
    case 1:
        if ((*(Hero *)(hero_globals_base + 0xE1D)).state.current != 0x1D) {
        if (update_npc_dialogue(moby, state) != 0) {
            begin_dialogue_camera_transition(2.0f, moby);
            moby->state = 3;
        }
        if (state->dialogue_state == 3) {
            state->previous_dialogue_state = state->dialogue_state;
            state->dialogue_state = 2;
            state->dialogue_state_frame = frame_number;
            moby->command = 0;
            set_race_camera(&race_camera_vectors[0], &race_camera_vectors[1], 0, 1);
            qcopy(&((HGVec *)(hero_globals_base + 0x2B1D))[0], &race_camera_vectors[2]);
            qcopy(&((HGVec *)(hero_globals_base + 0x2B1D))[1], &race_camera_vectors[3]);
            (*(Hero *)(hero_globals_base + 0xE1D)).item_use_locked = 1;
            set_race_camera_mode(2, 6);
        }
        }
        if (moby->command == 1) {
            set_race_camera(&race_camera_vectors[2], &race_camera_vectors[3], 0, 1);
            set_race_camera_mode(0, 8);
            if ((*(unsigned char *)(special_items_base + 0x13B)) == 0) {
                state->previous_dialogue_state = state->dialogue_state;
                state->dialogue_state = 4;
                state->dialogue_state_frame = frame_number;
                state->race_return_pending = 1;
            }
            moby->command = 0;
        }
        if (moby->command == 2) {
            set_race_camera_mode(0, 8);
            set_race_camera(&race_camera_vectors[2], &race_camera_vectors[3], 0, 1);
            moby->command = 0;
        }
        if (moby->command == 3) {
            set_race_camera_mode(0, 8);
            moby->command = 0;
            set_race_camera(&race_camera_vectors[0], &race_camera_vectors[1], 0, 1);
            qcopy(&((HGVec *)(hero_globals_base + 0x2B1D))[0], &race_camera_vectors[2]);
            qcopy(&((HGVec *)(hero_globals_base + 0x2B1D))[1], &race_camera_vectors[3]);
        }
        if ((moby->prev_seq == 0 || moby->prev_seq == 2) &&
            advance_timer(&state->idle_animation_timer)) {
            state->idle_animation_timer = truncate_time(scale_time(random_float(1200.0f, 2400.0f)));
            if (moby->prev_seq != 1) {
                blend_moby_animation(moby, 1, 0, scale_frame_count(10));
            }
        } else if (moby->anim_flags & 2) {
            animation = random_remainder(2) != 0 ? 0 : 2;
            if (moby->prev_seq != animation) {
                blend_moby_animation(moby, animation, 0, scale_frame_count(10));
            }
        }
        break;
    case 3:
        if (game_mode != 2) {
            moby->state = 1;
            if (state->previous_dialogue_state == 4) {
                (*(unsigned char *)(special_items_base + 0x13B)) = 1;
                queue_dialogue_message(0x1395, scale_frame_count(300));
                if (menu_item_enabled_115[0xA] != 0) {
                    func_0022EE28_x(1, 0, 0);
                    queue_dialogue_message(0x542D, -1);
                }
                func_0020BFC8_x(0, -1);
            }
        }
        break;
    default:
        if (moby->command != 0) {
            set_race_camera(&race_camera_vectors[2], &race_camera_vectors[3], 0, 1);
            moby->command = 0;
        }
        break;
    }

    rotation_step = 0.02f;
    rotation_limit = 0.3f;
    look_enabled = 0;
    if (moby->prev_seq == 0 || moby->prev_seq == 2) {
        look_enabled = 1;
        if (vector_distance(&moby->pos, &(*(Hero *)(hero_globals_base + 0xE1D)).motion.pos) < 8.0f &&
            absolute_angle_difference(
                moby->rot.z,
                angle_from_xy((*(Hero *)(hero_globals_base + 0xE1D)).motion.body_center.f[0] - moby->pos.x,
                              (*(Hero *)(hero_globals_base + 0xE1D)).motion.body_center.f[1] -
                                  moby->pos.y)) < 1.5707964f) {
            if (vector_length(&(*(Hero *)(hero_globals_base + 0xE1D)).motion.displacement) > 0.01f) {
                state->player_look_timer = scale_frame_count(120);
            } else {
                advance_timer(&state->player_look_timer);
            }
        } else if (state->player_look_timer != 0) {
            state->player_look_timer = 0;
            qcopy(&state->look_target, &(*(Hero *)(hero_globals_base + 0xE1D)).motion.body_center);
        }
        if (advance_timer(&state->random_look_timer)) {
            state->random_look_timer = truncate_time(scale_time(random_float(180.0f, 300.0f)));
            heading = add_angle(moby->rot.z, random_float(-90.0f, 90.0f) * 0.017453292f);
            pitch = random_float(0.0f, 30.0f) * 0.017453292f;
            vector_from_angles(6.0f, heading, pitch, &state->look_target);
            add_vector(&state->look_target, &state->look_target, &moby->pos);
        }
        if (state->player_look_timer != 0) {
            qcopy(&target_position, &(*(Hero *)(hero_globals_base + 0xE1D)).motion.body_center);
            rotation_step = 0.03f;
            rotation_limit = 0.3f;
        } else {
            qcopy(&target_position, &state->look_target);
        }
    }
    if (look_enabled) {
        qcopy(&head_position, &moby->pos);
        head_position.component[2] += 1.0f;
        subtract_vector(&direction, &target_position, &head_position);
        yaw = signed_angle_difference(angle_from_xy(direction.component[0], direction.component[1]),
                                      moby->rot.z);
        pitch = -angle_from_xy(vector_length_xy(&direction), direction.component[2]);
        if (yaw > 1.5707964f)
            yaw = 1.5707964f;
        else if (yaw < -1.5707964f)
            yaw = -1.5707964f;
        if (pitch > 0.5235988f)
            pitch = 0.5235988f;
        else if (pitch < -0.5235988f)
            pitch = -0.5235988f;
        state->joints[0].target_rotation[1] = pitch;
        state->joints[0].target_rotation[2] = yaw * 0.6f;
        state->joints[1].target_rotation[2] = yaw * 0.4f;
    }

    if (vector_distance(&moby->pos, &(*(Hero *)(hero_globals_base + 0xE1D)).motion.pos) < 15.0f &&
        absolute_angle_difference(
            moby->rot.z,
            angle_from_xy((*(Hero *)(hero_globals_base + 0xE1D)).motion.pos.f[0] - moby->pos.x,
                          (*(Hero *)(hero_globals_base + 0xE1D)).motion.pos.f[1] - moby->pos.y)) <
            1.2217305f) {
        if ((*(Hero *)(hero_globals_base + 0xE1D)).state.current == 11 && (*(Hero *)(hero_globals_base + 0xE1D)).state_timer == 15) {
            state->breast_growth_count_gp++;
            if (state->breast_growth_count_gp > 20)
                state->breast_growth_count_gp = 20;
            breast_growth_count_gp_q = state->breast_growth_count_gp;
        }
        if ((*(Hero *)(hero_globals_base + 0xE1D)).state.current == 4 && scale_frame_count(20) < (*(Hero *)(hero_globals_base + 0xE1D)).state_timer &&
            frame_number % 10 == 0) {
            state->breast_growth_count_gp--;
            if (state->breast_growth_count_gp < 0)
                state->breast_growth_count_gp = 0;
            breast_growth_count_gp_q = state->breast_growth_count_gp;
        }
    }
    if (game_mode != 2) {
        update_hoverboard_girl_boobs_animation_r(moby, state);
        if ((gCheats[0]) != 0)
            state->joints[0].target_scale = 2.75f;
        update_joint_animation(moby, &state->joints[0], 0, rotation_step * frame_scale_sq,
                               rotation_limit * frame_scale_sq);
        {
            float fs = frame_scale_sq;
            update_joint_animation(moby, &state->joints[1], 1, rotation_step * fs, rotation_limit * fs);
        }
    }
}

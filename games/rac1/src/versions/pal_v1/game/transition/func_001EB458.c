void update_gameplay_frame(void) {
    Vec4A first_position;
    Vec4A next_position;
    RenderSequenceActor_EB458 *actor;
    Vec4A *animation_positions;
    s32 actor_index;
    f32 fade;

    noop_callback_i_r();
    fade = gFade_r - 0.0625f;
    render_sequence_r2.frame++;
    render_sequence_r2.time++;
    gFade_r = fade;
    if (fade < 0.0f) {
        gFade_r = 0.0f;
    }
    if (render_sequence_r2.time >= render_sequence_r2.end_time) {
        render_sequence_r2.time = 0;
        render_sequence_r2.sequence_frame = 0;
        parse_space_scene_chunk_r2(0);
    } else if (render_sequence_r2.frame >= 0x60) {
        parse_space_scene_chunk_r2(++render_sequence_r2.sequence_frame);
    }
    Transition_UpdateMovieCamera_r();
    for (actor_index = 0; actor_index < render_sequence_r2.count; actor_index++) {
        u8 frame_index;
        actor = render_sequence_r2.actors[actor_index];
        frame_index = render_sequence_r2.frame >> 1;
        actor->current_frame = render_sequence_r2.frame >> 1;
        actor->next_frame = frame_index + 1;
        update_moby_animation_state_r(actor);
        actor->frame_fraction = IntToFloat_r(render_sequence_r2.frame & 1) * 0.5f;
        animation_positions = ((RenderSequenceSidecar_EB458 *)actor)->animation_positions;
        FastVecScale_r(&first_position, &animation_positions[actor->current_frame],
                            1.0f - actor->frame_fraction);
        FastVecScale_r(&next_position, &animation_positions[actor->next_frame],
                            actor->frame_fraction);
        FastVecAdd_r(&actor->position, &first_position, &next_position);
        actor->cached_frame = 0xFF;
        refresh_moby_spatial_bounds_r(actor);
        actor->update_enabled = 0;
        if (actor->class_id == 0) {
            noop_callback_e_r(actor);
        }
    }
    noop_callback_h_r();
    if (g_game_mode_r == 0) {
        intro_overlay_timer_r++;
        if (scale_game_frame_count_r(0x3C) < intro_overlay_timer_r) {
            if (++intro_overlay_alpha_r > 0x40) {
                intro_overlay_alpha_r = 0x40;
            }
        }
        if (scale_game_frame_count_r(0x78) < intro_overlay_timer_r) {
            language_intro_overlay_alpha_r =
                (s32)(FastCos_r((intro_overlay_timer_r - scale_game_frame_count_r(0x78)) % 60 * 0.10471976f +
                                          -3.1415927f) *
                      32.0f) +
                0x60;
        }
        if (pad0_pressed_r[0] & 0x840) {
            initialize_transfer_command_r();
        }
        sound_update_r();
    } else if (g_game_mode_r == 3) {
        intro_overlay_timer_r = scale_game_frame_count_r(0x3C);
        if ((intro_overlay_alpha_r -= 0x10) < 0) {
            intro_overlay_alpha_r = 0;
        }
        if ((language_intro_overlay_alpha_r -= 0x10) < 0) {
            language_intro_overlay_alpha_r = 0;
        }
        update_menu_system_r();
        sound_update_r();
    } else if (g_game_mode_r == 4) {
        update_mode_freeze_r();
        sound_update_r();
    }
}

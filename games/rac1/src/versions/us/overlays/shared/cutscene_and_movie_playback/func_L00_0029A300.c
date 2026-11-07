void update_cutscene_playback(void) {
    VendorCutscene *cs;
    Moby *m;
    s32 done;
    s32 last;
    s32 i;
    s32 id;
    s32 lim;
    Vec4 *keys;
    f32 fade;
    f32 a[4] __attribute__((aligned(16)));
    f32 b[4] __attribute__((aligned(16)));

    if ((g_debug_menu_r.update_flags & 2) || ((g_debug_menu_r.update_flags & 0x10) && (pad0_pressed & PAD_L3))) {
        reset_frame_counters();
    }
    if (!(g_debug_menu_r.update_flags & 0x10) || (pad0_pressed & (PAD_CIRCLE | PAD_L3))) {
        apply_level_specific_cutscene_moby_effects();
    }
    if ((g_debug_menu_r.update_flags & 2) || ((g_debug_menu_r.update_flags & 0x10) && (pad0_pressed & PAD_L3))) {
        update_visible_resident_objects();
        enqueue_voice_request();
    }
    Profile_Mark_v(level_profile_mark_after_mobys_r, 3);
    if ((g_debug_menu_r.update_flags & 1) || ((g_debug_menu_r.update_flags & 0x10) && (pad0_pressed & PAD_L3))) {
        coordinate_hero_frame_update();
    }
    Profile_Mark_v(level_profile_mark_after_hero_r, 7);
    if ((g_debug_menu_r.update_flags & 4) || ((g_debug_menu_r.update_flags & 0x10) && (pad0_pressed & PAD_L3))) {
        UpdateParts();
    }
    Profile_Mark_v(level_profile_mark_after_parts_r, 5);
    cs = &cutscene_state_r;
    fade = gFadeLvl_r - 0.34f;
    cs->f34++;
    cs->f38++;
    gFadeLvl_r = fade;
    if (gFadeLvl_r < 0.0f) {
        gFadeLvl_r = 0.0f;
    }
    if (cs->f34 >= cs->h48) {
        continue_audio_stream_if_ready();
    }
    done = cs->f34 >= cs->h40;
    if (cs->f34 >= scale_ticks(0x12) && gFadeLvl_r == 0.0f) {

        if (((challenge_mode) || D_0015EF20 || decode_mode || g_current_level_index <= 0) &&
            (g_controller_state_r.buttons.w.pressed & 0x800)) {
            done = 1;
        } else if ((g_controller_state_r.buttons.held_pressed & 0x8000000000FUL) == 0x8000000000FUL) {
            done = 1;
        }
    }
    if (done) {
        finish_cutscene();
    } else {
        lim = pal_mode ? 0x50 : 0x60;
        if (cutscene_state_r.f38 >= lim) {
            update_audio_stream_until_idle(1);
            cutscene_state_r.f3C++;
            update_world_object_animation();
            start_cutscene_chunk_read(cutscene_state_r.f3C + 1);
        }
        last = func_L00_0029A158_9A300();
        for (i = 0; i < cutscene_state_r.h44; i++) {
            m = cutscene_state_r.f178[i];
            id = cutscene_state_r.f38 >> 1;
            m->frame = id;
            m->prev_frame = id + 1;
            update_moby_animation_state(m);
            m->anim_time = IntToFloat(cutscene_state_r.f38 & 1) * 0.5f;
            if (last && (cutscene_state_r.f38 & 1)) {
                m->anim_time = 1.0f;
            }
            keys = (Vec4 *)m->pvars;
            FastVecScale(a, &keys[m->frame], 1.0f - m->anim_time);
            FastVecScale(b, &keys[m->prev_frame], m->anim_time);
            FastVecAdd(&m->pos, a, b);
            m->unk71 = 0xFF;
            moby_build_matrix(m);
            if (m->shadow_range) {
                update_moby_shadow_range(m);
            }
            if (m->oclass == MOBY_CLASS_RATCHET) {
                HeroAttachManipulators(m);
            }
            if (m->oclass == 10 || m->oclass == MOBY_CLASS_GIANT_CLANK || m->oclass == 0x555) {
                update_moby_pulsing_glow_effects(m);
            }
        }
    }
    Profile_Mark_v(level_profile_mark_after_camera_r, 8);
    sound_update();
    Profile_Mark_v(level_profile_mark_after_sound_r, 6);
    UpdateAllPointLights();
    update_resident_packed_direction();
    if ((g_debug_menu_r.update_flags & 2) || ((g_debug_menu_r.update_flags & 0x10) && (pad0_pressed & PAD_L3))) {
        update_game_frame_and_idle_counters();
    }
}

void render_level_frame(void) {
    f32 rotation_angle;
    f32 screen_y;
    f32 quad_extent;
    s32 overlay_alpha;
    f32 saved_projection_scale;
    struct LevelDisplayState *display_state;
    s64 texture;
    append_gif_transfer_packet();
    update_camera_view_matrices();
    prune_moby_references();
    ResetGsRegisters();
    saved_projection_scale = view_context.projection_scale;
    D_0015F704_305A0 = -1;
    if (saved_projection_scale < 0.63f) {
        view_context.projection_scale = 0.63f;
    }
    UpdateViewContext();
    update_camera_view_matrices();
    draw_sky_shells();
    view_context.projection_scale = saved_projection_scale;
    UpdateViewContext();
    update_camera_view_matrices();
    if (level_render_state_r2.mode == 4) {
        SetupGifPaging(1);
        draw_resident_textured_quad();
        DoGifPaging();
    }
    DrawMobys();
    append_dma_tag(0x02080000);
    SetupGifPaging(1);
    if ((g_current_level_index_r != LEVEL_VELDIN1) &&
        ((g_current_level_index_r != LEVEL_NOVALIS) || (level_available_base[0] != 0))) {
        build_resident_indexed_texture_warp_meshes(render_sequence_slots[0]);
    }
    if ((level_render_state_r2.mode == 4) && (render_sequence_time[0] >= 0x3D)) {
        overlay_alpha = (render_sequence_time[0] - 0x3C) * 2;
        if (overlay_alpha >= 0x81) {
            overlay_alpha = 0x80;
        }
        draw_resident_textured_banner(overlay_alpha);
    }
    if ((g_current_level_index_r != LEVEL_VELDIN1) &&
        ((g_current_level_index_r != LEVEL_NOVALIS) || (level_available_base[0] != 0))) {
        render_environment_mapped_object_r(render_sequence_slots[0]);
    }
    if ((memory_card_state_r.busy >= 3) || (memory_card_state_r.state >= 0)) {
        VU1_addGSregister(0x47, 0x3004B);
        quad_extent = 272.0f;
        texture = GetEffectTex_r(2);
        display_state = &g_screen_extent_r;
        DrawTexturedQuad(0x2C, display_state->screen_height - 0x60, 0x40, 0x40, 0, 0, 0x40, 0x40, 0x80808080, texture);
        screen_y = (f32)((display_state->screen_height - 0x40) * 16);
        rotation_angle = ((game_frame_counter % 55) * (-6.2831855f)) / 55.0f;
        /* The frame counter contributes only the sprite angle modulo 55. */
        /* Retail passes the full texture value in a2 and the five floats in f12-f16. */
        draw_rotated_sprite_v(1216.0f, screen_y, quad_extent, 272.0f, 0x40, 0x40, GetEffectTex_r(3), rotation_angle);
    }
    DoGifPaging();
    if (gFade > 0.0f) {
        if (gFade > 1.0f) {
            gFade = 1.0f;
        }
        emit_rgba_draw_packet(0, 0, 0, truncate_float_to_s32(gFade * 128.0f));
    }
    VU1_syncChain(0x10);
    PatchMobyGifs();
}

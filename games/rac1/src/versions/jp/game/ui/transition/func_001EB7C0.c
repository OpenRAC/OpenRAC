extern s32 D_J_0015F050 MACRO_ADDR;
extern s32 D_J_0015F054 MACRO_ADDR;
extern s32 D_J_0015F05C MACRO_ADDR;
extern s64 D_J_0015F048 SDATA(D_J_0015F048);
void Transition_DefaultDraw(s32 *arg0) {
    s32 n;

    if (level_sky_effect_data == 0 || level_sky_effect_data->unk4 != 0) {
        append_gif_transfer_packet();
    }
    FastMemSet(occl_visibility, -1, 0x80);
    Transition_UpdateMovieCamera_r2();
    update_camera_view_matrices();
    prune_moby_references();
    ResetGsRegisters();
    font_state = -1;
    if (level_sky_effect_data != 0) {
        Transition_DrawSky_r();
    }
    DrawTfrag();
    append_dma_tag(0x02010000);
    DrawTies_1();
    append_dma_tag(0x02020000);
    DrawShrubs();
    append_dma_tag(0x02040000);
    if (g_game_mode == 3) {
        render_level_effects_and_screen_sprites();
    } else {
        DrawMobys();
    }
    append_dma_tag(0x02080000);
    SetupGifPaging(0);
    vu1_gs_regs_alt();
    if (draw_callback_count != 0) {
        dispatch_callback_list_1();
    }
    vu1_gs_regs_alt();
    if (transition_draw_particles[0] != 0) {
        VU1_addGSregister_r(8, 5);
        vu1_gs_regs_alt();
        flush_cache(0);
        PartProc();
        font_state = 8;
    }
    AA_BlurPass();
    ResetGsRegisters();
    if (D_J_0015F050 != 0) {
        DrawTexturedQuad(0xEC, 0x10, 0x100, 0x80, 0, 0, 0x100, 0x80, (long)(D_J_0015F050 << 24 | 0x808080),
                         (D_J_0015F048));
    }
    if (D_J_0015F054 != 0) {
        n = (g_game_language)-1;
        if (n < 0) {
            n = 0;
        }
        DrawTexturedQuad(0xEA, 0x64, 0x100, 0x80, 0, 0, 0x100, 0x80, (long)(D_J_0015F054 << 24 | 0x808080),
                         GetEffectTex_v(n + 4));
    }
    if (D_J_0015F05C != 0) {
        DrawTexturedQuad(0x60, 0x134, 0x100, 0x80, 0, 0, 0x100, 0x80, (long)(D_J_0015F05C << 24 | 0x808080),
                         GetEffectTex_v(5));
        DrawTexturedQuad(0x160, 0x134, 0x100, 0x80, 0, 0, 0x100, 0x80, (long)(D_J_0015F05C << 24 | 0x808080),
                         GetEffectTex_v(6));
    }
    DoGifPaging();
    if (gFade > 0.0f) {
        if (gFade > 1.0f) {
            gFade = 1.0f;
        }
        emit_rgba_draw_packet(0, 0, 0, truncate_float_to_s32(gFade * 128.0f));
    }
    VU0_loadMicroProgram(frame_vu0_micro_program);
    flush_cache(0);
    if (g_game_mode == 4) {
        draw_freeze_message_box();
    }
    VU1_syncChain(2);
    LightTfrags(tfrag_dma_block);
    PatchTfragGifs();
    VU1_syncChain(4);
    LightTies(tie_light_data);
    PatchTieGifs();
    VU1_syncChain(8);
    LightShrubs(shrub_scratchpad_copy);
    patch_shrub_gifs();
    VU1_syncChain(0x10);
    PatchMobyGifs();
    UpdateFog();
}

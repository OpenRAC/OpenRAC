extern int start_audio_stream_read(int, int, int);
extern short update_audio_stream_until_idle(int);
void startlevel(void) {
    LevelHeader *hdr;
    LevelChunk *tbl;
    s32 n;
    s32 i;
    s32 cur;
    s32 prev;
    s32 frames;
    s32 code;
    s32 bank;
    s32 fade;
    u8 *p;

    boot_level_active = 1;
    InitOnce();
    reset_gs_texture_allocation();
    for (i = 0; i < startlevel_clear_end - menu_state_frame_count; i++) {
        menu_state_frame_count[i] = 0;
    }
    prev = 0;
    frames = 0;
    fade = 0x80;
    VU1_initChain();
    DMAC_VIF1_Enable();
    reset_draw_config();
    game_frame_counter = 0;
    sce_gs_sync_v(0);
    g_game_mode_r = 0;
    ResetGsRegisters();
    ResetGsRegistersPr();
    PutDispBuffer();
    SetBackgroundColor_r(0, 0, 0);
    hdr = (LevelHeader *)(((u32)image_end_round_16k & 0xFFFFC000) + 0x2C0000);
    VU1_initChain();
    while ((cur = check_memory_card()) != 0 && (frames < 11 || g_controller_state.pressed == 0)) {
        if (cur != prev) {
            if (cur == 1) {
                tbl = hdr->intro;
            } else {
                tbl = hdr->loading;
            }
            flush_cache(0);
            fast_decompress(tbl[g_game_language].off + (s32)hdr, hdr->code + (s32)hdr);
            flush_cache(0);
        }
        SetBackgroundColor_r(0, 0, 0);
        PutDrawBufferLarge();
        append_gif_transfer_packet();
        queue_framebuffer_upload(hdr->code + (s32)hdr);
        if (frames < 8) {
            VU1_addGSregister(0x42, 0x8000000044);
            emit_rgba_draw_packet(0, 0, 0, fade);
        }
        PutDrawBufferSmall();
        append_palette_transfer_packet();
        VU1_sendChain();
        VU1_swapChain();
        VU1_syncChain(1);
        sce_gs_sync_v(0);
        prev = cur;
        fade -= 0x10;
        UpdatePad();
        frames++;
    }
    if (prev != 0) {
        FadeToBlack(8);
    }
    start_audio_stream_read(0x1400000, disc_table_r.unk14E8.sector, disc_table_r.unk14E8.size);
    prev = 0;
    cur = 0x80;
    flush_cache(0);
    fast_decompress(hdr->gfx_alt + (s32)hdr, hdr->code + (s32)hdr);
    flush_cache(0);
    VU1_initChain();
    do {
        SetBackgroundColor_r(0, 0, 0);
        PutDrawBufferLarge();
        append_gif_transfer_packet();
        queue_framebuffer_upload(hdr->code + (s32)hdr);
        if (prev < 0x20) {
            VU1_addGSregister(0x42, 0x8000000044);
            emit_rgba_draw_packet(0, 0, 0, cur);
        }
        PutDrawBufferSmall();
        prev++;
        append_palette_transfer_packet();
        cur -= 4;
        VU1_sendChain();
        VU1_swapChain();
        VU1_syncChain(1);
        sce_gs_sync_v(0);
        game_frame_counter++;
    } while (prev < 0x78);
    FadeToBlack(0x1E);
    prev = 0;
    cur = 0x80;
    flush_cache(0);
    fast_decompress(hdr->intro[2].off + (s32)hdr, hdr->code + (s32)hdr);
    flush_cache(0);
    do {
        SetBackgroundColor_r(0, 0, 0);
        PutDrawBufferLarge();
        append_gif_transfer_packet();
        queue_framebuffer_upload(hdr->code + (s32)hdr);
        if (prev < 0x20) {
            VU1_addGSregister(0x42, 0x8000000044);
            emit_rgba_draw_packet(0, 0, 0, cur);
        }
        PutDrawBufferSmall();
        prev++;
        append_palette_transfer_packet();
        cur -= 4;
        VU1_sendChain();
        VU1_swapChain();
        VU1_syncChain(1);
        sce_gs_sync_v(0);
        game_frame_counter++;
    } while (prev < 0x78);
    FadeToBlack(0x1E);
    update_audio_stream_until_idle(1);
    decode_mode_r = -1;
    code = hdr->code + (s32)hdr;
    startlevel_code_addr = code;
    if (pal_mode_r == 0) {
        Movie_Play_r(intro_movie_disc_entry_ntsc[0], intro_movie_disc_entry_ntsc[1], (code + 0x3F) & ~0x3F,
                     (code + 0x2C003F) & ~0x3F, 0);
    } else {
        Movie_Play_r(intro_movie_disc_entry_pal[0], intro_movie_disc_entry_pal[1], (code + 0x3F) & ~0x3F,
                     (code + 0x2C003F) & ~0x3F, 0);
    }
    decode_mode_r = 0;
    FadeToBlack(scale_game_frame_count(0xC));
    STUB_printf(str_loading_sound_bank);
    bank = load_audio_bank_by_location(disc_table_r.sound_bank);
    snd_ResolveBankXREFS();
    startlevel_sound_defs[0].bank = bank;
    startlevel_sound_defs[1].bank = bank;
    startlevel_sound_defs[2].bank = bank;
    startlevel_sound_defs[3].bank = bank;
    startlevel_sound_defs[4].bank = bank;
    startlevel_sound_defs[5].bank = bank;
    moby_class_472_sound_defs[0].bank = bank;
    moby_class_472_sound_defs[1].bank = bank;
    moby_class_472_sound_defs[2].bank = bank;
    moby_class_472_sound_defs[3].bank = bank;
    moby_class_472_sound_defs[4].bank = bank;
    startlevel_sound_defs[6].bank = bank;
    level_sound_defs = startlevel_sound_defs;
    level_sound_def_count = 7;
    transition_do_transition();
    if (pal_mode_r != requested_video_mode[0]) {
        pal_mode_r = requested_video_mode[0];
        reset_vif1_gif_r();
        sceGsResetGraph_r(0, 1, pal_mode_r != 0 ? 3 : 2, 0);
        SetPalMode();
        PutDispBuffer();
    }
    p = D_0013E130;
    space_load_level = g_current_level_index;
    *(s16 *)(p + 0x2A) = 1;
    g_current_level_index = -1;
    DoSpaceTransition();
}

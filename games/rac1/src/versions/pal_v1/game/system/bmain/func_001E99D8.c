extern u8 D_E1_001653B0[];
extern SoundSlot D_E1_00186080[];
extern SoundSlot D_E1_00186160[];
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
    u8 *p;

    boot_level_active = 1;
    InitOnce();
    reset_gs_texture_allocation();
    for (i = 0; i < D_E1_001653B0 - menu_state_frame_count; i++) {
        menu_state_frame_count[i] = 0;
    }
    prev = 0;
    frames = 0;
    VU1_initChain();
    DMAC_VIF1_Enable();
    reset_draw_config();
    game_frame_counter = 0;
    sce_gs_sync_v(0);
    g_game_mode_r = 0;
    PutDispBuffer();
    SetBackgroundColor_r(0, 0, 0);
    hdr = (LevelHeader *)(((u32)image_end_round_16k & 0xFFFFC000) + 0x2C0000);
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
            VU1_initChain();
            SetBackgroundColor_r(0, 0, 0);
            PutDrawBufferLarge();
            append_gif_transfer_packet();
            draw_boot_image(hdr->code + (s32)hdr);
            PutDrawBufferSmall();
            append_palette_transfer_packet();
            VU1_sendChain();
            VU1_swapChain();
            VU1_syncChain(1);
        }
        sce_gs_sync_v(0);
        prev = cur;
        UpdatePad();
        frames++;
    }
    if (prev != 0) {
        FadeToBlack(0xA);
    }
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
    FadeToBlack(scale_game_frame_count(0x12));
    flush_cache(0);
    if (pal_mode_r != 0) {
        fast_decompress(hdr->gfx_alt + (s32)hdr, hdr->code + (s32)hdr);
    } else {
        fast_decompress(hdr->gfx + (s32)hdr, hdr->code + (s32)hdr);
    }
    flush_cache(0);
    VU1_initChain();
    SetBackgroundColor_r(0, 0, 0);
    PutDrawBufferLarge();
    append_gif_transfer_packet();
    draw_boot_image(hdr->code + (s32)hdr);
    PutDrawBufferSmall();
    append_palette_transfer_packet();
    VU1_sendChain();
    VU1_swapChain();
    VU1_syncChain(1);
    sce_gs_sync_v(0);
    game_frame_counter++;
    STUB_printf(str_loading_sound_bank);
    bank = load_audio_bank_by_location(disc_table_r.sound_bank);
    snd_ResolveBankXREFS();
    D_E1_00186080[0].bank = bank;
    D_E1_00186080[1].bank = bank;
    D_E1_00186080[2].bank = bank;
    D_E1_00186080[3].bank = bank;
    D_E1_00186080[4].bank = bank;
    D_E1_00186080[5].bank = bank;
    D_E1_00186160[0].bank = bank;
    D_E1_00186160[1].bank = bank;
    D_E1_00186160[2].bank = bank;
    D_E1_00186160[3].bank = bank;
    D_E1_00186160[4].bank = bank;
    D_E1_00186080[6].bank = bank;
    level_sound_defs = D_E1_00186080;
    level_sound_def_count = 7;
    transition_do_transition();
    if (pal_mode_r != requested_video_mode[0]) {
        pal_mode_r = requested_video_mode[0];
        reset_vif1_gif_r();
        sceGsResetGraph_r(0, 1, pal_mode_r != 0 ? 3 : 2, 0);
        SetPalMode();
        PutDispBuffer();
    }
    p = level_render_state;
    space_load_level = g_current_level_index;
    *(s16 *)(p + 0x2A) = 1;
    g_current_level_index = -1;
    DoSpaceTransition();
}

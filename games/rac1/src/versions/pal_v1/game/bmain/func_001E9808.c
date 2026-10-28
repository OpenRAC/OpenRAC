void start_level(int level) {
    int a;
    int b;
    int t;

    if (level < 0) {
        return;
    }
    if (pal_mode != 0) {
        b = disc_table_g.alt[level].b;
        a = disc_table_g.alt[level].a;
    } else {
        b = disc_table_g.normal[level].b;
        a = disc_table_g.normal[level].a;
    }
    decode_mode = 2;
    voice_pool[0x6B] |= 8;
    flush_cache(0);
    sound_StopAllSounds();
    music_Stop();
    FadeToBlack(scale_game_frame_count(12));
    g_game_mode = 1;
    flush_cache(0);
    sound_StopAllSounds();
    music_Stop();
    snd_StreamSafeCdSync(0);
    for (;;) {
        char *ld = memory_card_state;
        if (*(int *)(ld + 0xD4) < 3 && *(int *)(ld + 0xDC) < 0) {
            break;
        }
        memcard_update_state();
    }
    {
        char *g = level_resource_buffers;
        t = *(int *)(g + 0x1C);
    }
    Movie_Play(b, a, t + 0x100000, t + 0x400000, 0);
    sce_cd_sync_s(0);
    sce_gs_sync_v(0);
    wait_for_graphics_pipeline_idle(0, 0);
    sceGsSyncVCallback(vsync_callback);
    Hud_sendTexture(0x1000000, depth_buffer_address, 0x1B, 6, 6, 1);
    decode_mode = 0;
    FadeToBlack(4);
    g_game_mode = 0;
    voice_pool[0x6B] |= 0x10;
}

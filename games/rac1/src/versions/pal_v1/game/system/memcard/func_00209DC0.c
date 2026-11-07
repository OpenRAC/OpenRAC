void load_and_initialize_level_chunk(void) {
    int a;
    int b;
    int s0;
    int s1;

    calculate_ring_buffer_bounds(disc_table[5] << 11, &a, &b);
    update_audio_stream_until_idle(1);
    Load(a, disc_table[4], disc_table[5]);
    s1 = sound_volume;
    s0 = music_volume;
    memcard_RestoreGame(a + *(int *)(a + 0x10));
    sound_volume = s1;
    music_volume = s0;
    g_current_level_index_m = 0;
}

/* No recovered name. Sets movie globals, runs initialization and playback, then clears them. */
int play_mpeg_movie(int a, int b, int c, char *state, int e) {
    decoder_buffer_address = c;
    movie_gp_base = state;
    change_thread_priority(get_thread_id(), 1);
    if (initAll(a, b, e)) {
        char *base = movie_gp_base;
        read_mpeg(base + 0xD9048, base, base + 0xD9040);
    }
    termAll();
    decoder_buffer_address = 0;
    decoder_state = 0;
    return 0;
}

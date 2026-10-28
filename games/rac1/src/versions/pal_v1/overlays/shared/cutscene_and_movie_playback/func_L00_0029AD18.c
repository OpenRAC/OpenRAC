void play_queued_movie(void) {
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
    Movie_Play(queued_movie_request, movie_request_source_mode,
                  (*(int *)&movie_request_decoder_buffer + 0x3F) & ~0x3F,
                  (movie_request_decoder_state + 0x3F) & ~0x3F, movie_request_audio_context);
    sce_cd_sync_s(0);
    movie_exit_to_gameplay();
}

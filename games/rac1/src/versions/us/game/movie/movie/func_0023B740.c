/* No recovered name. Playback loop: feeds file data (*file bytes) through the
 * read buffer at `rb` into the video decoder `vd`, starts audio and display once
 * the output buffer fills, aborts on a pad combination, then drains the decoder.
 * movie_play_step records the step reached. Returns 1 if aborted. */
int read_mpeg(char *vd, char *rb, int *file) {
    int aborted = 0;
    int started = 0;
    int left = *file;
    int toread;
    int space;
    int got;
    int avail;
    int used;
    int put;
    int get;

    flush_cache(0);
    toread = left;
    flush_cache(2);
    sce_gs_sync_v(0);

    while (left >= 5 && videoDecGetState(vd) != 3) {
        int hit;

        UpdatePad();
        if (decode_mode != -1) {
            if (decode_mode == 2 && g_controller_state.pressed != 0) {
                hit = 1;
            } else if ((challenge_mode || D_0015EF20 || decode_mode || g_current_level_index <= 0) &&
                       (g_controller_state.pressed & PAD_START)) {
                hit = 1;
            } else if ((g_controller_state.held & (PAD_L2 | PAD_R2 | PAD_L1 | PAD_R1)) == 0xF &&
                       (g_controller_state.pressed & PAD_START)) {
                hit = 1;
            } else {
                hit = 0;
            }
            if (hit) {
                aborted = 1;
                videoDecAbort(movie_gp_base + 0xD9048);
            }
        }
        space = readBufBeginPut(rb, &put);
        if (toread > 0 && space > 0xFFFF) {
            got = read_cd_stream_sectors(file, put, 0x10000, 0);
            readBufEndPut(rb, got);
            toread -= got;
        }
        proceedAudio();
        switchThread();
        avail = readBufBeginGet(rb, &get);
        if (avail > 0) {
            int size = *(int *)(rb + 0x50008);

            used = sce_mpeg_demux_pss_ring_buffer(vd, get, avail, rb, size);
            readBufEndGet(rb, used);
            left -= used;
        }
        proceedAudio();
        if (!started && voBufIsFull(decoder_state + 0xD9168) && isAudioOK()) {
            started = 1;
            startDisplay(1);
            audioDecStart(decoder_state + 0xD9100);
        }
    }

    while (!videoDecFlush(vd)) {
        proceedAudio();
        switchThread();
    }
    while (!videoDecIsFlushed(vd) && videoDecGetState(vd) != 3) {
        proceedAudio();
        switchThread();
    }
    endDisplay();
    audioDecReset(decoder_state + 0xD9100);
    snd_SetMasterVolume(5, voice_pool.f5C);
    snd_FlushSoundCommands();
    return aborted;
}

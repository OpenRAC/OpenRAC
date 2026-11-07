void request_music_track(int track, int stinger) {
    char *b = cd_read_state;
    if (*(short *)(b + 0x38) == track) {
        if (*(signed char *)(b + 0x22) == -1)
            return;
    }
    if (*(short *)(b + 0x3E))
        goto pending;
    if (*(short *)(b + 0x76))
        goto pending;
    *(short *)(b + 0x38) = track;
    return;
pending:
    cd_read_state[0x22] = track;
    cd_read_state[0x23] = stinger;
}

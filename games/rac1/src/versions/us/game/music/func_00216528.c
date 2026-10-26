extern char D_0011C478[];
void music_start_track_30000(int arg0, int arg1, int arg2) {
    int *p = (int *)(D_0011C478 + (arg0 * 4 + g_game_language * 0x258));
    char *s;

    if (*p == 0) {
        return;
    }
    s = (char *)cd_read_state;
    if (*(int *)(s + 0x50) != 0) {
        return;
    }
    *(unsigned int *)(s + 0x50) = 0xFFFFFFFF;
    *(short *)(s + 0x5A) = 1;
    *(short *)(s + 0x54) = arg0;
    *(short *)(s + 0x58) = arg1;
    *(int *)(s + 0x64) = 10;
    *(int *)(s + 0x68) = 0xBB80;
    *(short *)(s + 0x56) = arg2;
    *(short *)(s + 0x60) = 0;
    snd_PlayVAGStreamByLocEx_CB(*p, 0, 0, 0, *(unsigned char *)&gHelpDeskVoice_w ? arg2 : 0,
                  0, 2, 0, 0x21, music_secondary_start_callback, (long)(unsigned int)music_player_dialogue);
}

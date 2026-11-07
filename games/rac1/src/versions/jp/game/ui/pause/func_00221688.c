int advance_audio_stream_state(MenuScreen *arg0) {
    switch (arg0->data.stream.state) {
        case 0:
            if (arg0->data.stream.buffer[0] != 0 && cd_read_state[4] == 0) {
                if (start_audio_stream_read(arg0->data.stream.buffer[0], disc_table_t.unk2C8[5].sector,
                                            disc_table_t.unk2C8[5].size) != 0) {
                    arg0->data.stream.state += 1;
                } else {
                    arg0->data.stream.state = -1;
                }
            }
            break;
        case 1:
            if (cd_read_state[4] == 0) {
                arg0->data.stream.state = 2;
            }
            break;
        case 2:
            if (arg0->data.stream.buffer[1] != 0 && cd_read_state[4] == 0) {
                if (start_audio_stream_read(arg0->data.stream.buffer[1], disc_table_t.unk2F8[5].sector,
                                            disc_table_t.unk2F8[5].size) != 0) {
                    arg0->data.stream.state += 1;
                } else {
                    arg0->data.stream.state = -1;
                }
            }
            break;
        case 3:
            if (cd_read_state[4] == 0) {
                arg0->data.stream.state = 4;
            }
            break;
    }
    return 0;
}

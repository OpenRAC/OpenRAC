extern char draw_registration_value_target[];
void moby_draw_registration_update(unsigned char *moby) {
    switch (moby[0x20]) {
        case 0:
            moby[0x20] = 1;
            moby[0x30] = 0xFF;
            prepare_scaled_float_table_with_target_byte_v(draw_registration_value_target, 0x80, 1.0f);
            break;
        case 1:
            AddDrawCallback(register_audio_stream_callback, moby);
            break;
    }
}

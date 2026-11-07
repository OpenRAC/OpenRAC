s32 update_streamed_moby_animation(MobyAnimationStream *stream) {
    extern s16 cd_read_active[];
    extern int mark_menu_stream_buffer_read_active(int);
    extern char *moby_class_resources[];
    StreamedAnimationMoby *moby;
    AnimationTableHeader *header;
    StreamedClassResource **class_resource;
    StreamedClassResource **classes;
    s32 table_index;
    s32 k;
    u8 *animation_table;
    s32 animation_index;
    u16 class_slot;

    switch (stream->state) {
        case 0:
            if (cd_read_active[0] != 0) {
                break;
            }
            table_index = 0x4F000 - (disc_table[0x1614 / 4] << 11);
            if (start_audio_stream_read((int)(stream->buffer + table_index), disc_table[0x1610 / 4],
                                        disc_table[0x1614 / 4]) != 0) {
                stream->read_offset = table_index;
                stream->state = 1;
                animation_asset_read_active[0] = 1;
                mark_menu_stream_buffer_read_active((s32)stream->buffer);
                return 0;
            }
            stream->state = 3;
            break;
        case 1:
            /* Decompress the completed read and relocate three animation tables. */
            if (cd_read_active[0] != 0) {
                break;
            }
            animation_asset_read_active[0] = 0;
            fast_decompress((int)(stream->buffer + stream->read_offset), (int)stream->buffer);
            class_slot = resident_class_slot_by_id[0x7A5];
            classes = (StreamedClassResource **)moby_class_resources;
            header = (AnimationTableHeader *)stream->buffer;
            class_resource = &classes[class_slot];
            for (k = 0; k < 3; k++) {
                animation_table = stream->buffer + ((AnimationTableHeader *)((u8 *)header + (k << 3)))->offset;
                (*class_resource)->animation_tables[k + 1] = animation_table;
                relocate_asset_entry_pointers((char *)*class_resource, k + 1);
            }
            stream->state = 2;
            if (D_0015EF20 == 0) {
                stream->moby->moby_state = 0;
            } else {
                stream->moby->moby_state = 4;
            }
            stream->moby->animation_frame_counter = 0;
            break;
        case 2:
            /* Even phases queue dialogue; odd phases wait for animation completion. */
            moby = stream->moby;
            switch (moby->moby_state) {
                case 0:
                case 2:
                case 4:
                    animation_index = moby->moby_state >> 1;
                    if (moby->animation_frame_counter == 0) {
                        queued_dialogue_id[0] = animation_index * 6 + 60005;
                    }
                    moby->animation_frame_counter++;
                    if (moby->animation_frame_counter > scale_game_frame_count(0x78)) {
                        moby->animation_frame_counter = scale_game_frame_count(0x78);
                    }
                    if (moby->animation_frame_counter < scale_game_frame_count(0x78)) {
                        return 0;
                    }
                    if (dialogue_playback_phase[0] != 3) {
                        return 0;
                    }
                    moby->animation_frame_counter = 0;
                    moby->moby_state++;
                    MobyBlendAnimation(moby, animation_index + 1, 0, scale_game_frame_count(0x18));
                    break;
                case 1:
                case 3:
                case 5:
                    if (moby->animation_frame_counter == 0 && moby->primary_animation == moby->secondary_animation) {
                        continue_audio_stream_if_ready();
                        moby->animation_frame_counter = 1;
                    }
                    if (moby->transition_flags & 2) {
                        if (moby->moby_state == 1) {
                            moby->moby_state = moby->moby_state + 1;
                        } else {
                            moby->moby_state = 6;
                        }
                        moby->animation_frame_counter = 0;
                        MobyBlendAnimation(moby, 0, 0, scale_game_frame_count(0x18));
                        break;
                    }
                    break;
                case 6:
                    moby->animation_frame_counter++;
                    if (moby->animation_frame_counter > scale_game_frame_count(0xF0)) {
                        moby->animation_frame_counter = scale_game_frame_count(0xF0);
                    }
                    if (moby->animation_frame_counter < scale_game_frame_count(0xF0)) {
                        return 0;
                    }
                    if (D_0015EF20 == 0) {
                        moby->moby_state = 0;
                    } else {
                        moby->moby_state = 4;
                    }
                    moby->animation_frame_counter = 0;
                    break;
            }
            break;
    }
    return 0;
}

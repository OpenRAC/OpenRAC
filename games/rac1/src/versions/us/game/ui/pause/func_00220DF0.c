s32 update_menu_resource_stream(MenuScreen *stream) {
    LanguageResourceOffsets language_offsets;
    s32 *completed_buffer_slot;
    s32 primary_entry_offset;
    s32 secondary_entry_offset;
    s32 replacement_entry_offset;
    s32 stream_flags;
    s32 slot_index;
    s32 primary_read_offset;
    s32 secondary_read_offset;
    s32 replacement_read_offset;
    s32 stream_state;
    s32 primary_buffer;
    s32 completed_buffer;
    s32 secondary_buffer;
    s32 replacement_buffer;
    s32 selection_flags;
    s32 resource_index;
    s32 primary_read_address;
    s32 secondary_read_address;
    s32 replacement_read_address;
    s32 primary_read_started;
    s32 secondary_read_started;
    s32 replacement_read_started;
    MenuStreamEntry *primary_entry;
    MenuStreamEntry *primary_entry_alternate;
    MenuStreamEntry *secondary_entry;
    MenuStreamEntry *secondary_entry_alternate;
    MenuStreamEntry *replacement_entry;
    MenuStreamEntry *replacement_entry_alternate;
    MenuScreen *item_selection;
    MenuScreen *group_selection;
    u8 *item_table_entry;

    selection_flags = stream->data.stream.flags;
    stream->data.stream.elapsed_frames = (s32)(stream->data.stream.elapsed_frames + 1);
    if (selection_flags & 1) {
        resource_index = stream->data.stream.fixed_entry;
        if (resource_index == -1) {
            return 0;
        }
        goto process_stream_state;
    } else {
        if (selection_flags & 2) {
            resource_index = selected_level_index_r[0];
        } else if (selection_flags & 4) {
            resource_index = D_001D5F70_20DF0.current->focus->data.grid.selected_cell;
        } else if (selection_flags & 0x100) {
            resource_index = D_001D5F70_20DF0.current->focus->data.save.slot;
            if (resource_index < 0) {
                resource_index = 0;
            }
            if (resource_index >= 5) {
                resource_index = 4;
            }
            if (memory_card_state_r3.busy < 3) {
                if (memory_card_state_r3.state < 0) {
                    if (stream->data.stream.state == -1) {
                        stream->data.stream.state = 0;
                    }
                    resource_index = memory_card_state_r3.card[0].slots[resource_index].id;
                    if (resource_index == -1) {
                        stream->data.stream.state = resource_index;
                    }
                    if (memory_card_state_r3.card[0].type != 2) {
                        stream->data.stream.state = -1;
                    }
                } else {
                    goto disable_stream;
                }
            } else {
                goto disable_stream;
            }
        } else if (selection_flags & 8) {
            group_selection = D_001D5F70_20DF0.current->focus;
            resource_index = (s32) * (s16 *)((u8 *)((group_selection->data.grid.selected_cell * 0xA) +
                                                    (s32)group_selection->data.grid.cells) +
                                             0x8);
            if (group_selection->data.grid.rows == 0) {
            disable_stream:
                stream->data.stream.state = -1;
            }
        } else if (selection_flags & 0x400) {
            resource_index = (s32)((s32)stream->data.stream.elapsed_frames / scale_game_frame_count_r2(0x12C)) % 19;
        } else if (selection_flags & 0x1000) {
            language_offsets = language_resource_offsets;
            resource_index = *stream->data.stream.language_base + language_offsets.values[g_game_language_r];
        } else {
            item_selection = D_001D5F70_20DF0.current->focus;
            resource_index = item_selection->data.list.selected;
            if (resource_index < 0) {
                resource_index = 0;
            }
            if (selection_flags & 0x4000) {
                item_table_entry = (u8 *)item_selection->data.list.items;
                item_table_entry += resource_index * 0xC;
                resource_index = (*(s16 *)(item_table_entry + 2) == 2) ? 9 : resource_index;
            }
        }
    process_stream_state:
        if (stream->data.stream.flags & 0x2000) {
            resource_index = (gSkillPoints_r[resource_index] == 0) ? 0x1E : resource_index;
        }
        stream_state = stream->data.stream.state;
        switch (stream_state) {
            case 0:
                primary_buffer = stream->data.stream.buffer[0];
                if (primary_buffer != 0 && cd_read_active_r[0] == 0) {
                    primary_entry_offset = resource_index * 8;
                    if (*(s32 *)((u8 *)(primary_entry_offset + (s32)stream->data.stream.entries) + 0x4) != 0) {
                        primary_read_address = primary_buffer;
                        if (stream->data.stream.flags & 0x20) {
                            primary_read_offset =
                                get_menu_stream_buffer_size_r2(primary_buffer) -
                                (*(s32 *)((u8 *)(primary_entry_offset + (s32)stream->data.stream.entries) + 0x4)
                                 << 0xB);
                            stream->data.stream.read_offset = primary_read_offset;
                            primary_read_address += primary_read_offset;
                        }
                        if (stream->data.stream.flags & 0x10) {
                            primary_entry =
                                (MenuStreamEntry *)(primary_entry_offset + (s32)stream->data.stream.entries);
                            primary_read_started = start_audio_stream_read_r3(
                                primary_read_address, primary_entry->sector, primary_entry->sector_count);
                        } else {
                            primary_entry_alternate =
                                (MenuStreamEntry *)(primary_entry_offset + (s32)stream->data.stream.entries);
                            primary_read_started =
                                start_audio_stream_read_r4(primary_read_address, primary_entry_alternate->sector,
                                                           primary_entry_alternate->sector_count);
                        }
                        if (primary_read_started != 0) {
                            mark_menu_stream_buffer_read_active_r2(stream->data.stream.buffer[0]);
                            stream->data.stream.loaded_entry[0] = resource_index;
                            stream->data.stream.state = stream->data.stream.state + 1;
                        } else {
                            stream->data.stream.state = -1;
                        }
                    }
                }
                break;
            case 1:
            case 3:
            case 5:
                if (cd_read_active_r[0] == 0) {
                    stream_flags = stream->data.stream.flags;
                    if (stream_flags & 0x20) {
                        completed_buffer_slot = &stream->data.stream.buffer[0];
                        slot_index = stream->data.stream.state == 3;
                        completed_buffer_slot += slot_index;
                        clear_menu_stream_buffer_read_active_r2(*completed_buffer_slot);
                        completed_buffer = *completed_buffer_slot;
                        fast_decompress_r3(completed_buffer + stream->data.stream.read_offset, completed_buffer);
                        stream->data.stream.read_offset = 0;
                    }
                    stream->data.stream.state = stream->data.stream.state + 1;
                }
                break;
            case 6:
                stream->data.stream.state = 2;
                /* fallthrough */
            case 2:
                if (resource_index != stream->data.stream.loaded_entry[0]) {
                    if (resource_index == stream->data.stream.loaded_entry[1]) {
                        stream->data.stream.state = 4;
                    } else {
                        secondary_buffer = stream->data.stream.buffer[1];
                        if (secondary_buffer == 0) {
                            stream->data.stream.state = 0;
                        } else if (cd_read_active_r[0] == 0) {
                            secondary_entry_offset = resource_index * 8;
                            if (*(s32 *)((u8 *)(secondary_entry_offset + (s32)stream->data.stream.entries) + 0x4) !=
                                0) {
                                secondary_read_address = secondary_buffer;
                                if (stream->data.stream.flags & 0x20) {
                                    secondary_read_offset =
                                        get_menu_stream_buffer_size_r2(secondary_buffer) -
                                        (*(s32 *)((u8 *)(secondary_entry_offset + (s32)stream->data.stream.entries) +
                                                  0x4)
                                         << 0xB);
                                    stream->data.stream.read_offset = secondary_read_offset;
                                    secondary_read_address += secondary_read_offset;
                                }
                                if (stream->data.stream.flags & 0x10) {
                                    secondary_entry =
                                        (MenuStreamEntry *)(secondary_entry_offset + (s32)stream->data.stream.entries);
                                    secondary_read_started = start_audio_stream_read_r3(
                                        secondary_read_address, secondary_entry->sector, secondary_entry->sector_count);
                                } else {
                                    secondary_entry_alternate =
                                        (MenuStreamEntry *)(secondary_entry_offset + (s32)stream->data.stream.entries);
                                    secondary_read_started = start_audio_stream_read_r4(
                                        secondary_read_address, secondary_entry_alternate->sector,
                                        secondary_entry_alternate->sector_count);
                                }
                                if (secondary_read_started != 0) {
                                    mark_menu_stream_buffer_read_active_r2(stream->data.stream.buffer[1]);
                                    stream->data.stream.loaded_entry[1] = resource_index;
                                    stream->data.stream.state = stream->data.stream.state + 1;
                                } else {
                                    stream->data.stream.state = -1;
                                }
                            }
                        }
                    }
                }
                break;
            case 4:
                if (resource_index != stream->data.stream.loaded_entry[1]) {
                    if (resource_index == stream->data.stream.loaded_entry[0]) {
                        stream->data.stream.state = 2;
                    } else {
                        replacement_buffer = stream->data.stream.buffer[0];
                        if (replacement_buffer == 0) {
                            stream->data.stream.state = -1;
                        } else if (cd_read_active_r[0] == 0) {
                            replacement_entry_offset = resource_index * 8;
                            if (*(s32 *)((u8 *)(replacement_entry_offset + (s32)stream->data.stream.entries) + 0x4) !=
                                0) {
                                replacement_read_address = replacement_buffer;
                                if (stream->data.stream.flags & 0x20) {
                                    replacement_read_offset =
                                        get_menu_stream_buffer_size_r2(replacement_buffer) -
                                        (*(s32 *)((u8 *)(replacement_entry_offset + (s32)stream->data.stream.entries) +
                                                  0x4)
                                         << 0xB);
                                    stream->data.stream.read_offset = replacement_read_offset;
                                    replacement_read_address += replacement_read_offset;
                                }
                                if (stream->data.stream.flags & 0x10) {
                                    replacement_entry = (MenuStreamEntry *)(replacement_entry_offset +
                                                                            (s32)stream->data.stream.entries);
                                    replacement_read_started =
                                        start_audio_stream_read_r3(replacement_read_address, replacement_entry->sector,
                                                                   replacement_entry->sector_count);
                                } else {
                                    replacement_entry_alternate = (MenuStreamEntry *)(replacement_entry_offset +
                                                                                      (s32)stream->data.stream.entries);
                                    replacement_read_started = start_audio_stream_read_r4(
                                        replacement_read_address, replacement_entry_alternate->sector,
                                        replacement_entry_alternate->sector_count);
                                }
                                if (replacement_read_started != 0) {
                                    mark_menu_stream_buffer_read_active_r2(stream->data.stream.buffer[0]);
                                    stream->data.stream.loaded_entry[0] = resource_index;
                                    stream->data.stream.state = stream->data.stream.state + 1;
                                } else {
                                    stream->data.stream.state = -1;
                                }
                            }
                        }
                    }
                }
                break;
            default:
                break;
        }
    }
    return 0;
}

s32 update_menu_stream_image(MenuScreen *stream) {
    s32 flags;
    s32 idx;
    s32 r;
    s32 x;
    s32 y;
    u8 *p;
    struct ClutImage *b;
    u8 *p20;
    u8 *p420;

    flags = stream->data.stream.flags;
    if (flags & 1) {
        idx = stream->data.stream.fixed_entry;
        if (idx == -1) {
            return 0;
        }
    } else if (flags & 2) {
        idx = selected_level_index_r[0];
    } else if (flags & 4) {
        idx = D_001D5F70_209A0.current->focus->data.grid.selected_cell;
    } else if (flags & 0x100) {
        idx = D_001D5F70_209A0.current->focus->data.save.slot;
        if (idx <= -1) {
            idx = 0;
        }
        if (idx >= 5) {
            idx = 4;
        }
        if (memory_card_state_r2.busy < 3 && memory_card_state_r2.state < 0) {
            if (stream->data.stream.state == -1) {
                stream->data.stream.state = 0;
            }
            idx = memory_card_state_r2.card[0].slots[idx].id;
        } else {
            stream->data.stream.state = -1;
        }
    } else {
        idx = D_001D5F70_209A0.current->focus->data.list.selected;
        if (idx <= -1) {
            idx = 0;
        }
    }

    switch (stream->data.stream.state) {
        case 0:
        case 2:
            if (idx == stream->data.stream.loaded_entry[0]) {
                break;
            }
            if (stream->data.stream.buffer[0] == 0) {
                break;
            }
            if (cd_read_active_r[0] != 0) {
                break;
            }
            if (stream->data.stream.entries[idx].sector_count == 0) {
                break;
            }
            p = (u8 *)stream->data.stream.buffer[0];
            if (stream->data.stream.flags & 0x20) {
                r = get_menu_stream_buffer_size_r(stream->data.stream.buffer[0]) -
                    (stream->data.stream.entries[idx].sector_count << 11);
                stream->data.stream.read_offset = r;
                p += r;
            }
            if (stream->data.stream.flags & 0x10) {
                r = start_audio_stream_read_r2(p, stream->data.stream.entries[idx].sector,
                                               stream->data.stream.entries[idx].sector_count);
            } else {
                r = start_audio_stream_read_r2(p, stream->data.stream.entries[idx].sector,
                                               stream->data.stream.entries[idx].sector_count);
            }
            if (r != 0) {
                mark_menu_stream_buffer_read_active_r(stream->data.stream.buffer[0]);
                stream->data.stream.loaded_entry[0] = idx;
                stream->data.stream.state++;
            } else {
                stream->data.stream.state = -1;
            }
            break;
        case 1:
        case 3:
            if (cd_read_active_r[0] != 0) {
                break;
            }
            clear_menu_stream_buffer_read_active_r(stream->data.stream.buffer[0]);
            if (stream->data.stream.flags & 0x20) {
                fast_decompress_r2((u8 *)stream->data.stream.buffer[0] + stream->data.stream.read_offset,
                                   stream->data.stream.buffer[0]);
                stream->data.stream.read_offset = 0;
            }
            b = (struct ClutImage *)stream->data.stream.buffer[0];
            p20 = b->clut;
            p420 = b->pixels;
            x = log2dim(b->width);
            y = log2dim(b->height);
            level_map_selection_r2.tex0 = register_8bit_texture(x, y, p20, p420, level_map_selection_r2.tex_clut_vram,
                                                                level_map_selection_r2.tex0_vram);
            flush_pending_texture_uploads();
            stream->data.stream.state = 2;
            break;
    }
    return 0;
}

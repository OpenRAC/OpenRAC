extern int D_E1_001D5C94;
s32 DrawMapScreen(MenuScreen *screen) {
    s32 prev;
    s32 idx;
    s32 t;
    s32 id;
    s32 slot;
    s32 n;
    struct MapHdr *hdr;
    u8 *pal;
    u8 *pix;
    s32 buf;
    u8 *base;
    u8 *a;
    u8 *tex;
    u8 *tbase;
    u8 *b;
    u8 *c;
    s32 pick;
    s32 next;
    s32 k;
    struct MapEntry *e;

    update_map_zoom_and_pan();
    if (!(screen->data.raw.unk34 & 0x40)) {
        prev = level_map_selection_r.level;
        if (g_controller_state_r.pressed_unmasked & 0xD00) {
            if (D_E1_001D5C94 == 0) {
                return 1;
            }
        }
        if (g_controller_state_r.pressed_unmasked & 0x10) {
            if (menu_system_r4.current->back != 0) {
                menu_system_r4.next = menu_system_r4.current->back;
            } else if (menu_system_r4.close_locked == 0) {
                return -1;
            }
        }
        if (g_controller_state_r.pressed_unmasked & 0x40) {
            menu_system_r4.next = (MenuPage *)mission_menu_page;
        }
        if ((g_controller_state_r.pressed_unmasked & 0x20) && level_map_selection_r.level != 0) {
            menu_system_r4.unkF0 = (MenuPage *)map_menu_page;
            menu_system_r4.unkF4 = 0xB;
            menu_system_r4.close_request = 3;
            menu_system_r4.unkE4 = level_map_selection_r.level;
            Moby_PlaySound_r3(0, 0x11, screen->moby);
            return 0;
        }
        idx = find_id_in_terminated_table(level_map_selection_r.level);
        if (idx >= 0) {
            if (g_controller_state_r.pressed_unmasked & 8) {
                if (idx < 0x13) {
                    t = menu_level_indices_r[idx + 1];
                    if (t != 0) {
                        level_map_selection_r.level = t;
                    }
                }
            }
            if ((g_controller_state_r.pressed_unmasked & 4) && idx != 0) {
                if (menu_level_indices_r[idx - 1] != 0) {
                    level_map_selection_r.level = menu_level_indices_r[idx - 1];
                }
            }
        }
        if (level_map_selection_r.level != prev) {
            Moby_PlaySound_r3(1, 0x11, screen->moby);
            update_mission_list_r();
        }
    }

    id = level_map_selection_r.level;
    if (gHaveMapOMatic[0] != 0) {
        id += 0x100;
    }
    slot = find_map_entry_slot(id);
    if (level_map_selection_r.level != level_map_selection_r.loaded && slot != -1) {
        n = pick_map_slot_to_evict();
        if (level_map_selection_r.slot[0] != 0 && n > 0) {
            move_map_entry_slot(n, 0);
            if (slot == 0) {
                slot = n;
            }
        }
        fast_decompress_r(level_map_selection_r.slot[slot], level_map_selection_r.hdr);
        hdr = level_map_selection_r.hdr;
        pix = (u8 *)hdr + hdr->unk8;
        pal = (u8 *)hdr + hdr->unkC;
        buf = acquire_menu_stream_buffer_r(0);
        if (buf != 0) {
            base = (u8 *)level_map_selection_r.hdr;
            a = base + hdr->unk0 + 8;
            if (level_map_selection_r.level == g_current_level_index_r) {
                compose_bitmap_from_mask(pal, pix, pal, level_map_selection_r.mask);
            } else {
                if (level_visit_state[level_map_selection_r.level] != 0) {
                    load_map_chunk(buf, level_map_coverage_r + (level_map_selection_r.level << 11), base + hdr->unk4);
                } else {
                    decode_map_mask(buf, a, a, base);
                }
                compose_bitmap_from_mask(pal, pix, pal, buf);
            }
        }
        if (!(screen->data.raw.unk34 & 0x80)) {
            tbase = (u8 *)level_map_selection_r.hdr;
            tex = tbase + hdr->unk10;
            b = tbase + hdr->unk14 + 0x420;
            c = tbase + hdr->unk18 + 0x420;
            level_map_selection_r.tex0 = register_8bit_texture_r(
                7, 7, tex + 0x20, tex + 0x420, level_map_selection_r.tex_clut_vram, level_map_selection_r.tex0_vram);
            level_map_selection_r.tex1 = register_8bit_texture_r(
                7, 7, tex + 0x20, b, level_map_selection_r.tex_clut_vram, level_map_selection_r.tex1_vram);
            level_map_selection_r.tex2 = register_8bit_texture_r(
                7, 7, tex + 0x20, c, level_map_selection_r.tex_clut_vram, level_map_selection_r.tex2_vram);
            if (buf != 0) {
                register_8bit_texture_r(9, 9, pal, pal, 0x3FF000, level_map_selection_r.map_image_vram);
            }
            flush_pending_texture_uploads_r();
        }
        if (buf != 0) {
            release_menu_stream_buffer_r(buf);
        }
        level_map_selection_r.slot[0] = (s32)pix;
        level_map_selection_r.loaded = level_map_selection_r.level;
        update_map_icons(level_map_selection_r.level, 0);
    }

    if (cd_read_state_r.unk8 == 0) {
        if (level_map_selection_r.sel != -1) {
            menu_system_r4.pending_buffer = 0;
            level_map_selection_r.slot_id[level_map_selection_r.sel] ^= 0x1000;
            level_map_selection_r.sel = -1;
        }
    }
    pick = -1;
    if (cd_read_state_r.unk8 == 0) {
        pick = promote_first_available_map_entry();
        if (pick != -1) {
            next = pick_next_map();
            if (next != -1) {
                k = (g_current_level_index_r + level_map_selection_r.unk230 == 0) ? 0 : 0x100;
                if (k == next) {
                    stash_receive_data_r(level_map_selection_r.slot[pick], level_map_selection_r.unk22C, 0,
                                         level_map_selection_r.unk234, 0);
                    level_map_selection_r.slot_size[pick] = level_map_selection_r.unk234;
                } else {
                    if (next & 0x100) {
                        e = &map_screen_disc_entries_alt[next ^ 0x100];
                    } else {
                        e = &map_screen_disc_entries[next];
                    }
                    start_audio_stream_read_r(level_map_selection_r.slot[pick], e->a, e->b);
                    menu_system_r4.pending_buffer = 1;
                    level_map_selection_r.slot_size[pick] = (e->b << 11) >> 4;
                }
                level_map_selection_r.slot_id[pick] = next | 0x1000;
                level_map_selection_r.sel = pick;
            }
        }
    }
    slot = find_map_entry_slot(id);
    if (level_map_selection_r.level != level_map_selection_r.loaded) {
        if (cd_read_state_r.unk8 == 0 && slot == -1 && pick == slot) {
            pick_map_slot_to_evict(level_map_selection_r.loaded);
        }
    }
    return 0;
}

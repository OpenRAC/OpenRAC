extern int func_001160D8(void);
extern SoundCue_BB48 D_E1_00186230[];
void transition_do_transition(void) {
    WadHeader_BB48 *hdr;
    u8 *p;
    s32 n;
    s32 level;
    s32 wait;
    s32 i;
    s32 old;
    s32 first_size;
    u32 alignment_mask = 0xFFFFFFF0;
    DiscTable *tbl;

    g_current_level_index = 0;
    *(volatile u32 *)0x10000010 = 0x83;
    *(volatile u32 *)0x10000000 = 0;
    load_and_initialize_level_chunk();
    transition_load_wad_BB48();
    tbl = &disc_table_BB48;
    p = *(u8 **)vu1_chain_buffer_2;
    hdr = (WadHeader_BB48 *)p;
    p = (u8 *)hdr + 0x60;
    level_root = hdr;
    first_size = Load(p, ((DiscFile_BB48 *)((DiscFile_BB48 *)&tbl->wad_chunks[1]))->sector,
                      ((DiscFile_BB48 *)((DiscFile_BB48 *)&tbl->wad_chunks[1]))->size);
    hdr->chunk[1].size = first_size;
    hdr->chunk[1].off = p - (u8 *)hdr;
    p += ((u32)first_size + 0xF) & alignment_mask;
    hdr->chunk[2].size = Load(p, tbl->wad_chunks[2].sector, tbl->wad_chunks[2].size);
    hdr->chunk[2].off = p - (u8 *)hdr;
    p += ((u32)hdr->chunk[2].size + 0xF) & alignment_mask;
    hdr->chunk[3].size = Load(p, tbl->wad_chunks[3].sector, tbl->wad_chunks[3].size);
    hdr->chunk[3].off = p - (u8 *)hdr;
    p += ((u32)hdr->chunk[3].size + 0xF) & alignment_mask;
    hdr->chunk[4].size = Load(p, tbl->wad_chunks[4].sector, tbl->wad_chunks[4].size);
    hdr->chunk[4].off = p - (u8 *)hdr;
    p += ((u32)hdr->chunk[4].size + 0xF) & alignment_mask;
    hdr->chunk[5].size = Load(p, tbl->wad_chunks[5].sector, tbl->wad_chunks[5].size);
    hdr->chunk[5].off = p - (u8 *)hdr;
    p += ((u32)hdr->chunk[5].size + 0xF) & alignment_mask;
    hdr->chunk[0].size = Load(p, tbl->wad_chunks[0].sector, tbl->wad_chunks[0].size);
    hdr->chunk[0].off = p - (u8 *)hdr;
    Hud_HeapReset();
    initialize_sif_rpc();
    init_stash();
    init_hud();
    LoadHudBanks();
    for (n = *(volatile u32 *)0x10000000 / 0x109; n < scale_game_frame_count(0xB4); n++) {
        sce_gs_sync_v(0);
    }
    snd_reset_BB48();
    while (snd_FlushSoundCommands() != 0) {
    }
    wait = 0;
    level = func_001160D8() % 4;
    snd_reenable_command_flush();
    while (space_load_pending == 0) {
        VU1_sendChain();
        VU1_swapChain();
        PutDrawBufferSmall();
        append_palette_transfer_packet();
        PutDrawBufferLarge();
        memcard_update_state();
        Menu_RunStateHandler();
        UpdatePad();
        queue_dma_BB48(dma_index_BB48);
        space_load_in_progress = 0;
        for (i = 0; D_E1_00186230[i].start != -1; i++) {
            if (D_E1_00186230[i].end == -1) {
                if (transfer_BB48.time < D_E1_00186230[i].start) {
                    D_E1_00186230[i].voice = -1;
                    continue;
                }
                if (D_E1_00186230[i].voice != -1) {
                    continue;
                }
            } else if (transfer_BB48.time < D_E1_00186230[i].start || D_E1_00186230[i].end < transfer_BB48.time) {
                goto stop;
            } else if (is_active_BB48(D_E1_00186230[i].voice, D_E1_00186230[i].sound) != 0) {
                continue;
            }
            D_E1_00186230[i].voice = allocate_voice_for_group_entry(D_E1_00186230[i].sound, 0, 0);
            continue;
        stop:
            if (is_active_BB48(D_E1_00186230[i].voice, D_E1_00186230[i].sound) != 0) {
                release_voice_slot(D_E1_00186230[i].voice);
            }
            D_E1_00186230[i].voice = -1;
        }
        update_gameplay_BB48();
        default_draw_BB48();
        if (game_mode_BB48 != 0) {
            wait = 0;
        } else if (++wait >= scale_game_frame_count(0x5DC)) {
            start_level(level);
            wait = 0;
            level = (level + 1) % 4;
            sequence_BB48 = 1.0f;
            transfer_BB48.time = 0;
            transfer_BB48.f3C = 0;
            parse_space_scene_chunk(0);
            intro_alpha_BB48 = 0;
            language_alpha_BB48 = 0;
            intro_timer_BB48 = 0;
            VU1_initChain();
            continue;
        }
        if (requested_video_mode_BB48 != pal_mode_BB48) {
            pal_mode_BB48 = pal_mode_BB48 == 0;
            FadeToBlack(4);
            reset_vif1_gif_BB48();
            reset_graph_BB48(0, 1, pal_mode_BB48 != 0 ? 3 : 2, 0);
            old = tex_start_BB48;
            sequence_BB48 = 1.0f;
            SetPalMode();
            tex_start_BB48 = old;
        }
        VU1_syncChain(1);
        sce_gs_sync_v(0);
        game_frame_counter++;
    }
    if (pal_mode_BB48 == 0) {
        tex_base_BB48 = 0x280000;
    }
    tex_cursor_BB48 = tex_start_BB48 = tex_base_BB48;
}

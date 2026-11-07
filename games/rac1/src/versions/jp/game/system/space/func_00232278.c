void initialize_level_runtime(void) {
    s64 texture_packet[4];
    LevelHeader *header;
    s32 data_base;
    s32 class_data_base;
    ClassEntry *class_entries;
    ClassEntry *class_entry;
    s32 *output;
    MobyEntry *moby_entry;
    s32 *texture_offsets;
    s32 language_index;
    s32 archive_size;
    u8 *allocation_cursor;
    s32 *scene_offsets;
    s32 *scene_offset;
    Chunk *scene_chunk;
    s32 i;
    s32 j;
    s32 k;
    s32 texture_cursor;
    s32 scene_variant;
    s32 workspace_offset;

    level_render_state_r.state = 4;
    level_render_state_r.timer = -1;
    sequence = 1.0f;
    D_001413D0[0] = 0;
    current_vu_chain_r = 0x100000;
    space_load_in_progress_r = 0;
    D_0015F440 = 0;
    init_mem_slots();
    gs_texture_allocation_cursor = gs_texture_allocation_base;
    gs_texture_allocation_start = gs_texture_allocation_base;
    fill_transfer_words(D_00194180, 0x87654321, 0x10);
    fill_transfer_words(resident_class_slot_by_id_r, -1, 0x800);
    fill_transfer_words(resident_class_material_maps, -1, 0xE00);
    fill_transfer_words(D_001B6180, 0, 0xE0);
    init_view_context();
    update_view_context();
    vu1_init_chain();
    submit_audio_stream_io_request(D_001941C0_q.buffer + 0x400000, disc_table_r.unk13B8.sector,
                                   disc_table_r.unk13B8.size);
    fade_to_black(scale_game_frames(0xC));
    sceCdSync(0);
    FlushCache(0);
    archive_size = func_0020B618(D_001941C0_q.buffer + 0x400000, D_001941C0_q.buffer);
    FlushCache(0);
    header = (LevelHeader *)D_001941C0_q.buffer;
    upload_texture_images((s32)header + header->tex, header->texCount, (s32)header + header->texInfo);
    data_base = (s32)header + header->data_offset;
    class_data_base = data_base + header->unk30;
    special_material_template[0] =
        (s32)((gs_texture_allocation_base + header->unk40) >> 8) | 0x1D308000 | ((s64)0xB800 << 19) |
        ((s64)((gs_texture_allocation_base + header->unk44) >> 8) << 37) | 0x8000000000000000LL;
    special_material_template[1] = 0x0000FFA0000000E0LL;
    special_material_template[2] = 0x0040000400004000LL;
    class_entries = (ClassEntry *)((s32)header + header->classes);
    D_0015FF08 = header->classCount;
    for (i = 0; i < D_0015FF08; i++) {
        D_001B5980[i] = class_data_base + class_entries[i].offset + (highest_set_bit_index(class_entries[i].id) << 28);
    }
    copy_blocks_16_forward(resident_indexed_textures, class_entries, D_0015FF08 * 16);
    relocate_sky_definition(data_base + header->sky);
    moby_entry = (MobyEntry *)((s32)header + header->mobys);
    D_0015FF00 = 0;
    for (j = 0; j < header->mobyCount; j++) {
        register_moby_class(moby_entry->offset == 0 ? 0 : data_base + moby_entry->offset, (s32)header + header->classes,
                            (s32)moby_entry->body, moby_entry->oclass);
        moby_entry++;
    }
    level_texture_payload_address = data_base + header->unk38;
    unpack_point_records((s32)header + header->points, header->pointCount);
    {
        s32 particle_data_base = data_base + header->unk34;
        s32 particle_entries = (s32)header + header->parts;

        parse_particle_textures((s32)header + header->unk3C, particle_data_base, particle_entries, header->partCount);
    }
    texture_offsets = (s32 *)(data_base + header->unk4C);
    language_index = 5;
    upload_mip_texture((s32)texture_offsets + texture_offsets[current_level_index + 1], texture_packet);
    D_00160580 = texture_packet[0];
    upload_mip_texture((s32)texture_offsets + texture_offsets[language_index * 19 + current_level_index + 0x14],
                       texture_packet);
    texture_cursor = gs_texture_allocation_cursor + 0x2000;
    allocation_cursor = D_001941C0_q.buffer + archive_size;
    gs_texture_allocation_cursor = texture_cursor;
    D_001941C0_q.unk18 = (s32)allocation_cursor;
    gs_texture_allocation_start = texture_cursor;
    cutscene_moby_ids_by_content_variant = texture_packet[0];
    clear_blocks_16(point_lights, 0x100);
    clear_blocks_16(point_light_data, 0x180);
    clear_blocks_16(dir_lights, 0x400);
    copy_blocks_16_forward(dir_lights, D_001D9740, 0x40);
    D_0015FF18 = allocation_cursor;
    fill_transfer_words(allocation_cursor, 0, 0x4000);
    allocation_cursor += 0x4000;
    moby_instance_end_r = D_0015FF18;
    D_0015FF18[0x20] = 0xFF;
    moby_vars = allocation_cursor;
    allocation_cursor += 0x2000;
    D_001600AC = allocation_cursor;
    D_001600B4 = -1;
    moby_instance_perm_end = D_0015FF18 + 0x3F00;
    D_001600B0 = 0;
    D_001600B8 = 0;
    clear_blocks_16(D_001CD780, 0x200);
    reset_draw_globals();
    occl_fog_count = 0x1F4;
    part_clip_dist = 0x1F4000;
    scene_variant = (rand() >> 16) & 3;
    level_render_state_r.scene_state = 0;
    level_render_state_r.scene_mode = scene_variant;
    level_render_state_r.history_index = 0;
    level_render_state_r.history_count = 0;
    qcopy(&shrub_tex_dma_tag, &shrub_colors);
    if (current_level_index == 0 || (current_level_index == 1 && level_available_r[3] == 0)) {
        level_render_state_r.scene_mode = 4;
        level_render_state_r.scene_state = 2;
    }
    fill_transfer_words(&D_0018CB20, 0, 0x1C0);
    fill_transfer_words(D_00186310, 0, 0x40);
    fill_transfer_words(D_00186350, 0, 0x40);
    scene_offsets = header->scenes;
    workspace_offset = current_vu_chain - 0x60000;
    scene_offset = &scene_offsets[level_render_state_r.scene_mode];
    D_0018CB20.unk58 = D_001941C0_q.unk4 + workspace_offset;
    D_0018CB20.unk5C = D_001941C0_q.unk8 + workspace_offset;
    current_vu_chain = workspace_offset;
    scene_chunk = (Chunk *)(data_base + header->scenes[level_render_state_r.scene_mode]);
    /* The archive list is terminated by a zero size and the resident table holds 70 entries. */
    for (k = 0; k < 70 && scene_chunk->size != 0; k++) {
        {
            s32 chunk_offset = scene_chunk->offset + 0x800;
            D_0018CB20.chunks[k] = data_base + *scene_offset + chunk_offset;
        }
        scene_chunk++;
    }
    parse_space_scene_chunk_r(0);
    sound_bank_handle_r = 0xFFFFFFFF;
    snd_bank_load_from_ee_cb(data_base + header->sound, store_async_sound_bank_handle, (u32)&sound_bank_handle_r);
    do {
        FlushCache(0);
        sceGsSyncV(0);
        snd_flush_sound_commands();
    } while (sound_bank_handle_r == 0xFFFFFFFF);
    snd_resolve_bank_xrefs();
}

extern s32 D_J_0015F058 MACRO_ADDR;
extern s64 D_J_0015F048 SDATA(D_J_0015F048);
void transition_load_wad(void) {
    u8 li[0x60];
    WadHeader *hdr;
    u8 *data;
    u8 *base;
    s32 size;
    s32 i;
    s32 j;
    s32 k;
    s32 cnt;
    s32 v;
    s32 *out;
    int new_var;
    WadTex *tex;
    s32 new_var4;
    WadClass20_EABE8 *c20;
    u8 *new_var3;
    WadClass30_EABE8 *c30;
    WadSound *snd;
    TextEntry *te;
    WadTex *new_var2;
    s32 k_800 = 0x800;
    s32 adj;
    s64 t;
    s64 u;
    D_J_0015F058 = 0;
    i = 0;
    select_level_vu_chain();
    InitMemSlots();
    current_vu_chain = 0x100000;
    g_gs_texture_allocation_base_r = 0x2C0000;
    g_gs_texture_allocation_start_r = 0x2C0000;
    g_gs_texture_allocation_cursor_r = 0x2C0000;
    FastMemSet(coll_output_aux_buffer_r, 0x87654321, 0x10);
    FastMemSet(resident_class_slot_by_id, -1, k_800);
    FastMemSet(resident_class_material_maps, -1, 0xE00);
    FastMemSet(resident_class_original_field_2c_r, 0, 0xE0);
    InitViewContext();
    UpdateViewContext();
    VU1_initChain();
    flush_cache(0);
    size = fast_decompress((u8 *)0x1400000, level_resource_buffers.hdr);
    flush_cache(0);
    hdr = (WadHeader *)level_resource_buffers.hdr;
    upload_texture_images(((u8 *)hdr) + hdr->x0, hdr->x8, ((u8 *)hdr) + hdr->xC);
    t = ((s64)((g_gs_texture_allocation_base_r + hdr->x70) >> 8)) | 0x1D308000;
    u = (((s64)((g_gs_texture_allocation_base_r + hdr->x74) >> 8)) << 37) | (((s64)0xB800) << 19);
    data = ((u8 *)hdr) + hdr->data;
    base = data + hdr->x60;
    special_material_template[0] = (t | u) | (((s64)(-1)) << 63);
    special_material_template[1] = 0xFFA0000000E0;
    special_material_template[2] = 0x0040000400004000;
    tex = (WadTex *)(((u8 *)hdr) + hdr->x34);
    if ((tfrag_texture_definition_count = hdr->n30) > 0) {
        do {
            tfrag_texture_size_table[i] = (((s32)base) + tex[i].offset) + (log2dim(tex[i].x4) << 28);
            i++;
        } while (i < tfrag_texture_definition_count);
    }
    k = 0;
    tex = (new_var2 = (WadTex *)(((u8 *)hdr) + hdr->x3C));
    if ((resident_indexed_texture_count = hdr->n38) > 0) {
        do {
            moby_texture_lookup_table[k] = (((s32)base) + tex[k].offset) + (log2dim(tex[k].x4) << 28);
            k++;
        } while (k < resident_indexed_texture_count);
    }
    k = 0;
    tex = (new_var2 = (WadTex *)(((u8 *)hdr) + hdr->x44));
    if ((object_render_texture_definition_count = hdr->n40) > 0) {
        do {
            object_render_texture_lookup_table[k] = (((s32)base) + tex[k].offset) + (log2dim(tex[k].x4) << 28);
            k++;
        } while (k < object_render_texture_definition_count);
    }
    k = 0;
    tex = (new_var2 = (WadTex *)(((u8 *)hdr) + hdr->x4C));
    if ((shrub_render_texture_definition_count = hdr->n48) > 0) {
        do {
            shrub_render_texture_lookup_table[k] = (((s32)base) + tex[k].offset) + (log2dim(tex[k].x4) << 28);
            k++;
        } while (k < shrub_render_texture_definition_count);
    }
    initialize_tfrag_render_data(data + hdr->x10, ((u8 *)hdr) + hdr->x34);
    relocate_sky_definition(data + hdr->x14);
    c20 = (WadClass20_EABE8 *)(((u8 *)hdr) + hdr->x1C);
    resident_moby_class_count = 0;
    registered_object_render_class_count = 0;
    registered_shrub_render_class_count = 0;
    for (k = 0; k < hdr->n18; k++) {
        register_moby_class((c20->offset != 0) ? (data + c20->offset) : (0), ((u8 *)hdr) + hdr->x3C, c20->x10, c20->x4);
        c20++;
    }

    c20 = (WadClass20_EABE8 *)(((u8 *)hdr) + hdr->x24);
    for (k = 0; k < hdr->n20; k++) {
        register_object_render_class(data + c20->offset, ((u8 *)hdr) + hdr->x44, c20->x10, c20->x4);
        c20++;
    }

    c30 = (WadClass30_EABE8 *)(((u8 *)hdr) + hdr->x2C);
    for (k = 0; k < hdr->n28; k++) {
        register_shrub_render_class(data + c30->offset, ((u8 *)hdr) + hdr->x4C, c30->x10, c30->x20, c30->x4);
        c30++;
    }

    level_texture_payload_address = (s32)(data + hdr->x68);
    unpack_point_records(((u8 *)hdr) + hdr->x5C, hdr->x58);
    new_var3 = data + hdr->x64;
    new_var4 = hdr->x54;
    ParseParticleTexs(((u8 *)hdr) + (new_var = hdr->x6C), new_var3, ((u8 *)hdr) + new_var4, hdr->x50);
    sceGsSetDefLoadImage(li, (g_gs_texture_allocation_cursor_r << 8) >> 16, 4, 0, 0, 0, 0x100, 0x80);
    flush_cache(0);
    sceGsExecLoadImage(li, data + hdr->x84);
    wait_for_graphics_pipeline_idle(0, 0);
    level_resource_buffers.x18 = ((s32)level_resource_buffers.hdr) + size;
    v = g_gs_texture_allocation_cursor_r;
    D_J_0015F048 = ((v >> 8) | 0x20010000) | (((s64)0xB800) << 19);
    g_gs_texture_allocation_cursor_r = v + 0x20000;
    g_gs_texture_allocation_start_r = v + 0x20000;
    level_resource_buffers.x1C = initialize_level_render_state_r(data + hdr->x7C);
    FastMemSet(&render_sequence_r, 0, 0x1C0);
    FastMemSet(level_init_cleared_block, 0, 0x40);
    render_sequence_r.x58 = level_resource_buffers.x1C;
    render_sequence_r.x5C = (level_resource_buffers.x1C += 0x40000);
    level_resource_buffers.x1C += 0x40000;
    snd = (WadSound *)(data + hdr->x80);
    cnt = 0;
    if (snd->size != 0) {
        out = render_sequence_r.x60;
        do {
            cnt++;
            *out = ((s32)(data + hdr->x80)) + (snd->offset + k_800);
            snd++;
            out++;
        } while ((cnt < 0x46) && (snd->size != 0));
    }
    parse_space_scene_chunk(0);
    help_msg_data_r = level_resource_buffers.x1C;
    Load_r(level_resource_buffers.x1C, disc_table_r.help_text.sector, disc_table_r.help_text.size);
    help_offset_table_r = help_msg_data_r;
    level_resource_buffers.x1C = help_msg_data_r + (disc_table_r.help_text.size << 11);
    for (k = 0; k < 1; k++) {
        queue_dma_transfer_r(k);
        j = 0;
        if (g_help_state_r.count > 0) {
            te = help_msgs_r;
            adj = ((s32)te) - 8;
            {
                do {
                    te[j].offset += adj;
                    j++;
                } while (j < g_help_state_r.count);
            }
        }
    }

    cnt = (new_var = resident_class_slot_by_id[0x472]);
    if (cnt >= 0) {
        moby_class_resources[cnt]->sound_defs = (s32)moby_class_472_sound_defs;
        (*(u8 *)&moby_class_resources[cnt]->pad_D) = 5;
    }
}

extern s32 D_J_L00_001AFE00[];
s32 load_level_archive_resources(s32 full, s32 keep) {
    Load244110 *ld = ((Load244110 *)&lvl_resource_buffers);
    Hdr244 *h = (Hdr244 *)ld->f4;
    s32 *src = level_archive_load_state.shared;
    char *base;
    s32 used;
    char *e;
    char *free;
    char *end;
    Span244110 *span;
    TexRef244110 *tex;
    TexRef244110 *t1;
    TexRef244110 *t3;
    TexRef244110 *t4;
    char *tbl;
    s32 i;
    s32 j;
    s32 n;
    s32 k;
    s32 c1, c2, c3, c4;
    s32 m;
    char *p;
    s32 sq;
    char *sp;
    char *q;
    s32 z;
    s32 ci;
    char *x;
    char *y;
    s32 a1, a2, a3;
    char *q1, *q2;
    u64 *t;

    if (full)
        LoadHudBanks();
    base = ld->f14;
    if (full) {
        FastMemCopy(h, (char *)(src[4] + (s32)src), src[5]);
        level_texture_data_base = 0;
        copy_memory_by_alignment(mission_state, saved_mission_state + g_current_level_index * 16, 0x10);
        upload_texture_images((char *)(src[6] + (s32)src), h->v[0], (char *)h + h->v[1]);
        saved_sound_bank_handle = sound_bank_handle;
        flush_cache(0);
        used = fast_decompress((char *)(src[0x14] + (s32)src), base);

        level_texture_data_base = (s32)(base + h->v[0x18]);
        level_tex0_regs_128a[0] = (((u64)((g_gs_texture_allocation_base + h->v[0x24]) >> 8) | ((u64)(2) << 14) | ((u64)(0x13) << 20) | ((u64)(7) << 26) | ((u64)(7) << 30) | ((u64)(1) << 34) | ((u64)(0) << 35) | ((u64)((g_gs_texture_allocation_base + h->v[0x25]) >> 8) << 37) | ((u64)(0) << 51) | ((u64)(0) << 55) | ((u64)(0) << 56) | ((u64)(4) << 61)));
        level_tex0_regs_128a[1] = 0x0000FFA0000000E0;
        level_tex0_regs_128a[2] = 0x0040000400004000;
        level_tex0_regs_64[0] = (((u64)((g_gs_texture_allocation_base + h->v[0x26]) >> 8) | ((u64)(1) << 14) | ((u64)(0x13) << 20) | ((u64)(6) << 26) | ((u64)(6) << 30) | ((u64)(1) << 34) | ((u64)(0) << 35) | ((u64)((g_gs_texture_allocation_base + h->v[0x27]) >> 8) << 37) | ((u64)(0) << 51) | ((u64)(0) << 55) | ((u64)(0) << 56) | ((u64)(4) << 61)));
        level_tex0_regs_64[1] = 0x0000FFA0000000E0;
        level_tex0_regs_64[2] = 0x0040000400004000;
        level_tex0_regs_128b[0] = (((u64)((g_gs_texture_allocation_base + h->v[0x30]) >> 8) | ((u64)(2) << 14) | ((u64)(0x13) << 20) | ((u64)(7) << 26) | ((u64)(7) << 30) | ((u64)(1) << 34) | ((u64)(0) << 35) | ((u64)((g_gs_texture_allocation_base + h->v[0x31]) >> 8) << 37) | ((u64)(0) << 51) | ((u64)(0) << 55) | ((u64)(0) << 56) | ((u64)(4) << 61)));
        level_tex0_regs_128b[1] = 0x0000FFA0000000E0;
        level_tex0_regs_128b[2] = 0x0040000400004000;

        t1 = (TexRef244110 *)((char *)h + h->v[0xD]);
        tfrag_texture_ref_count = h->v[0xC];
        for (c1 = 0; c1 < tfrag_texture_ref_count; c1++)
            tfrag_texture_addrs[c1] = level_texture_data_base + t1[c1].offset + (log2dim(t1[c1].kind) << 28);
        tex = (TexRef244110 *)((char *)h + h->v[0xF]);
        moby_texture_count = h->v[0xE];
        for (c2 = 0; c2 < moby_texture_count; c2++)
            moby_texture_addrs[c2] = level_texture_data_base + tex[c2].offset + (log2dim(tex[c2].kind) << 28);
        FastMemCopy(moby_texture_records, tex, moby_texture_count * 16);
        t3 = (TexRef244110 *)((char *)h + h->v[0x11]);
        tie_texture_ref_count = h->v[0x10];
        for (c3 = 0; c3 < tie_texture_ref_count; c3++)
            tie_texture_addrs[c3] = level_texture_data_base + t3[c3].offset + (log2dim(t3[c3].kind) << 28);
        t4 = (TexRef244110 *)((char *)h + h->v[0x13]);
        shrub_texture_ref_count = h->v[0x12];
        for (c4 = 0; c4 < shrub_texture_ref_count; c4++)
            shrub_texture_addrs[c4] = level_texture_data_base + t4[c4].offset + (log2dim(t4[c4].kind) << 28);

        initialize_tfrag_render_data(base + h->v[2], (char *)h + h->v[0xD]);
        if (h->v[3])
            occlusion_grid = (s32)(base + h->v[3]);
        else
            occlusion_grid = 0;
        if (h->v[4])
            relocate_sky_definition(base + h->v[4]);
        else
            sky_data = 0;
        resolve_level_asset_block_pointers(base + h->v[5]);
        for (i = 0, q2 = (char *)h + h->v[7]; i < h->v[6]; i++, q2 += 0x20)
            register_moby_class(*(s32 *)q2 ? base + *(s32 *)q2 : 0, (char *)h + h->v[0xF], q2 + 0x10, *(s32 *)(q2 + 4));
        print_moby_class_sizes((int *)((char *)h + h->v[7]), h->v[7]);
        for (sq = 0, p = (char *)h + h->v[0x1E]; sq < moby_class_headers[0]->seq_count; sq++, p += 4) {
            if (*(s32 *)p) {
                moby_class_headers[0]->seqs[sq] = base + *(s32 *)p;
                relocateObjectPointers(moby_class_headers[0], sq);
            }
        }
        loaded_swap_moby_class_set = -1;
        q = (char *)h + h->v[0x21];
        swap_moby_class_block_field_8 = 0;
        moby_class_reference_count = h->v[0x20];
        for (m = 0; m < h->v[0x20]; m++, q += 0x10) {
            swap_moby_class_ids[m] = *(s32 *)(q + 4);
            swap_moby_class_data[m] = base + *(s32 *)q;
            D_J_L00_001AFE00[m] = *(s32 *)(q + 8);
            { s32 o = g_class_slot[*(s32 *)(q + 4)] * 32 + 0x10;
            qcopy(&swap_moby_class_headers[m], (char *)h + h->v[7] + o); }
        }
        for (i = 0, q2 = (char *)h + h->v[9]; i < h->v[8]; i++, q2 += 0x20)
            register_object_render_class(base + *(s32 *)q2, (char *)h + h->v[0x11], q2 + 0x10, *(s32 *)(q2 + 4));
        Profile_Mark((char *)h + h->v[9], h->v[8]);
        for (i = 0, q2 = (char *)h + h->v[0xB]; i < h->v[0xA]; i++, q2 += 0x30)
            register_shrub_render_class(base + *(s32 *)q2, (char *)h + h->v[0x13], q2 + 0x10, q2 + 0x20, *(s32 *)(q2 + 4));
        level_effects_data = base + h->v[0x1A];
        unpack_point_records((char *)h + h->v[0x17], h->v[0x16]);
        if (h->v[0x28]) {
            relocate_counted_pointer_table(base + h->v[0x28]);
        } else {
            level_textured_sphere_count = 0;
            level_textured_spheres = 0;
            level_textured_sphere_textures = 0;
        }
        if (h->v[0x29]) {
            load_two_int_two_float_header(base + h->v[0x29]);
        } else {
            height_map_width = 0;
            height_map_rows = 0;
            height_map_max_height = 0;
            height_map_min_height = 0;
            height_map_data = 0;
        }
        if (h->v[0x2A])
            level_header_section_2a_data = base + h->v[0x2A];
        else
            level_header_section_2a_data = 0;
        {
            char *x1 = base + h->v[0x19];
            char *x2 = (char *)h + h->v[0x15];
            ParseParticleTexs((char *)h + h->v[0x1B], x1, x2, h->v[0x14]);
        }

        tbl = (char *)h + h->v[0x1C];
        span = (Span244110 *)tbl;
        free = (char *)((u32)(base + used + 0x3F) & ~0x3F);
        n = span->count;
        level_sound_slots = (Slot244110 *)free;
        level_sound_slot_count = n;
        copy_memory_by_alignment(free, tbl + span->off, n * 32);
        span++;
        free += n * 32;
        e = tbl + span->off;
        for (a3 = 0; a3 < level_sound_slot_count; a3++) {
            if (level_sound_slots[a3].sound >= span->count) {
                ((void (*)())STUB_printf)(sound_slot_out_of_range_msg);
                level_sound_slots[a3].sound = -1;
            } else {
                level_sound_slots[a3].sound = *(u16 *)(e + level_sound_slots[a3].sound * 4);
            }
            level_sound_slots[a3].time = sound_bank_handle;
        }
        end = free + 0x3F;
        span++;
        for (ci = 0; ci < moby_class_count; ci++, span++) {
            Class244110 *cls = moby_class_headers[ci];
            if (cls == 0) {
                for (j = 0; j < moby_class_reference_count; j++)
                    if (swap_moby_class_ids[j] == moby_class_ids[ci])
                        break;
                if (j < moby_class_reference_count) {
                    FastMemSet(swap_moby_class_sounds[j], -1, 0x20);
                    e = tbl + span->off;
                    for (k = 0; k < span->count && k < 15; k++, e += 4)
                        swap_moby_class_sounds[j][k] = *(u16 *)e;
                }
            } else {
                if (span->count != cls->count0D)
                    ((void (*)())STUB_printf)(class_sound_count_mismatch_fmt, moby_class_ids[ci]);
                e = tbl + span->off;
                for (k = 0; k < moby_class_headers[ci]->count0D; k++, e += 4) {
                    moby_class_headers[ci]->f28[k].sound = *(u16 *)e;
                    moby_class_headers[ci]->f28[k].time = sound_bank_handle;
                }
            }
        }
        ((Load244110 *)&lvl_resource_buffers)->f18 = load_level_texture_bank((char *)((u32)end & ~0x3F), (s32 *)h);
    }
    ((Load244110 *)&lvl_resource_buffers)->f1C = load_level_gameplay(full);
    if (full) {
        char *top;
        ((void (*)())STUB_printf)(mem_report_header_msg);
        ((void (*)())STUB_printf)(mem_report_low_mem_fmt, (((*(f64 (*)(f32))func_00120778)(IntToFloat((s32)(0x100000)) * (1.0f / 1024.0f)))));
        top = level_overlay_end;
        ((void (*)())STUB_printf)(mem_report_code_fmt, (((*(f64 (*)(f32))func_00120778)(IntToFloat((s32)(top - 0x100000)) * (1.0f / 1024.0f)))));
        ((void (*)())STUB_printf)(mem_report_buffer1_fmt, (((*(f64 (*)(f32))func_00120778)(IntToFloat((s32)(((Load244110 *)&lvl_resource_buffers)->f0 - top)) * (1.0f / 1024.0f)))));
        ((void (*)())STUB_printf)(mem_report_buffer2_fmt, (((*(f64 (*)(f32))func_00120778)(IntToFloat((s32)(((Load244110 *)&lvl_resource_buffers)->fC - ((Load244110 *)&lvl_resource_buffers)->f0)) * (1.0f / 1024.0f)))));
        ((void (*)())STUB_printf)(mem_report_buffer3_fmt, (((*(f64 (*)(f32))func_00120778)(IntToFloat((s32)(((Load244110 *)&lvl_resource_buffers)->f10 - ((Load244110 *)&lvl_resource_buffers)->fC)) * (1.0f / 1024.0f)))));
        ((void (*)())STUB_printf)(mem_report_buffer4_fmt, (((*(f64 (*)(f32))func_00120778)(IntToFloat((s32)(((Load244110 *)&lvl_resource_buffers)->f14 - ((Load244110 *)&lvl_resource_buffers)->f10)) * (1.0f / 1024.0f)))));
        if (occlusion_grid != 0)
            ((void (*)())STUB_printf)(mem_report_base_data_fmt, (((*(f64 (*)(f32))func_00120778)(IntToFloat((s32)((char *)occlusion_grid - ((Load244110 *)&lvl_resource_buffers)->f14)) * (1.0f / 1024.0f)))));
        else if (sky_data != 0)
            ((void (*)())STUB_printf)(mem_report_base_data_fmt, (((*(f64 (*)(f32))func_00120778)(IntToFloat((s32)((char *)sky_data - ((Load244110 *)&lvl_resource_buffers)->f14)) * (1.0f / 1024.0f)))));
        else
            ((void (*)())STUB_printf)(mem_report_no_base_data_msg);
        if (occlusion_grid != 0) {
            if (sky_data != 0)
                ((void (*)())STUB_printf)(mem_report_occlusion_fmt, (((*(f64 (*)(f32))func_00120778)(IntToFloat((s32)(sky_data - (s32)occlusion_grid)) * (1.0f / 1024.0f)))));
            else
                ((void (*)())STUB_printf)(mem_report_occlusion_fmt, (((*(f64 (*)(f32))func_00120778)(IntToFloat((s32)(((char *)coll_hit_result.f0) - (s32)occlusion_grid)) * (1.0f / 1024.0f)))));
        }
        if (h->v[4])
            ((void (*)())STUB_printf)(mem_report_sky_fmt, (((*(f64 (*)(f32))func_00120778)(IntToFloat((s32)(((char *)coll_hit_result.f0) - (char *)sky_data)) * (1.0f / 1024.0f)))));
        ((void (*)())STUB_printf)(mem_report_collision_fmt, (((*(f64 (*)(f32))func_00120778)(IntToFloat((s32)(level_texture_data_base - (s32)((char *)coll_hit_result.f0))) * (1.0f / 1024.0f)))));
        ((void (*)())STUB_printf)(mem_report_texture_data_fmt, (((*(f64 (*)(f32))func_00120778)(IntToFloat((s32)((particle_texture_vram_table[1] >> 4) - level_texture_data_base)) * (1.0f / 1024.0f)))));
        ((void (*)())STUB_printf)(mem_report_particle_vram_fmt, (((*(f64 (*)(f32))func_00120778)(IntToFloat((s32)((s32)level_effects_data - (particle_texture_vram_table[1] >> 4))) * (1.0f / 1024.0f)))));
        ((void (*)())STUB_printf)(mem_report_effects_vram_fmt, (((*(f64 (*)(f32))func_00120778)(IntToFloat((s32)((char *)moby_class_headers[0] - level_effects_data)) * (1.0f / 1024.0f)))));
        ((void (*)())STUB_printf)(mem_report_moby_classes_fmt, (((*(f64 (*)(f32))func_00120778)(IntToFloat((s32)(((char *)level_tie_classes[0]) - (char *)moby_class_headers[0])) * (1.0f / 1024.0f)))));
        if (h->v[0xA])
            ((void (*)())STUB_printf)(mem_report_tie_classes_fmt, (((*(f64 (*)(f32))func_00120778)(IntToFloat((s32)(((char *)shrub_class_table[0]) - ((char *)level_tie_classes[0]))) * (1.0f / 1024.0f)))));
        else
            ((void (*)())STUB_printf)(mem_report_tie_classes_fmt, (((*(f64 (*)(f32))func_00120778)(IntToFloat((s32)(((char *)tie_instances) - ((char *)level_tie_classes[0]))) * (1.0f / 1024.0f)))));
        if (h->v[0xA])
            ((void (*)())STUB_printf)(mem_report_shrub_classes_fmt, (((*(f64 (*)(f32))func_00120778)(IntToFloat((s32)(moby_class_headers[0]->seqs[0] - ((char *)shrub_class_table[0]))) * (1.0f / 1024.0f)))));
        ((void (*)())STUB_printf)(mem_report_moby_seqs_fmt, (((*(f64 (*)(f32))func_00120778)(IntToFloat((s32)(((char *)tie_instances) - moby_class_headers[0]->seqs[0])) * (1.0f / 1024.0f)))));
        ((void (*)())STUB_printf)(mem_report_tie_instances_fmt, (((*(f64 (*)(f32))func_00120778)(IntToFloat((s32)(((char *)shrub_instances) - ((char *)tie_instances))) * (1.0f / 1024.0f)))));
        ((void (*)())STUB_printf)(mem_report_shrub_instances_fmt, (((*(f64 (*)(f32))func_00120778)(IntToFloat((s32)(((char *)moby_array_start) - ((char *)shrub_instances))) * (1.0f / 1024.0f)))));
        ((void (*)())STUB_printf)(mem_report_mobys_fmt, (((*(f64 (*)(f32))func_00120778)(IntToFloat((s32)(moby_spawn_pvar_pool - ((char *)moby_array_start))) * (1.0f / 1024.0f)))));
        ((void (*)())STUB_printf)(mem_report_moby_pvars_fmt, (((*(f64 (*)(f32))func_00120778)(IntToFloat((s32)(level_path_data - moby_spawn_pvar_pool)) * (1.0f / 1024.0f)))));
        ((void (*)())STUB_printf)(mem_report_paths_fmt, (((*(f64 (*)(f32))func_00120778)(IntToFloat((s32)(particle_pool - level_path_data)) * (1.0f / 1024.0f)))));
        ((void (*)())STUB_printf)(mem_report_particles_fmt, 128.0);
        ((void (*)())STUB_printf)(mem_report_separator_msg);
        ((void (*)())STUB_printf)(mem_report_particle_pool_end_fmt, (*(f64 (*)(f32))func_00120778)(IntToFloat((s32)particle_pool + 0x20000) * (1.0f / 1048576.0f)));
        ((void (*)())STUB_printf)(mem_report_separator_msg);
        {
            char *lo = ((Load244110 *)&lvl_resource_buffers)->f0 - 0x2D0000;
            ((void (*)())STUB_printf)(mem_report_total_used_fmt, (*(f64 (*)(f32))func_00120778)(IntToFloat(((Load244110 *)&lvl_resource_buffers)->f1C - lo) * (1.0f / 1048576.0f)));
        }
        ((void (*)())STUB_printf)(mem_report_separator_msg);
    }
    if (keep) {
        restore_or_clear_level_state();
    } else {
        clear_scene_state_buffers();
        for (z = 15; z >= 0; z--)
            mission_death_counts[z] = 0;
    }
    active_scene_trigger.f10 = 0;
    *(s32 *)&active_scene_trigger.f8 = 0;
    *(s32 *)&active_scene_trigger.fC = 0;
    ((void (*)(void))link_and_update_all_mobys)();
    level_frame_counter++;
    return 0;
}

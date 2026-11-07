extern unsigned char D_E1_L00_001BB684[];
extern Tfrag *D_E1_L00_00160F0C SDATA(D_E1_L00_00160F0C);
char *load_level_gameplay(int arg0) {
    Hdr_422D8 *hdr;
    char *mem;
    float *ship;
    short *idx =
        gameplay_moby_index_remap; /* moby index by file order; a pointer local, so its address is formed at each use */
    int tmp; /* scratch int: the level number in the moby loop, a group's offset and end flag in the group loop */

    hdr = lvl_resource_buffers.f4;
    mem = lvl_resource_buffers.f18;
    if (arg0 == 0) {
        if (pal_mode != 0) {
            Load_r(mem, disc_table_r2.sound_archive_alt.sector, disc_table_r2.sound_archive_alt.size);
        } else {
            Load_r(mem, disc_table_r2.sound_archive.sector, disc_table_r2.sound_archive.size);
        }
        level_root[1] = mem; /* sound_archive_base */
    }
    flush_cache(0);
    fast_decompress(level_root[1], hdr);
    flush_cache(0);
    FastMemSet(&g_hero, 0, 0x2310);
    FastMemSet(gCameraLvl, 0, 0x3A0);
    FastMemSet(moby_animation_slots, 0, 0x180);
    srand(0x4D2);
    if (pal_mode != 0) {
        if (frame_scale == 1.0f) {
            SetUpdateModeTiming(1);
        }
    } else {
        if (frame_scale != 1.0f) {
            SetUpdateModeTiming(0);
        }
    }
    coll_hit_result.f4 = coll_hit_vector_buffer;
    coll_hit_result.f8 = coll_hit_moby_record;
    coll_hit_result.fC = 0;
    coll_hit_result.f10 = 0;
    coll_hit_result.f14 = 0;
    FastMemZero16(coll_cell_grid, 0x10000);
    FastMemZero16(coll_cell_slot_bitmap, 0x60);
    {
        int *p = (int *)((char *)hdr + hdr->f0);
        int *c;

        render_view_params.f23C = *p++;
        render_view_params.f240 = *p++;
        render_view_params.f244 = *p++;
        *(unsigned char *)&level_fog_color_red = *p++;
        *(unsigned char *)&level_fog_color_green = *p++;
        *(unsigned char *)&level_fog_color_blue = *p++;
        level_fog_near_distance = ((float *)p)[0];
        level_fog_far_distance = ((float *)p)[1];
        level_fog_near_intensity = ((float *)p)[2];
        level_fog_far_intensity = ((float *)p)[3];
        veldin1_hero_fall_state_height = ((float *)p)[4];
        ship = (float *)(p + 5);
        c = p + 9;
        p += 12;
        level_render_state.f30 = c[0];
        level_render_state.f34 = c[1];
        level_render_state.f38 = c[2];
    }
    UpdateFog();
    level_tfrag_draw_distance = 512000.0f;
    level_tie_draw_distance = 720.0f;
    shrub_draw_distance = 500.0f;
    moby_draw_distance = 500;
    level_particle_clip_dist = 0x1F4000;
    SetBackgroundColor(render_view_params.f23C, render_view_params.f240, render_view_params.f244);
    VU0_loadMicroProgram(frame_vu0_micro_program);
    FastMemZero16(level_point_lights, 0x100);
    FastMemZero16(level_point_light_data, 0x180);
    FastMemZero16(level_directional_lights, 0x400);
    {
        char *p = (char *)hdr + hdr->f4;
        int n = *(int *)p;

        p += 0x10;
        if (n >= 12) {
            STUB_printf(too_many_moby_lights_msg);
            n = 12;
        }
        if (n != 0) {
            copy_memory_by_alignment_v(level_directional_lights, p, n << 6);
        }
    }
    if (hdr->f80 != 0) {
        char *p = (char *)hdr + hdr->f80;

        env_zone_count = *(int *)p;
        p += 0x10;
        if (env_zone_count != 0) {
            Light *l;
            int i;

            copy_memory_by_alignment_v(env_zone_spheres, p, env_zone_count << 4);
            copy_memory_by_alignment_v(l00_gameplay_lights, p + (env_zone_count << 4), env_zone_count << 7);
            for (i = 0; i < env_zone_count; i++) {
                l = &l00_gameplay_lights[i];
                if (l->f48 < 0) {
                    l->f48 = 11;
                }
                if (l->f4C < 0) {
                    l->f4C = 11;
                }
            }
        }
    } else {
        env_zone_count = 0;
    }
    {
        int i;

        for (i = 0; i < tfrag_count; i++) {
            tfrag_records[i].f36 = 0xFFFF;
        }
    }
    {
        int i = 0;

        while (i < tfrag_count) {
            unsigned short *q = (unsigned short *)0x70003000;
            int j;

            for (j = 0; j < 0x3FF && i < tfrag_count; j++) {
                *q = i;
                q++;
                i++;
            }
            *q = 0xFFFF;
            LightTfrags((void *)0x70003000);
        }
    }
    if (hdr->f10[0] != 0) {
        int lang = g_game_language * 4;
        char *p = (char *)hdr + *(int *)((char *)hdr + lang + 0x10);
        int size;
        int i;

        help_popup_state.f2C = *(int *)p;
        p += 4;
        size = *(int *)p;
        p += 4;
        help_text_entries = (Text *)mem;
        size += 3;
        size &= ~3;
        size -= 8;
        copy_memory_by_alignment_v(mem, p, size);
        for (i = 0; i < help_popup_state.f2C; i++) {
            int delta = (int)help_text_entries - 8;

            help_text_entries[i].f0 += delta;
        }
        mem = (char *)(((u32)mem + size + 0x3F) & 0xFFFFFFC0);
    }
    {
        char *p = (char *)hdr + hdr->f34;
        int n = *(int *)p;
        int i;
        int last;

        level_tie_instances = (TieInst *)mem;
        p += 0x10;
        tie_instance_count = n;
        mem += n << 5;
        if (n != 0) {
            FastMemSet(level_tie_instances, 0, n << 5);
        }
        mem = (char *)(((u32)mem + 0x3F) & 0xFFFFFFC0);
        level_tie_matrices = (TieMtx *)mem;
        mem += tie_instance_count * 0x1C0;
        if (tie_instance_count != 0) {
            FastMemSet(level_tie_matrices, 0, tie_instance_count * 0x1C0);
        }
        last = -1;
        tie_instances_end = level_tie_instances + tie_instance_count;
        for (i = 0; i < tie_instance_count; i++) {
            TieInst *inst = level_tie_instances + i;
            TieMtx *mtx = level_tie_matrices + i;
            int id;
            float scale;

            inst->f1A = tie_class_slot_by_id[*(int *)p];
            if (inst->f1A != last) {
                level_tie_classes[inst->f1A]->f28 = inst;
                level_tie_classes[inst->f1A]->f26 = 0;
                last = inst->f1A;
            }
            level_tie_classes[inst->f1A]->f26++;
            inst->f10 = mtx;
            inst->f14 = (float)*(int *)(p + 4);
            inst->f1E = 0xFFFF;
            inst->f1B = 0;
            inst->f1C = 0;
            id = *(int *)(p + 0xC);
            qcopy(mtx, p + 0x10);
            qcopy(mtx->m1, p + 0x20);
            qcopy(mtx->m2, p + 0x30);
            qcopy(mtx->m3, p + 0x40);
            inst->f18 = id;
            p += 0x50;
            mtx->f3C = level_tie_classes[inst->f1A]->f40;
            transform_vector_by_basis(inst, level_tie_classes[inst->f1A]->f30, mtx);
            scale = FastVecLength(mtx);
            scale = max_float(scale, FastVecLength(mtx->m1));
            inst->fC = level_tie_classes[inst->f1A]->f3C * max_float(scale, FastVecLength(mtx->m2));
            scale_vector(inst, inst, mtx->f3C);
            FastVecAdd(inst, inst, mtx->m3);
            level_tie_matrices[i].fC = 1.0f / FastVecLength(&level_tie_matrices[i]);
            level_tie_matrices[i].f1C = 1.0f / FastVecLength(level_tie_matrices[i].m1);
            level_tie_matrices[i].f2C = 1.0f / FastVecLength(level_tie_matrices[i].m2);
            copy_memory_by_alignment_v(mtx->f140, p, 0x80);
            p += 0x80;
            inst->f1C = *(unsigned short *)p;
            p += 0x10;
        }
    }
    {
        unsigned short *q = (unsigned short *)0x70000000;
        int i;

        for (i = 0; i < tie_instance_count; i++) {
            *q++ = i;
        }
        *q = 0xFFFF;
        LightTies((void *)0x70000000);
    }
    {
        char *p = (char *)hdr + hdr->f3C;
        int n = *(int *)p;
        int last;
        int i;

        level_shrub_instances = (ShrubInst *)mem;
        p += 0x10;
        level_shrub_count = n;
        mem += n << 5;
        if (n != 0) {
            FastMemSet(level_shrub_instances, 0, n << 5);
        }
        mem = (char *)(((u32)mem + 0x3F) & 0xFFFFFFC0);
        shrub_matrices = (ShrubMtx *)mem;
        mem += level_shrub_count << 6;
        if (level_shrub_count != 0) {
            FastMemSet(shrub_matrices, 0, level_shrub_count << 6);
        }
        shrub_vertex_colors = (unsigned char *)mem;
        mem += level_shrub_count * 0x60;
        mem = (char *)(((u32)mem + 0x3F) & 0xFFFFFFC0);
        if (level_shrub_count != 0) {
            FastMemSet(shrub_vertex_colors, 0, level_shrub_count * 0x60);
        }
        last = -1;
        shrub_color_buffer_descriptor[1] = (int)shrub_vertex_colors;
        shrub_instances_end = level_shrub_instances + level_shrub_count;
        for (i = 0; i < level_shrub_count; i++) {
            ShrubInst *inst = level_shrub_instances + i;
            ShrubMtx *mtx = shrub_matrices + i;
            int cls = shrub_class_slot_by_id[*(int *)p];
            float scale;

            inst->f1A = cls;
            if (cls != last) {
                last = cls;
                shrub_class_table[cls]->f18 = inst;
                shrub_class_table[cls]->f16 = 0;
            }
            shrub_class_table[cls]->f16++;
            inst->f18 = i;
            inst->f10 = *(float *)(p + 4);
            if (inst->f10 < 16.0f) {
                inst->f10 = 16.0f;
            }
            inst->f1B = 0;
            inst->f1E = 0xFFFF;
            if (shrub_class_table[cls]->f1C != 0) {
                float d;

                inst->f17 = truncate_float_to_s32(*shrub_class_table[cls]->f1C);
                d = IntToFloat(inst->f17) + 24.0f;
                if (inst->f10 < d) {
                    inst->f10 = d;
                }
            }
            qcopy(mtx, p + 0x10);
            qcopy(mtx->m1, p + 0x20);
            qcopy(mtx->m2, p + 0x30);
            qcopy(mtx->m3, p + 0x40);
            p += 0x50;
            mtx->f3C = shrub_class_table[cls]->f20;
            {
                float a = (FastVecLength(mtx) + FastVecLength(mtx->m1)) * 0.5f;
                float b = FastVecLength(mtx->m2);
                int ia = truncate_float_to_s32(a * 4096.0f);
                int ib = truncate_float_to_s32(b * 4096.0f);

                if (ib > 0x10000) {
                    ib = 0x10000;
                }
                if (ia > 0x10000) {
                    ia = 0x10000;
                }
                mtx->f2C = ia | (ib << 16);
            }
            {
                int r;
                int g;
                int b;

                r = *(int *)p;
                p += 4;
                g = *(int *)p;
                p += 4;
                b = *(int *)p;
                p += 8;
                g = (g << 8) | 0x80000000;
                mtx->fC = (b << 16) | g | r;
            }
            inst->f1C = *(unsigned short *)p;
            p += 0x10;
            transform_vector_by_basis(inst, shrub_class_table[cls], mtx);
            scale = FastVecLength(mtx);
            scale = max_float(scale, FastVecLength(mtx->m1));
            inst->fC = shrub_class_table[cls]->fC * max_float(scale, FastVecLength(mtx->m2));
            scale_vector(inst, inst, mtx->f3C);
            FastVecAdd(inst, inst, mtx->m3);
        }
    }
    {
        unsigned short *q = (unsigned short *)0x70000000;
        int i;

        for (i = 0; i < level_shrub_count; i++) {
            *q++ = i;
        }
        *q = 0xFFFF;
        LightShrubs((void *)0x70000000);
    }
    {
        int i;

        for (i = 0; i < level_shrub_count; i++) {
            unsigned char *q = shrub_vertex_colors + i * 0x60;
            int r = 0;
            int g = 0;
            int b = 0;
            int n = 24;
            int j;

            for (j = 0; j < n; j++) {
                r += q[0];
                g += q[1];
                b += q[2];
                q += 4;
            }
            g /= n;
            b /= n;
            r /= n;
            *(int *)((char *)shrub_matrices + i * 0x40 + 0x1C) = (b << 16) | (g << 8) | r;
        }
    }
    if (hdr->f8 != 0) {
        char *p = (char *)hdr + hdr->f8;
        int n = *(int *)p;
        int i;

        camera_descriptors = (Cuboid *)mem;
        p += 0x10;
        gCameraDefCountLvl = n;
        mem += n << 5;
        if (n != 0) {
            FastMemSet(camera_descriptors, 0, n << 5);
        }
        mem = (char *)(((u32)mem + 0x3F) & 0xFFFFFFC0);
        for (i = 0; i < gCameraDefCountLvl; i++) {
            camera_descriptors[i].fC = *(int *)p;
            p += 4;
            camera_descriptors[i].f0 = ((float *)p)[0];
            camera_descriptors[i].f4 = ((float *)p)[1];
            camera_descriptors[i].f8 = ((float *)p)[2];
            p += 0xC;
            camera_descriptors[i].f10 = ((float *)p)[0];
            camera_descriptors[i].f14 = ((float *)p)[1];
            camera_descriptors[i].f18 = ((float *)p)[2];
            p += 0xC;
            camera_descriptors[i].f1C = *(int *)p;
            p += 4;
        }
    }
    if (hdr->fC != 0) {
        char *p = (char *)hdr + hdr->fC;
        int n = *(int *)p;

        p += 0x10;
        voice_pool.fD90 = n;
        if (n != 0) {
            int size = n * 0x90;
            int i;

            voice_pool.fD94 = (SndInst *)mem;
            mem += size;
            mem = (char *)(((u32)mem + 0x3F) & 0xFFFFFFC0);
            FastMemSet(voice_pool.fD94, 0, size);
            for (i = 0; i < voice_pool.fD90; i++) {
                SndInst *e = voice_pool.fD94 + i;

                copy_memory_by_alignment_v(e, p, 0x90);
                p += 0x90;
                resolve_lookup_record((short *)e);
            }
        } else {
            voice_pool.fD94 = 0;
        }
    }
    {
        char *p = (char *)hdr + hdr->f44;
        int n2;
        int count = 0;
        int n;
        char *next;
        int i;

        n = *(int *)p;
        p += 4;
        n2 = *(int *)p;
        p += 0xC;
        moby_array_start = (Moby *)mem;
        FastMemSet(mem, 0, (n + n2) << 8);
        for (i = 0; i < n; i++) {
            int mission;
            int flags;
            int uid;
            int bolts;
            int bolts2;
            int b1;
            int b2;
            unsigned char vis = 0xFE;
            int skip = 0;

            next = p + ((*(int *)p >> 2) << 2);
            p += 4;
            mission = *(int *)p;
            p += 4;
            flags = *(int *)p;
            p += 4;
            uid = *(int *)p;
            p += 4;
            bolts = *(int *)p;
            p += 4;
            bolts2 = *(int *)p;
            p += 4;
            b1 = bolts;
            b2 = b1;
            if (flags != 0) {
                tmp = g_current_level_index; /* read before the test: see the group loop */
                if (flags & 0x10) {
                    vis = find_or_assign_bolt_source_slot(tmp, uid);
                } else {
                    vis = 0xFF;
                }
                if (D_E1_L00_001BB684[uid] != 0) {
                    skip = 1;
                } else if (flags & 3) {
                    if (mission_state[mission] == 0xFF) {
                        if (flags & 2) {
                            b1 = bolts2;
                            if ((session_moby_killed[uid >> 5] >> (uid & 0x1F)) & 1) {
                                b2 = (b1 + 1) / 2;
                            } else {
                                b2 = b1;
                            }
                        } else {
                            skip = 1;
                        }
                    } else {
                        if (flags & 1) {
                            if ((level_moby_killed[g_current_level_index][uid >> 5] >> (uid & 0x1F)) & 1) {
                                b2 = (bolts + 1) / 2;
                            }
                        } else {
                            skip = 1;
                        }
                    }
                } else if (flags & 8) {
                    if ((session_moby_killed[uid >> 5] >> (uid & 0x1F)) & 1) {
                        skip = 1;
                    }
                } else if ((flags & 0xC) == 4) {
                    if ((level_moby_killed[g_current_level_index][uid >> 5] >> (uid & 0x1F)) & 1) {
                        skip = 1;
                    }
                }
            }
            if (skip) {
                idx[i] = -1;
            } else {
                Moby *m;
                int a;
                float h;
                int pv;
                int c0;
                int c1;
                int c2;
                int upd;

                idx[i] = count;
                m = moby_array_start + count;
                InitMobyInstance(m, *(int *)p);
                p += 4;
                m->unkB1 = vis;
                m->save_id = uid;
                m->mission = mission;
                m->unkB4 = b2;
                m->unkB6 = b1;
                if (m->pclass != 0) {
                    m->scale = m->pclass->scale * *(float *)p;
                }
                p += 4;
                m->draw_dist = *(unsigned short *)p;
                p += 4;
                m->update_dist = *(unsigned char *)p;
                p += 0xC;
                m->pos.x = ((float *)p)[0];
                m->pos.y = ((float *)p)[1];
                m->pos.z = ((float *)p)[2];
                p += 0xC;
                m->rot.x = ((float *)p)[0];
                m->rot.y = ((float *)p)[1];
                m->rot.z = ((float *)p)[2];
                p += 0xC;
                m->group = *(unsigned char *)p;
                p += 4;
                a = *(int *)p;
                p += 4;
                h = *(float *)p;
                p += 4;
                if (a != 0) {
                    float z = probe_ground_height(&m->pos.x, 0, 0.5f);

                    if (0.0f < z) {
                        m->pos.z = z + h;
                    }
                }
                p += 4;
                pv = *(int *)p;
                m->occlusion = 0x7F80;
                p += 4;
                (*(int *)&m->pvars) = pv;
                if (*(int *)p == 0) {
                    m->occlusion = 0;
                }
                p += 4;
                if (m->pclass != 0) {
                    m->pclass->flags |= *(unsigned short *)p;
                }
                m->flags |= *(unsigned short *)p;
                p += 4;
                if (m->pclass != 0) {
                    moby_build_matrix(m);
                }
                c0 = *(int *)p;
                p += 4;
                c1 = *(int *)p;
                p += 4;
                c2 = *(int *)p;
                p += 4;
                upd = *(int *)p;
                p += 4;
                m->unk80 = (c2 << 16) + (c1 << 8) + c0;
                pack_render_command_fields(m, m->unk80, 0, 0, 0);
                (*(int *)&m->spawn_frame) = upd;
                if (*(int *)p != -1) {
                    store_current_level_table_value((int)m, *(int *)p);
                }
                count++;
            }
            p = next;
        }
        level_mobys_end = moby_array_start + count;
        level_mobys_end->state = 0xFF;
        mem += (count + n2) << 8;
        l00_active_moby_pool_end = mem;
        mem += n2 << 7;
        mem = (char *)(((u32)mem + 0x3F) & 0xFFFFFFC0);
        moby_array_last = (Moby *)((int)moby_array_start + ((count + n2) << 8) - 0x100);
    }
    if (hdr->f58 != 0) {
        char *pv = (char *)hdr + hdr->f58;
        PvarEnt *tbl = (PvarEnt *)((char *)hdr + hdr->f54);
        int i;
        Moby *m;

        for (i = 0; i < gCameraDefCountLvl; i++) {
            int k = camera_descriptors[i].f1C;

            if (k < 0) {
                camera_descriptors[i].f1C = 0;
            } else {
                int size = tbl[k].f4;
                char *src = pv + tbl[k].f0;

                camera_descriptors[i].f1C = (int)mem;
                FastMemCopy(mem, src, size);
                tbl[k].f0 = (int)mem;
                mem += size;
            }
        }
        for (i = 0; i < voice_pool.fD90; i++) {
            int k = voice_pool.fD94[i].f8;

            if (k < 0) {
                voice_pool.fD94[i].f8 = 0;
            } else {
                int size = tbl[k].f4;
                char *src = pv + tbl[k].f0;

                voice_pool.fD94[i].f8 = (int)mem;
                FastMemCopy(mem, src, size);
                tbl[k].f0 = (int)mem;
                mem += size;
            }
        }
        for (m = moby_array_start; m != level_mobys_end; m++) {
            int k = (*(int *)&m->pvars);

            if (k < 0) {
                (*(int *)&m->pvars) = 0;
            } else {
                int size = tbl[k].f4;
                char *src = pv + tbl[k].f0;

                (*(int *)&m->pvars) = (int)mem;
                FastMemCopy(mem, src, size);
                tbl[k].f0 = (int)mem;
                mem += size;
            }
        }
        mem = (char *)(((u32)mem + 0x3F) & 0xFFFFFFC0);
    }
    if (hdr->f50 != 0) {
        int *q = (int *)((char *)hdr + hdr->f50);
        PvarEnt *tbl = (PvarEnt *)((char *)hdr + hdr->f54);

        while (q[0] >= 0) {
            int x = tbl[q[0]].f0;
            int off = q[1];

            if (x >= (int)hdr) {
                int *r = (int *)(x + off);

                if (*r >= 0) {
                    *r = idx[*r];
                }
            }
            q += 2;
        }
    }
    if (hdr->f5C != 0) {
        int *q = (int *)((char *)hdr + hdr->f5C);
        PvarEnt *tbl = (PvarEnt *)((char *)hdr + hdr->f54);

        while (q[0] >= 0) {
            int x = tbl[q[0]].f0;
            int off = q[1];

            if (x >= (int)hdr) {
                int *r = (int *)(x + off);

                *r += x;
            }
            q += 2;
        }
    }
    if (hdr->f48 != 0) {
        char *q = (char *)hdr + hdr->f48;
        char *base = mem;
        int *offs;
        int n;
        int size;
        int i;

        n = *(int *)q;
        q += 4;
        size = *(int *)q;
        offs = (int *)(q + 0xC);
        mem += size;
        q = (char *)offs + (n << 2);
        mem = (char *)(((u32)mem + 0x3F) & 0xFFFFFFC0);
        if (size != 0) {
            FastMemCopy(base, q, size);
        }
        FastMemSet(level_moby_groups, 0, 0x1C0);
        for (i = 0; i < n; i++, offs++) {
            tmp = *offs;
            if (tmp >= 0) {
                int g = (int)(base + tmp);
                unsigned short *src;
                short *dst;

                level_moby_groups[i] = g;
                src = (unsigned short *)g;
                dst = (short *)src;
                moby_group_count = i + 1;
                do {
                    int id = *src;
                    short t;

                    tmp = *(short *)src;
                    g = (id & 0x7FFF) << 1; /* the entry's slot in the moby index, in two steps */
                    g += (int)idx;
                    tmp &= 0x8000;
                    t = *(short *)g;
                    if (t >= 0) {
                        *dst++ = t;
                    }
                    if (tmp) {
                        if (level_moby_groups[i] < (int)dst) {
                            dst[-1] |= 0x8000;
                        } else {
                            level_moby_groups[i] = 0;
                        }
                        break;
                    }
                    src++;
                } while (!tmp);
            }
        }
    }
    if (hdr->f4C != 0) {
        char *q = (char *)hdr + hdr->f4C;
        char *base = mem;
        PvarEnt *tbl;
        int size;
        int n;
        int i;

        size = *(int *)q;
        q += 4;
        n = *(int *)q;
        q += 0xC;
        mem += size;
        mem = (char *)(((u32)mem + 0x3F) & 0xFFFFFFC0);
        if (size != 0) {
            FastMemCopy(base, q, size);
        }
        q += size;
        tbl = (PvarEnt *)((char *)hdr + hdr->f54);
        for (i = 0; i < n; i++) {
            int w = *(int *)q;
            int hi = w >> 16;
            int x = tbl[w & 0xFFFF].f0;

            if (x >= (int)hdr) {
                *(int *)(x + hi) = (int)(base + *(int *)(q + 4));
            }
            q += 8;
        }
    }
    if (hdr->f70 != 0) {
        char *q = (char *)hdr + hdr->f70;
        char *src;
        int size;
        int i;

        level_path_count = *(int *)q;
        src = q + *(int *)(q + 4);
        size = *(int *)(q + 8);
        level_path_data = mem;
        mem += size;
        q += 0x10;
        mem = (char *)(((u32)mem + 0x3F) & 0xFFFFFFC0);
        if (size != 0) {
            FastMemCopy(level_path_data, src, size);
        }
        for (i = 0; i < level_path_count; i++) {
            toad_paths[i] = level_path_data + *(int *)q;
            q += 4;
        }
    }
    if (hdr->f74 != 0) {
        char *q = (char *)hdr + hdr->f74;
        char *src;
        char *base;
        int n;
        int size;
        int i;

        n = *(int *)q;
        level_rail_count = n;
        src = q + *(int *)(q + 4);
        size = *(int *)(q + 8);
        level_rails = (Spline *)mem;
        mem += n << 5;
        q += 0x10;
        mem = (char *)(((u32)mem + 0x3F) & 0xFFFFFFC0);
        if (n > 0) {
            Spline *e = level_rails;

            i = n;
            do {
                qcopy(e, q);
                q += 0x10;
                e->f14 = *(int *)q;
                q += 0x10;
                e++;
            } while (--i != 0);
        }
        base = mem;
        mem += size;
        mem = (char *)(((u32)mem + 0x3F) & 0xFFFFFFC0);
        if (size != 0) {
            FastMemCopy(base, src, size);
        }
        for (i = 0; i < level_rail_count; i++) {
            level_rails[i].f10 = base + *(int *)q;
            q += 4;
        }
    }
    if (hdr->f6C != 0) {
        char *q = (char *)hdr + hdr->f6C;
        int n = *(int *)q;
        int i;

        level_clip_volumes = mem;
        level_clip_volume_count = n;
        q += 0x10;
        mem += n * 0x90;
        mem = (char *)(((u32)mem + 0x3F) & 0xFFFFFFC0);
        if (n != 0) {
            FastMemSet(level_clip_volumes, 0, n * 0x90);
        }
        for (i = 0; i < level_clip_volume_count; i++) {
            copy_memory_by_alignment_v(level_clip_volumes + i * 0x90, q, 0x90);
            q += 0x80;
        }
    }
    if (hdr->f60 != 0) {
        char *q = (char *)hdr + hdr->f60;
        int n = *(int *)q;
        int i;

        level_cuboids = mem;
        q += 0x10;
        level_cuboid_count = n;
        mem += n << 7;
        mem = (char *)(((u32)mem + 0x3F) & 0xFFFFFFC0);
        if (n != 0) {
            FastMemSet(level_cuboids, 0, n << 7);
        }
        for (i = 0; i < level_cuboid_count; i++) {
            copy_memory_by_alignment_v(level_cuboids + (i << 7), q, 0x80);
            q += 0x80;
        }
    }
    if (hdr->f64 != 0) {
        char *q = (char *)hdr + hdr->f64;
        int n = *(int *)q;
        int i;

        level_spheres = mem;
        q += 0x10;
        level_sphere_count = n;
        mem += n << 7;
        mem = (char *)(((u32)mem + 0x3F) & 0xFFFFFFC0);
        if (n != 0) {
            FastMemSet(level_spheres, 0, n << 7);
        }
        for (i = 0; i < level_sphere_count; i++) {
            copy_memory_by_alignment_v(level_spheres + (i << 7), q, 0x80);
            q += 0x80;
        }
    }
    if (hdr->f68 != 0) {
        char *q = (char *)hdr + hdr->f68;
        int n = *(int *)q;
        int i;

        level_cylinders = mem;
        q += 0x10;
        level_cylinder_count = n;
        mem += n << 7;
        mem = (char *)(((u32)mem + 0x3F) & 0xFFFFFFC0);
        if (n != 0) {
            FastMemSet(level_cylinders, 0, n << 7);
        }
        for (i = 0; i < level_cylinder_count; i++) {
            copy_memory_by_alignment_v(level_cylinders + (i << 7), q, 0x80);
            q += 0x80;
        }
    }
    if (hdr->f84 != 0) {
        char *q = (char *)hdr + hdr->f84;
        int size = *(int *)q;

        camera_collision_grid = mem;
        q += 0x10;
        size += 0x4000;
        mem += size;
        copy_memory_by_alignment_v(camera_collision_grid, q, size);
        mem = (char *)(((u32)mem + 0x3F) & 0xFFFFFFC0);
    }
    if (hdr->f78 != 0) {
        char *q = (char *)hdr + hdr->f78;
        int size = *(int *)q;

        level_circle_zone_grid = mem;
        q += 0x10;
        size += 0x4000;
        mem += size;
        copy_memory_by_alignment_v(level_circle_zone_grid, q, size);
        mem = (char *)(((u32)mem + 0x3F) & 0xFFFFFFC0);
    }
    if (hdr->f7C != 0) {
        char *q = (char *)hdr + hdr->f7C;
        int n = *(int *)q;

        level_circle_zones = mem;
        q += 0x10;
        level_circle_zone_count = n;
        mem += n << 5;
        mem = (char *)(((u32)mem + 0x3F) & 0xFFFFFFC0);
        if (n != 0) {
            copy_memory_by_alignment_v(level_circle_zones, q, n << 5);
        }
    }
    if (hdr->f88 != 0) {
        char *q = (char *)hdr + hdr->f88;
        int n = *(int *)q;

        environment_samples = mem;
        environment_sample_count = n;
        q += 0x10;
        mem += n * 0x30;
        mem = (char *)(((u32)mem + 0x3F) & 0xFFFFFFC0);
        if (n != 0) {
            copy_memory_by_alignment_v(environment_samples, q, n * 0x30);
        }
    }
    particle_pool = mem;
    mem += 0x20000;
    particle_pool_field_8 = -1;
    particle_pool_field_4 = 0;
    particle_pool_field_c = 0;
    FastMemZero16(particle_slot_bitmap, 0x200);
    level_render_state.f0 = 0;
    if (0.0f < ship[0]) {
        Moby *m;

        if (g_current_level_index == LEVEL_GEMLIK && level_render_state.f26 == 2) {
            if (ship_level_entry_active != 0) {
                for (m = moby_array_start; m->state != 0xFF; m++) {
                    if (m->oclass == first_class_ids[level_render_state.f26]) {
                        initialize_moby_state_record((char *)m);
                        MobySetAnimation(m, 1, 0);
                        break;
                    }
                }
            }
        }
        if (level_render_state.f0 == 0) {
            float rot;

            m = CreateMoby(first_class_ids[level_render_state.f26]);
            m->pos.x = ship[0];
            m->pos.y = ship[1];
            m->pos.z = ship[2];
            rot = ship[3];
            (*(void **)&m->update) = update_moby_proximity_interaction;
            m->draw_dist = 0xFF;
            m->rot.z = rot;
            m->update_dist = 0x10;
            m->flags &= ~2;
            MobySetAnimation(m, 1, 0);
            moby_build_matrix(m);
            level_render_state.f0 = m;
            store_current_level_table_value((int)m, 0);
        }
    }
    if (occlusion_grid != 0 && hdr->f8C != 0) {
        int *q = (int *)((char *)hdr + hdr->f8C);
        int *q0;
        int n1;
        int n2;
        int n3;
        int bad1 = 1;
        int bad2;

        q0 = q;
        n1 = q[0];
        n2 = q[1];
        n3 = q[2];
        q += 4;
        if (tfrag_count == n1) {
            int i;

            bad1 = 0;
            for (i = 0; i < n1; i++) {
                if (tfrag_records[i].f3D != q[1]) {
                    bad1 = 1;
                }
                q += 2;
            }
        } else {
            q += n1 * 2;
        }
        bad2 = 1;
        if (tie_instances_end - level_tie_instances == n2) {
            int i;

            bad2 = 0;
            for (i = 0; i < n2; i++) {
                int id = (unsigned short)q[1];

                if (level_tie_instances[i].f18 != id) {
                    bad2 = 1;
                }
                q += 2;
            }
        } else {
            q += n2 * 2;
        }
        {
            Moby *m;
            int flag = 0;
            int cnt = 0;
            int k = 0;

            for (m = moby_array_start; m->state != 0xFF; m++) {
                if (m->occlusion != 0x7F80) {
                    int uid = (*(short *)&m->save_id);
                    int j;

                    k++;
                    for (j = 0; j < n3; j++) {
                        if (q[j * 2 + 1] == uid) {
                            break;
                        }
                    }
                    if (j == n3) {
                        m->occlusion = 0x7F80;
                        flag = 1;
                        cnt++;
                    } else {
                        int w = q[j * 2];

                        m->occlusion = ((w >> 3) << 8) | (1 << (w & 7));
                    }
                }
            }
            if (flag) {
                STUB_printf(occlusion_moby_mismatch_fmt, cnt, k);
            }
        }
        if (bad1) {
            int i;

            STUB_printf(occlusion_tfrag_mismatch_msg);
            for (i = 0; i < tfrag_count; i++) {
                D_E1_L00_00160F0C[i].f3A = 0x7F80;
            }
        } else {
            int i;

            q = q0 + 4;
            for (i = 0; i < n1; i++) {
                int w = q[0];

                tfrag_records[i].f3A = ((w >> 3) << 8) | (1 << (w & 7));
                q += 2;
            }
        }
        if (bad2) {
            TieInst *t;
            int cnt = 0;
            int k = 0;

            q = &q0[n1 * 2 + 4];
            for (t = level_tie_instances; t != tie_instances_end; t++) {
                int id = t->f18;
                int j;

                k++;
                for (j = 0; j < n2; j++) {
                    if (q[j * 2 + 1] == id) {
                        break;
                    }
                }
                if (j == n2) {
                    t->f18 = 0x7F80;
                    cnt++;
                } else {
                    int w = q[j * 2];

                    t->f18 = ((w >> 3) << 8) | (1 << (w & 7));
                }
            }
            STUB_printf(occlusion_tie_mismatch_fmt, cnt, k);
        } else {
            int i;

            q = &q0[n1 * 2 + 4];
            for (i = 0; i < n2; i++) {
                int w = q[0];

                level_tie_instances[i].f18 = ((w >> 3) << 8) | (1 << (w & 7));
                q += 2;
            }
        }
        gDebugFreeCamLvl[7] = 2;
    } else {
        Moby *m;
        TieInst *t;
        int i;

        STUB_printf(no_occlusion_data_msg);
        for (m = moby_array_start; m != moby_array_last; m++) {
            m->occlusion = 0x7F80;
        }
        for (t = level_tie_instances; t != tie_instances_end; t++) {
            t->f18 = 0x7F80;
        }
        for (i = 0; i < tfrag_count; i++) {
            D_E1_L00_00160F0C[i].f3A = 0x7F80;
        }
        gDebugFreeCamLvl[7] = 0;
    }
    Level_ResetGameplayGlobals();
    initialize_hero();
    reset_hero_to_idle();
    setup_level_cameras();
    Camera_UpdateFrame();
    reset_pad_history();
    enable_font_color_codes();
    active_homing_missiles[0] = 0;
    thrown_mine_list[0] = 0;
    decoy_list[0] = 0;
    level_frame_counter = 0;
    primary_nanotech_ball_moby = 0;
    if (level_render_state.f0 != 0) {
        *(long *)((int *)&level_render_state.f0->spawn_frame) = *(long *)((int *)&g_hero.moby->spawn_frame);
    }
    switch (g_current_level_index) {
        case LEVEL_ARIDIA:
        case LEVEL_RILGAR:
        case LEVEL_BLARG:
        case LEVEL_UMBRIS:
        case LEVEL_BATALIA:
        case LEVEL_ORXON:
            level_render_setup_packet = level_render_setup_packet_alt;
            break;
        default:
            level_render_setup_packet = level_render_setup_packet_default;
            break;
    }
    level_render_setup_packet_copy = level_render_setup_packet;
    return mem;
}

void initialize_pause_menu(void) {
    Menu *m = &(*(Menu *)level_menu_system);
    int *page;
    int queued = 0;
    int s, i, j, n, k, t, u;
    int *p;

    coll_hit_result[4] |= 0x80000000;
    s = m->state;
    m->unk124 = 0;
    m->unkD0 = 0;
    if (s == 10) {
        (*(Menu *)level_menu_system).state = 11;
        page = pause_menu_state_0a_page;
        m->page = page;
        m->next = page;
        queued = 1;
    } else if (s == 14) {
        (*(Menu *)level_menu_system).state = 15;
        page = pause_menu_state_0e_page;
        m->page = page;
        m->next = page;
        queued = 1;
    } else if (s == 0x21) {
        (*(Menu *)level_menu_system).state = 0x22;
        page = (int *)&pause_menu_state_21_page;
        m->page = page;
        m->next = page;
        m->unk124 = 1;
    } else if (s == 0x23) {
        page = pause_menu_state_23_page;
        m->page = page;
        m->next = page;
    } else if (s == 0x2D) {
        page = pause_menu_state_2d_page;
        (*(Menu *)level_menu_system).state = 2;
        m->page = page;
        m->next = page;
    } else {
        page = pause_menu_default_page;
        (*(Menu *)level_menu_system).state = 2;
        m->page = page;
        m->next = page;
    }

    if ((*(Menu *)level_menu_system).queued != 0) {
        int st;
        decode_mode = (*(Menu *)level_menu_system).unk130;
        (*(Menu *)level_menu_system).page = (*(Menu *)level_menu_system).queued;
        (*(Menu *)level_menu_system).next = (*(Menu *)level_menu_system).queued;
        st = (*(Menu *)level_menu_system).queuedState;
        (*(Menu *)level_menu_system).state = st;
        if (st == 11 || st == 15) {
            queued = 1;
            (*(Snd *)map_menu_state).unk224 = (*(Menu *)level_menu_system).unkE4;
        }
        (*(Menu *)level_menu_system).queued = 0;
        (*(Menu *)level_menu_system).queuedState = 0;
    }

    {
        char *r = (char *)level_moby_lights_14_direction;
        FastVecScale(r, &pause_menu_light_direction, 1.0f);
        qcopy(r - 0x10, &pause_menu_light_color);
        r += 0x20;
        qzero(r);
    }
    qzero((char *)level_moby_lights_14_direction + 0x10);
    qcopy(&(*(Menu *)level_menu_system).v40, &g_debug_camera[0]);
    qcopy(&(*(Menu *)level_menu_system).v50, &g_debug_camera[4]);
    {
        float zfar = 524288.0f;
        float fog = 255.0f;
        float scale = 0.63f;
        (*(Menu *)level_menu_system).unkE8 = ((Cam *)render_view_params)->fB0;
        ((Cam *)render_view_params)->fB0 = scale;
        ((Cam *)render_view_params)->f21C = zfar;
        ((Cam *)render_view_params)->f228 = fog;
        ((Cam *)render_view_params)->i22C = 0;
        ((Cam *)render_view_params)->i218 = 0;
    }
    UpdateViewContext();
    reset_active_camera_position_and_basis();
    update_camera_view_matrices();
    VU1_syncChain(1);
    sce_gs_sync_v(0);
    rendered_frame_count = rendered_frame_count + 1;

    if (queued) {
        int a, b, c, d;
        a = lvl_resource_buffers.f4 + 0x30000;
        b = lvl_resource_buffers.f8 + 0x30000;
        c = a + 0x3C000;
        d = b + 0xE0000;
        (*(Menu *)level_menu_system).unkFC = c + 0x11800;
        lvl_vu_chain_buffer_size = 0x30000;
        (*(Menu *)level_menu_system).unk104 = a;
        (*(Menu *)level_menu_system).unk10 = b;
        (*(Menu *)level_menu_system).unk108 = c;
        (*(Menu *)level_menu_system).unk10C = d;
        (*(Menu *)level_menu_system).unk100 = d;
        initialize_menu_stream_buffers(0);
    } else {
        int a, b, c, d, e, f;
        a = lvl_resource_buffers.f4 + 0x60000;
        b = lvl_resource_buffers.f8 + 0x60000;
        c = a + 0x3C000;
        d = b + 0xE0000;
        e = c + 0xC1000;
        f = d + 0x11800;
        (*(Menu *)level_menu_system).unkFC = e;
        lvl_vu_chain_buffer_size = 0x60000;
        (*(Menu *)level_menu_system).unk104 = a;
        (*(Menu *)level_menu_system).unk10 = b;
        (*(Menu *)level_menu_system).unk108 = c;
        (*(Menu *)level_menu_system).unk10C = d;
        (*(Menu *)level_menu_system).unk100 = f;
        initialize_menu_stream_buffers(1);
    }
    level_vu1_chain_reserved_bytes = 0x2000;
    VU1_initChain();

    if ((*(Menu *)level_menu_system).state == 11 || (*(Menu *)level_menu_system).state == 15) {
        initialize_render_archive_transfer();
    }
    apply_group_entry_adjustment(0);
    if ((*(Menu *)level_menu_system).state == 11 || (*(Menu *)level_menu_system).state == 15 ||
        (*(Menu *)level_menu_system).state == 0x12) {
        decompress_and_relocate_selected_groups(1);
    } else {
        decompress_and_relocate_selected_groups(0);
    }
    transfer_draw_buffer_chunks((*(Menu *)level_menu_system).unk10, draw_buffer_address, frame_size << 11);
    t = g_gs_texture_allocation_start;
    (*(Menu *)level_menu_system).unk18 = t;
    if ((*(Menu *)level_menu_system).state == 11 || (*(Menu *)level_menu_system).state == 15) {
        (*(Snd *)map_menu_state).unk240 = t;
        u = t + 0x40000;
        (*(Snd *)map_menu_state).unk244 = u;
        g_gs_texture_allocation_start = u + 0x400;
        for (j = 0; j < 4; j++) {
            int v = g_gs_texture_allocation_start;
            (*(Snd *)map_menu_state).bufs[j] = v;
            g_gs_texture_allocation_start = v + 0x4000;
        }
    } else {
        int z;
        (*(Snd *)map_menu_state).unk240 = 0;
        (*(Snd *)map_menu_state).unk244 = 0;
        p = &(*(Snd *)map_menu_state).bufs[3];
        for (z = 0; z < 4; z++) {
            *p = 0;
            p--;
        }
    }
    select_moby_context(1);
    (*(Menu *)level_menu_system).unkCC = moby_class_headers[0]->seq_count;
    moby_class_headers[0]->seq_count += 0x1C;
    for (n = 0; n < 4; n++) {
        (*(Menu *)level_menu_system).unk30[n] = ((int *)last_weapon_id)[n];
    }

    if ((*(Menu *)level_menu_system).page != 0) {
        for (i = 0; i < 14; i++) {
            Moby_277208 *o = create_menu_preview_moby(0x472);
            ((Moby_277208 **)l00_pause_menu_page_mobys)[i] = o;
            if (o != 0) {
                char *mtx;
                o->flags &= 0xFFFD;
                ((Moby_277208 **)l00_pause_menu_page_mobys)[i]->update = (void *)quad_moby_update;
                mtx = gCameraLvl;
                ((Moby_277208 **)l00_pause_menu_page_mobys)[i]->x = *(float *)(mtx + 0x140);
                ((Moby_277208 **)l00_pause_menu_page_mobys)[i]->y = *(float *)(mtx + 0x144);
                ((Moby_277208 **)l00_pause_menu_page_mobys)[i]->z = *(float *)(mtx + 0x148);
                ((Moby_277208 **)l00_pause_menu_page_mobys)[i]->r40 = 0;
                ((Moby_277208 **)l00_pause_menu_page_mobys)[i]->r44 = 0;
                ((Moby_277208 **)l00_pause_menu_page_mobys)[i]->r48 = 0;
                k = ((struct { int ids[14]; } *)(*(Menu *)level_menu_system).page)->ids[i];
                MobySetAnimation(((Moby_277208 **)l00_pause_menu_page_mobys)[i], k,
                                 ((Moby_277208 **)l00_pause_menu_page_mobys)[i]->cls->anims[k]->b10 - 1);
            }
        }
    }
}

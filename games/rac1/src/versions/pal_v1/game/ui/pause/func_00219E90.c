void initialize_menu_preview_scene(void) {
    s32 i;
    s32 a, b, c, d;
    menu_system_r.state = 2;
    menu_system_r.current = &pause_menu_start_page;
    menu_system_r.next = &pause_menu_start_page;
    menu_system_r.close_locked = 0;
    FastVecScale((s32)menu_light_direction, (s32)&menu_light_direction_init, 1.0f);
    qcopy(menu_light_direction - 0x10, &menu_light_color_init);
    qzero(menu_light_direction + 0x20);
    qzero(D_0019C260_19E90);
    VU1_syncChain(1);
    sce_gs_sync_v(0);
    game_frame_counter_r++;
    a = level_resource_buffers[1] + 0xA0000;
    b = level_resource_buffers[2] + 0xA0000;
    c = a + 0x3C000;
    d = b + 0xE0000;
    menu_system_r.unkFC = c + 0xC1000;
    menu_system_r.unk100 = d + 0x11800;
    current_vu_chain = 0xA0000;
    menu_system_r.unk104 = a;
    menu_system_r.unk10 = b;
    menu_system_r.help_text_buffer = c;
    menu_system_r.unk10C = d;
    func_00226D50_19E90(1);
    VU1_initChain();
    menu_system_r.saved_texture_start = g_gs_texture_allocation_start_r;
    if (menu_system_r.current != 0) {
        for (i = 0; i < 14; i++) {
            struct O2_19E90 *o = (struct O2_19E90 *)create_menu_preview_moby_r(0x472);
            panel_slots_r[i] = o;
            if (o != 0) {
                s32 k;
                o->f34 &= 0xFFFD;
                panel_slots_r[i]->f74 = D_0023B578_19E90;
                panel_slots_r[i]->f10 = gCamera_r2.pos.f[0];
                panel_slots_r[i]->f14 = gCamera_r2.pos.f[1];
                panel_slots_r[i]->f18 = gCamera_r2.pos.f[2];
                panel_slots_r[i]->f40 = 0;
                panel_slots_r[i]->f44 = 0;
                panel_slots_r[i]->f48 = 0;
                k = menu_system_r.current->moby_anims[i];
                MobySetAnimation_r(panel_slots_r[i], k, panel_slots_r[i]->f24->tbl[k]->f10 - 1);
            }
        }
    }
}

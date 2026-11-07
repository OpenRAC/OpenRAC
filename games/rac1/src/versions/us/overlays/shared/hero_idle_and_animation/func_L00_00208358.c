void draw_hero_glow_quads(void) {
    Hero *g;
    int i;
    float f;
    float u;
    if (cinematic_mode == 2 || cinematic_mode == 6) {
        for (i = 0; i < ((Hero *)(hero_globals_base + 0xE1D))->draw_queue_count; i++) {
            draw_offset_textured_effect_quad(&((Hero *)(hero_globals_base + 0xE1D))->draw_queue_vec[i],
                                             ((Hero *)(hero_globals_base + 0xE1D))->draw_queue_val[i],
                                             ((Hero *)(hero_globals_base + 0xE1D))->draw_queue_f0[i],
                                             ((Hero *)(hero_globals_base + 0xE1D))->draw_queue_f1[i]);
        }
        return;
    }
    g = (Hero *)(hero_globals_base + 0xE1D);
    if (g->vehicle_mode == 3) {
        int v = g->unk22E4;
        Vec4 *q = g->unk1D50;
        int k = 2;
        while (k >= 0) {
            draw_offset_textured_effect_quad(q, v, 0.2f, 0.08f);
            q++;
            k--;
        }
        return;
    }
    if (g->vehicle_mode == 0) {
        if ((hero_slot_item_id(3) == 3 || hero_slot_item_id(3) == 2) && g->pack_disabled == 0) {
            u = 0.015f;
            g->unk1D50[3].f[2] = g->unk1D50[3].f[2] + u;
            draw_offset_textured_effect_quad(&g->unk1D50[3], 0x280000C0, 0.057f, 0.08f);
            g->unk1D50[3].f[2] = g->unk1D50[3].f[2] - u;
        }
        for (i = 0; i < ((Hero *)(hero_globals_base + 0xE1D))->draw_queue_count; i++) {
            draw_offset_textured_effect_quad(&((Hero *)(hero_globals_base + 0xE1D))->draw_queue_vec[i],
                                             ((Hero *)(hero_globals_base + 0xE1D))->draw_queue_val[i],
                                             ((Hero *)(hero_globals_base + 0xE1D))->draw_queue_f0[i],
                                             ((Hero *)(hero_globals_base + 0xE1D))->draw_queue_f1[i]);
        }
        return;
    }
    if (g->vehicle_mode == 2) {
        u = 0.07f;
        g->unk1D50[3].f[2] = g->unk1D50[3].f[2] - u;
        draw_offset_textured_effect_quad(&g->unk1D50[3], 0x300000C0, 0.25f, 0.08f);
        g->unk1D50[3].f[2] = g->unk1D50[3].f[2] + u;
        return;
    }
    if (g->vehicle_mode == 1) {
        f = 0.07f;
        if (g->unk22FC != 0)
            f = 0.101500005f;
        if (gCheats[3] != 0) {
            g->unk1D50[3].f[2] = g->unk1D50[3].f[2] - 0.01f;
            f = f * 1.7f;
        } else {
            g->unk1D50[3].f[2] = g->unk1D50[3].f[2] - 0.02f;
        }
        ApproachValue((float *)(hero_globals_base + 0x2449), f, frame_scale * 0.003f);
        {
            Hero *h = (Hero *)(hero_globals_base + 0xE1D);
            draw_offset_textured_effect_quad(&h->unk1D50[3], h->unk2300, h->unk162C, 0.08f);
        }
    }
}

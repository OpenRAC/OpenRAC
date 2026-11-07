int Npc_TryStartDialog(char *p, char *q) {
    extern float fast_vec_dist(void *, void *);
    extern s32 get_dialogue_entry(void *);
    struct Hero *g = &g_hero;
    char *x;
    char *e;
    char *y;
    char *tb;
    unsigned char *tb2;
    int h4;
    float f, f0;
    int r, i, m;
    int *ptr;
    if (g->state.current == 0x1D || g->health.hp == 0)
        return 0;
    if (*(unsigned char *)(q + 9) == 0)
        DialogRequest_Init(p, q);
    if (cinematic_mode != 0)
        return 0;
    if (q == 0)
        return 0;
    if (*(short *)(q + 0x36) == -1)
        return 0;
    if (*(unsigned char *)(q + 8) == 0xFF)
        return 0;
    f0 = fast_vec_dist(p + 0x10, &g->motion.pos);
    f = *(float *)(q + 0xC);
    f = f + f;
    if (f < f0)
        return 0;
    process_menu_entry(p, q, 0);
    if (*(unsigned char *)(q + 8) == 0) {
        f = fast_atan2(g->motion.pos.f[0] - *(float *)(p + 0x10), g->motion.pos.f[1] - *(float *)(p + 0x14));
        f = FastDiffRots(f, *(float *)(p + 0x48));
        if (*(float *)(q + 0xC) < f)
            return 0;
        f = fast_atan2(*(float *)(p + 0x10) - g->motion.pos.f[0], *(float *)(p + 0x14) - g->motion.pos.f[1]);
        f = FastDiffRots(f, g->motion.rot.f[2]);
        if (1.5700000524520874f < f)
            return 0;
    }
    x = active_scene_trigger;
    if (level_frame_counter < *(int *)(x + 0x10))
        return 0;
    if (*(unsigned char *)(q + 8) != 0 || (pad0_pressed & 0x10) != 0) {
        i = get_dialogue_entry(p);
        tb = race_gates_base + 0xB3;
        tb = tb + 0xC;
        ptr = (int *)(tb + (i << 4));
        if (*ptr == 0)
            *ptr = 1;
        *(int *)(x + 8) = (int)p;
        *(int *)(x + 0xC) = (int)q;
        e = (char *)*(int *)(q + 0x3C) + *(short *)(q + 0x36) * 0x1C;
        if (*(unsigned short *)(e + 0x10) & 8) {
            if (*(short *)(e + 8) == 1)
                gBolts = gBolts - *(int *)(item_price_table + *(short *)(e + 0xA) * 0x18);
            else if (*(short *)(e + 8) == 4) {
                tb2 = gSpecialItems;
                tb2[*(short *)(e + 0xA)] = 0;
            }
        }
        if (*(short *)(e + 4) == -1) {
            process_menu_entry(p, q, 1);
            return 1;
        }
        y = (char *)&active_scene_trigger;
        *(int *)(y + 8) = (int)p;
        *(int *)(y + 0xC) = (int)q;
        if (*(unsigned short *)(e + 4) & 0x4000) {
            PlayMovieEntryIfIdle((short)(*(unsigned short *)(e + 4) ^ 0x4000));
            return 1;
        }
        remove_hud_item(bolt_hud_item);
        h4 = *(short *)(e + 4);
        bolt_hud_item = -1;
        start_scene(h4);
        return 1;
    }
    if (hud_keep_visible(bolt_hud_item, 0xA)) {
        bolt_hud_item = queue_animation_update(0xC, 0, (void *)initialize_numeric_hud_element,
                                               (void *)DefaultVtbl_DeleteMoby, (void *)draw_hud_text_panel, 0, 0);
    }
    {
        char *e2 = (char *)*(int *)(q + 0x3C) + *(short *)(q + 0x36) * 0x1C;
        if (*(short *)(e2 + 8) == 1 || *(short *)(e2 + 8) == 6) {
            m = queue_animation_update(2, 0x754E, (void *)initialize_bolt_counter_hud,
                                       (void *)update_bolt_counter_hud_animation_and_visibility,
                                       (void *)draw_bolt_counter_hud_panel, &gBolts, 0x98967F);
            hud_keep_visible(m, scale_ticks(0x3C));
        }
    }
    return 0;
}

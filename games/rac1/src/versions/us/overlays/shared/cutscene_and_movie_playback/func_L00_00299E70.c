typedef struct {
    char pad[0x10];
    float v[4];
    char pad2[0x24];
    short n;
    short x46;
    char pad4[0x130];
    Mob *m[1];
} CTv1;
extern int skip_frame_gp SDATA(skip_frame);
void finish_cutscene(void) {
    Obj *o;
    O5A *h = (O5A *)(cd_read_state - 0);
    char *p, *b;
    float g, f;
    int i;
    if ((unsigned short)(h->x5a - 6) >= 2)
        h->x5a = 5;
    skip_frame_gp = 1;
    select_level_vu_chain();
    FadeToBlack(12);
    render_view_tan_half_fov = 0.63f;
    cinematic_mode = 0;
    gFadeLvl = 0;
    UpdateViewContext();
    for (i = 0; i < cutscene_state_c.n; i++) {
        Mob *m = cutscene_state_c.m[i];
        if (m) {
            m->cls->n--;
            m->cls->x48[m->cls->n] = 0;
            DeleteMoby(m);
        }
    }
    for (o = level_mobys_end; o->st != 0xFF; o++) {
        if (!(o->st & 0x80) && (o->type == 0x4A || o->type == 0xCB))
            o->flags &= 0xFF7F;
    }
    p = &g_hero.motion.pos;
    HeroEnterMoveState(HERO_STATE_IDLE, 1);
    g = probe_ground_height(p, 0, 0.5f);
    if (2.0f < g) {
        p -= 0x80;
        if (absolute_float(*(float *)(p + 0x88) - g) < 4.5f)
            *(float *)(p + 0x88) = g;
    }
    b = (char *)&g_hero;
    b[0x20A5] = 0;
    update_hero_item_moby_flags();
    if (((CTv1 *)&cutscene_state_c)->x46) {
        f = probe_ground_height(cutscene_state_c.v, 0, 0.5f);
        if (2.0f < f && absolute_float(cutscene_state_c.v[2] - f) < 2.0f)
            cutscene_state_c.v[2] = f;
        set_race_camera(cutscene_hero_pos, cutscene_hero_pos + 0x10, 0, 1);
    }
    if (active_scene_trigger.a)
        active_scene_trigger.a->flags &= ~1;
    if (level_menu_system.p) {
        level_menu_system.p->flags &= ~1;
        level_menu_system.p = 0;
    }
    if (active_scene_trigger.a) {
        Fl *a = active_scene_trigger.a;
        void *b = active_scene_trigger.b;
        active_scene_trigger.a = 0;
        active_scene_trigger.b = 0;
        process_menu_entry(a, b, 1);
    }
    music_set_active_track_value(scale_ticks(30));
}

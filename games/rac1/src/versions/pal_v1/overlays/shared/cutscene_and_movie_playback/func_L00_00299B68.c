extern int skip_frame_gp SDATA(skip_frame);
void start_scene(int arg) {
    struct Hero *d;
    Obj *p;
    char *h;
    char *q;
    char *qq;
    int i;
    int v;
    float one;
    if (arg < 0)
        return;
    d = &g_hero;
    if (d->health.hp == 0) {
        hero_respawn();
        return;
    }
    music_Pause(0);
    update_audio_stream_until_idle(0);
    skip_frame_gp = 1;
    FastMemSet(cutscene_state, 0, 0x1C0);
    FastMemSet(extra_scale_manipulator, 0, 0x40);
    FastMemSet(hero_scale_manipulator, 0, 0x40);
    FastMemSet(cheat_scale_manipulator, 0, 0x40);
    FastMemSet(cheat_scale_manipulator_class_555, 0, 0x40);
    FastMemSet(cheat_scale_manipulator_giant_clank, 0, 0x40);
    *(int *)(cutscene_state + 0x30) = arg;
    v = lvl_vu_chain_buffer_size - 0x3C000;
    one = 1.0f;
    big_head_cutscene_fov_scale = one;
    *(int *)((char *)&big_head_cutscene_fov_scale + 4) = 0;
    *(int *)(cutscene_state + 0x34) = 0;
    *(int *)(cutscene_state + 0x3C) = 0;
    lvl_vu_chain_buffer_size = v;
    *(int *)(cutscene_state + 0x58) = lvl_resource_buffers[1] + v;
    *(int *)(cutscene_state + 0x5C) = lvl_resource_buffers[2] + v;
    clear_active_help_record();
    *(float *)&gFadeLvl = one;
    cinematic_mode = 2;
    level_white_fade_level = 0;
    HeroEnterMoveState(0x64, 2);
    *(char *)&d->unk20A5 = 1;
    set_hero_item_moby_flag_bit0();
    if (active_scene_trigger_target != 0)
        *(unsigned short *)(active_scene_trigger_target + 0x34) |= 1;
    if (level_menu_system_target != 0)
        *(unsigned short *)(level_menu_system_target + 0x34) |= 1;
    start_cutscene_chunk_read(0);
    FadeToBlack(scale_ticks(6));
    update_audio_stream_until_idle(1);
    update_world_object_animation();
    for (p = level_mobys_end; p->st != 0xFF; p++) {
        if (p->st & 0x80)
            continue;
        if (p->type == 0x4A || p->type == 0xCB) {
            p->flags |= 0x80;
        }
    }
    h = cd_read_state;
    *(int *)(h + 0x1C) = ((Vd *)cutscene_state)->a30;
    while (qq = cd_read_state, *(short *)(qq + 0x5A) != 3) {
        sound_update();
        sce_gs_sync_v(0);
    }
    q = cutscene_state;
    *(short *)(q + 0x48) = -3;
    ((int (*)(void))continue_audio_stream_if_ready)();
    ((int (*)(int))start_cutscene_chunk_read)(1);
    for (i = 0; *(short *)(q + 0x48) < i; i--) {
        sce_gs_sync_v(0);
    }
}

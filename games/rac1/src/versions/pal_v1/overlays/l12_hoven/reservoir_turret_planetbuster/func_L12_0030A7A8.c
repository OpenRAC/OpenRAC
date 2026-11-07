void clip_volume_scene_trigger_update(char *moby) {
    char *data = ((char *)((Moby *)moby)->pvars);
    char *base;
    char *p;
    char *q;
    if (hoven_frame_count != 0 && hoven_cinematic_mode == 0) {
        base = gSpecialItems;
        if (*(unsigned char *)(((struct TriggerSceneOnceInClipVolumeVars *)data)->unk_04 + base + 0x5C) != 0) {
            DeleteMoby(moby);
            return;
        }
        p = &g_hero.motion.pos;
        if (is_point_inside_clip_volume(p, *(int *)data) != 0) {
            *(char *)(((struct TriggerSceneOnceInClipVolumeVars *)data)->unk_04 + base + 0x5C) = 1;
            qcopy(hoven_cutscene_hero_pos, p);
            q = hoven_cutscene_hero_pos - 0x10;
            *(float *)(q + 0x18) = *(float *)(p + 0x258);
            qcopy(hoven_cutscene_hero_pos + 0x10, p + 0x10);
            *(short *)(q + 0x46) = 1;
            start_scene(((struct TriggerSceneOnceInClipVolumeVars *)data)->unk_08);
        }
    }
}

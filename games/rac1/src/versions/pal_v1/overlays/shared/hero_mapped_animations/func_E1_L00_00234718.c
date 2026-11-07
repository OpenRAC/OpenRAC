void func_E1_L00_00234718(char *obj) {
    float f20 = hero_scale;
    char *s0;
    if (cinematic_mode == 6) {
        if (1.33f < f20)
            f20 = 1.33f;
    }
    s0 = hero_scale_manipulator;
    if (*(unsigned char *)(s0 + 1) == 0) {
        AttachManipulator(obj, 0x18, s0);
        *(float *)(s0 + 0x20) = f20;
        *(float *)(s0 + 0x24) = f20;
        *(float *)(s0 + 0x28) = f20;
    }
    if (g_current_level_index == LEVEL_ARIDIA || g_current_level_index == LEVEL_GASPAR ||
        g_current_level_index == LEVEL_POKITARU || g_current_level_index == LEVEL_GEMLIK) {
        int i;
        char *qq;
        for (i = 0; qq = cutscene_state, i < *(short *)(qq + 0x44); i++) {
            struct Moby *m = (struct Moby *)(((char **)(cutscene_state + 0x178))[i]);
            if (m != 0) {
                int h = m->oclass;
                if (h == 0x1B1 || h == 0x50A || h == 0x509) {
                    m->scale = *(float *)(*(char **)&m->pclass + 0x24) * f20;
                }
            }
        }
    }
    func_L00_002352D0(obj);
}

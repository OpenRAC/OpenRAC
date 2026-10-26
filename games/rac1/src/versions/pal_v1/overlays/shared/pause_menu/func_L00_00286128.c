extern B_00284e50 D_E1_L00_001BB280;
void jet_reset_camera(void *a0, void *a1)
{
    unsigned char *src;
    int i;
    int lvl;
    char *g;

    copy_memory_by_alignment((int)&D_E1_L00_001BB280, &current_moby_states, 0x404);
    lvl = g_current_level_index_m;
    for (src = current_moby_states.f, i = 0; i < 0x7FF; i++, src++) {
        int v = *src;
        if ((unsigned char)v == 0) continue;
        if ((unsigned char)v != 1 && saved_mission_state[lvl][(unsigned char)v - 2] != 0xFF) continue;
        D_E1_L00_001BB280.f[i] = v;
        level_moby_killed[lvl][i >> 5] |= 1 << (i & 0x1F);
        session_moby_killed[i >> 5] |= 1 << (i & 0x1F);
    }
    g = (char *)checkpoint_level_state;
    *(int *)g = 1;
    qcopy(g + 0x10, a0);
    qcopy(g + 0x20, a1);
    {
        char *hero = g_hero;
        char *stat = voice_pool;
        char *fb = cd_read_state;
        char *p = *(char **)(hero + 0x2080);
        signed char v = *(signed char *)(fb + 0x22);

        *(int *)(g + 0x30) = *(int *)((char *)p + 0x38);
        *(int *)(g + 0x34) = *(int *)((char *)p + 0x3C);
        *(int *)(g + 0x38) = *(int *)((char *)p + 0x80);
        *(int *)(g + 0x44) = *(unsigned char *)(hero + 0x20A4);
        *(int *)(g + 0x48) = (int)p;
        *(int *)(g + 0x4C) = *(short *)(hero + 0x22CC);
        *(int *)(g + 0x3C) = *(int *)(stat + 0x64);
        *(unsigned char *)(g + 0x40) = *(unsigned char *)(stat + 0x68);
        *(unsigned char *)(g + 0x41) = *(unsigned char *)(stat + 0x69);
        *(unsigned char *)(g + 0x42) = *(unsigned char *)(stat + 0x6A);
        if (v != -1) {
            *(int *)(g + 0xC54) = v;
        } else {
            *(int *)(g + 0xC54) = *(short *)(fb + 0x38);
        }
    }
}

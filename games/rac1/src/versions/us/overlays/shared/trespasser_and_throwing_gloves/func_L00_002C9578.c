char *spawn_glove_of_doom_held_moby(int a0, char *pos, char *vec) {
    char *m = CreateMoby_m(0xE6);
    struct GloveHeldVars *d;
    struct Hero *g;

    if (m != 0) {
        d = (struct GloveHeldVars *)((Moby *)m)->pvars;
        ((Moby *)m)->update_dist = 0xFF;
        ((Moby *)m)->draw_dist = 0xFF;
        ((struct Moby *)m)->visible = 1;
        ((struct Moby *)m)->state = 0;
        qcopy(&((struct Moby *)m)->pos, pos);
        qcopy(&d->velocity, vec);
        d->owner = a0;
        d->state = 0;
        d->timer = 0;
        ((Moby *)m)->scale = *(float *)(*(char **)&((struct Moby *)m)->pclass + 0x24) * 0.01f;
        g = (struct Hero *)g_hero;
        if (g->unk20A5 != 0 || g->unk20AF != 0) {
            ((Moby *)m)->flags |= 0x41;
        }
    }
    return m;
}

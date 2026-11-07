char *spawn_blarg_turret_child(char *parent) {
    Moby *m = (Moby *)CreateMoby(0x24);
    if (m) {
        struct TurretChildVars *d = (struct TurretChildVars *)m->pvars;
        d->parent = parent;
        d->hit_block = (char *)&d->health;
        d->fade_block = (char *)d->color_fade;
        m->update_dist = parent[0x30];
        *(unsigned short *)&m->draw_dist = *(unsigned short *)(d->parent + 0x32);
        m->visible = 1;
        m->flags |= 0x20;
        m->state = 0;
        m->command = 0;
        qcopy(&m->pos, d->parent + 0x10);
        m->scale = *(float *)(*(char **)&m->pclass + 0x24) * *(float *)(parent + 0x2C) /
                   *(float *)(*(char **)(parent + 0x24) + 0x24);
        d->health = 1.0f;
        d->unk_24 = 1;
        d->unk_28 = 0;
        d->unk_29 = 1;
        d->unk_2A = 3;
        d->unk_30 = 0.9f;
        moby_build_matrix(m);
        copy_hero_moby_field_38(m);
    }
    return (char *)m;
}

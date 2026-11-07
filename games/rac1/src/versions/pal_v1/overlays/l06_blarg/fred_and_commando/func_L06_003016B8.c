void spawn_moby_breakup_effect(char *m, int burst) {
    struct Moby *n;
    float w[4];
    float v[4];
    int i;
    *(int *)(((char *)((Moby *)m)->pvars) + 4) = 0;
    n = (struct Moby *)CreateMoby(0x43C);
    n->draw_dist = 0x40;
    n->visible = 1;
    *(long *)&n->spawn_frame = *(long *)(m + 0x38);
    n->flags = ((Moby *)m)->flags;
    qcopy(&n->pos, m + 0x10);
    qcopy(&n->rot, m + 0x40);
    n->scale = ((Moby *)m)->scale;
    moby_build_matrix(n);
    ((Moby *)m)->flags |= 1;
    {
        for (i = 0; i < 50; i++) {
            float spd = random_float_between(1.0f, 6.0f) * g_frame_time;
            float a = random_angle_radians();
            float c = FastCos(a);
            float s;
            v[0] = c + c;
            s = FastSin(a);
            v[1] = s + s;
            v[2] = 0.0f;
            FastVecAdd(w, m + 0x10, v);
            w[2] += random_float_between(moby_breakup_z_offset_gp + 0.5f, moby_breakup_z_offset_gp + 4.5f);
            vec_scale_to_length(v, v, spd);
            spawn_spinning_moby_fragment(m, w, v);
        }
    }
}

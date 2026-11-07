void apply_hero_reference_tilt(void) {
    unsigned char *g = (unsigned char *)hero_globals_base + 0xE1D;
    struct Hero *h;
    M_c m;
    V_c a, b, c;
    float f20, one = 1.0f, k = 0.8f;
    U128 z = 0;
    f20 = g_frame_scale_sq * 0.3f;
    *(U128 *)&a = z;
    *(U128 *)&b = z;
    *(U128 *)&c = z;
    a.f[2] = one;
    b.f[2] = k;
    c.f[2] = k;
    transform_vector(&b, &b, g);
    *(U128 *)&a = *(U128 *)&((struct Hero *)g)->ground_normal.i[0];
    if (((struct Hero *)g)->gravity_mode == 2)
        FastVecScale(&a, &((struct Hero *)g)->gravity_dir, -1.0f);
    if (((struct Hero *)g)->gravity_mode == 1) {
        if (((struct Hero *)g)->air_frames.s < scale_ticks(10) || ((struct Hero *)g)->ground_distance < one)
            *(U128 *)&a = *(U128 *)&((struct Hero *)g)->ground_normal.i[0];
    }
    h = (struct Hero *)((unsigned char *)hero_globals_base + 0xE1D);
    if (h->air_frames.s != 0 && h->unk1F8 != 0) {
        f20 = g_frame_scale_sq * 0.001f;
    }
    ((void (*)(V_c *, M_c *, float, float, float, void *))compute_hero_reference_tilt_rotation)(&a, &m, f20, f20, 0.0f,
                                                                                                0);
    {
        struct Hero *n = (struct Hero *)((unsigned char *)hero_globals_base + 0xE1D);
        if (n->gravity_mode == 1 && n->air_frames.s != 0) {
            transform_vector(&c, &c, &m);
            FastVecSub(&n->motion.pos, &n->motion.pos, &c);
            FastVecAdd(&n->motion.pos, &n->motion.pos, &b);
        }
    }
}

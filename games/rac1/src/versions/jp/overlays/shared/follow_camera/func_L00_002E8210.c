void follow_camera_setup(Camera *m) {
    CamA *a;
    CamB *b;
    Cam_2E8210 *c;
    Cam_2E8210 *e;
    CamE *f;
    Cam_2E8210 *d;
    CamG *g;
    char *q1;
    char *q2;
    char *q3;
    Cam240_2E8210 *p240;
    int r;
    float t;
    float v0[4];
    float v1[4];

    D_L00_00161E5C_2E8210 = 0;
    D_L00_00161E60_2E8210 = 0;
    a = &((Cam_2E8210 *)m->state_buf)->a;
    qcopy(a->pos, gHeroPos_r);
    a->fB0 = 1.5f;
    a->iC0 = (int)g_hero_r.moby;
    a->bC4 = 0;
    a->bD6 = 0;
    a->fE0 = 0.2f;
    a->fDC = D_L00_00161D70_2E8210;
    a->fE4 = D_L00_00161D70_2E8210;
    a->fE8 = 0.2f;
    a->fB8 = 0.005f;
    a->hC6 = 0;
    a->hD8 = 0;
    a->iB4 = 0;
    a->hDA = 0;
    a->iBC = 0;

    b = &((Cam_2E8210 *)m->state_buf)->b;
    b->f2C = 4.64f;
    b->f48 = 0.003f;
    b->f54 = 0.003f;
    b->f30 = 2.0f;
    b->f58 = 4.64f;
    b->i34 = 0;
    b->i38 = 0;
    b->i28 = 0;
    b->i20 = 0;
    b->i24 = 0;
    b->h3C = 0;
    b->h3E = 0;
    b->i44 = 0;
    b->i50 = 0;

    c = (Cam_2E8210 *)m->state_buf;
    c->h10 = 1;
    c->h12 = 0;
    c->h14 = 0;
    c->h16 = 0;
    follow_cam_turn_blend_weight_r = 0.7f;

    e = (Cam_2E8210 *)m->state_buf;
    e->e.h0 = 0;
    f = &e->e;
    f->i10 = 0;
    f->iC = 0;
    f->h2 = 0;
    f->i14 = 0;
    f->i18 = 0;

    q1 = (char *)m->state_buf;
    ((Cam_2E8210 *)q1)->p190.i0 = 0;
    q1 = (char *)&((Cam_2E8210 *)q1)->p190;
    ((Cam190_2E8210 *)q1)->f8 = 0.005f;
    ((Cam190_2E8210 *)q1)->fC = 0.2f;
    ((Cam190_2E8210 *)q1)->f10 = 0.15707964f;
    ((Cam190_2E8210 *)q1)->i4 = 0;
    ((Cam190_2E8210 *)q1)->f14 = 14.2857f;

    q2 = (char *)m->state_buf;
    ((Cam_2E8210 *)q2)->p1A8.flags = 0;
    q2 = (char *)&((Cam_2E8210 *)q2)->p1A8;
    ((Cam1A8_2E8210 *)q2)->f14 = follow_cam_setting_value_r2;
    ((Cam1A8_2E8210 *)q2)->f18 = 0.5235988f;
    ((Cam1A8_2E8210 *)q2)->i8 = 0;
    ((Cam1A8_2E8210 *)q2)->i4 = 0;
    ((Cam1A8_2E8210 *)q2)->iC = 0;
    ((Cam1A8_2E8210 *)q2)->i10 = 0;
    ((Cam1A8_2E8210 *)q2)->i1C = 0;
    ((Cam1A8_2E8210 *)q2)->i20 = 0;
    ((Cam1A8_2E8210 *)q2)->i24 = 0;

    q3 = (char *)m->state_buf;
    ((Cam_2E8210 *)q3)->p220.i0 = 0;
    q3 = (char *)&((Cam_2E8210 *)q3)->p220;
    ((Cam220_2E8210 *)q3)->i10 = 0;
    ((Cam220_2E8210 *)q3)->h4 = 0;
    ((Cam220_2E8210 *)q3)->h6 = 0;
    ((Cam220_2E8210 *)q3)->f8 = b->f2C;
    ((Cam220_2E8210 *)q3)->fC = D_L00_00161DEC_2E8210;

    d = (Cam_2E8210 *)m->state_buf;
    r = scale_ticks(0x7D0);
    g = &d->g;
    g->f30 = 0.0f;
    g->h3A = 0;
    g->h36 = 0;
    g->h38 = 0;
    g->i4C = 0;
    g->h34 = r;
    g->f44 = D_L00_00161DAC_2E8210;
    g->f3C = D_L00_00161DB4_2E8210;
    g->f40 = D_L00_00161DB0_2E8210;
    g->i48 = gCamSphereMaskBaseLvl_r2;
    clear_u64_value(d->g.v10);

    p240 = &((Cam_2E8210 *)m->state_buf)->p240;
    p240->h28 = 0;
    p240->i24 = 0;
    qcopy(p240->v0, gHeroPos_r);
    qcopy(p240->v10, gHeroPos_r);

    if (m->handoff_state == 2) {
        reset_follow_camera_state_from_position_r(m);
        qcopy(m->type_data, m->basis);
        if (camera_cross_axis_ptr->h86 == 7 && g_hero_r.frames_off_ground.s == 0) {
            e->e.h0 = 0;
        } else {
            f->h0 = scale_ticks(D_L00_00161DF4_2E8210);
        }
    } else {
        follow_camera_reset_r(m);
        rebuild_follow_camera_pose_r(1);
    }
    FastVecSub(b->v0, &m->pos, a->f50);
    qcopy(b->v10, b->v0);
    qcopy(c->pos, &m->pos);
    qcopy(g->v20, &m->pos);
    FastVecScale(v0, hero_gravity_dir_r, FastVecDot(b->v0, hero_gravity_dir_r));
    FastVecSub(v1, b->v0, v0);
    t = b->f2C - FastVecLength(v1);
    g->f30 = t;
    if (t < -0.1f) {
        g->h3A = 1;
    }
    f->f8 = b->f30;
    f->f4 = a->fB0;
    m->transition_state = 0;
}

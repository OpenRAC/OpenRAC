extern float D_E1_L11_00161FAC SDATA(D_E1_L11_00161FAC);
extern float D_E1_L11_00161FB0 SDATA(D_E1_L11_00161FB0);
extern float D_E1_L11_00161F7C SDATA(D_E1_L11_00161F7C);
extern float D_E1_L11_00161F78 SDATA(D_E1_L11_00161F78);
extern float D_E1_L11_00161F80 SDATA(D_E1_L11_00161F80);
extern int D_E1_L11_00161F84 SDATA(D_E1_L11_00161F84);
extern float D_E1_L11_00161F98 SDATA(D_E1_L11_00161F98);
extern float D_E1_L11_00161F94 SDATA(D_E1_L11_00161F94);
extern float D_E1_L11_00161FA0 SDATA(D_E1_L11_00161FA0);
extern float D_E1_L11_00161F9C SDATA(D_E1_L11_00161F9C);
extern float D_E1_L11_00161FA4 SDATA(D_E1_L11_00161FA4);
extern float D_E1_L11_00161FA8 SDATA(D_E1_L11_00161FA8);
extern float D_E1_L11_00161F88_f SDATA(D_E1_L11_00161F88);
extern float D_E1_L11_00161F8C_f SDATA(D_E1_L11_00161F8C);
void update_player_ship_flight(void *a0, void *a1) {
    Moby *moby = a0;
    Jet *o = a1;
    Vec4 start;
    Vec4 back;
    Vec4 up;
    Vec4 ofs;
    Vec4 dir;
    Vec4 rot;
    Vec4 cam;
    Vec4 lag;
    Vec4 want[2];
    Vec4 goal[2];
    M_313D00 mat;
    float pv[2];
    float f8;
    float rate, r;
    int boost = 0x40;

    up.q = 0;
    up.f[2] = g_frame_time_r * 8.0f * D_L11_00162120_313D00;
    back.q = up.q;
    up.q = 0;
    up.f[2] = 1.0f;
    moby->flags &= 0xFEFF;
    if (((PadState *)g_controller_state)->raw_pressed & 0x8000) {
        *(unsigned char *)((char *)o + 0x60) += 1;
        if (*(unsigned char *)((char *)o + 0x60) >= 2)
            *(unsigned char *)((char *)o + 0x60) = 0;
    }
    if (D_L11_00162080_313D00 < o->h68) {
        if (((PadState *)g_controller_state)->raw_held & boost) {
            int per;
            Stats *s = (Stats *)usage_stats;
            if (s->uses <= 0xFFFE)
                s->uses++;
            per = 600;
            if (scale_ticks(gameplay_event_frame_count_r) / per > s->best)
                s->best = scale_ticks(gameplay_event_frame_count_r) / 600;
            s->levels = s->levels | (1 << g_current_level_index_r) | 0x80000000;
            {
                float step = D_E1_L11_00161F7C * D_L11_00162120_313D00;
                float top = D_E1_L11_00161F78 * D_L11_00162120_313D00;
                float v = o->f64 + step;
                o->f64 = v;
                if (top < v)
                    o->f64 = top;
            }
        } else {
            float top = D_L11_00162084_313D00 * D_L11_00162120_313D00;
            float step = D_L11_0016207C_313D00 * D_L11_00162120_313D00;
            float v = o->f64;
            if (v < top - step) {
                o->f64 = v + step;
            } else if (top < v) {
                o->f64 = v + (top - v) * D_E1_L11_00161F80;
            } else {
                o->f64 = top;
            }
        }
    } else {
        o->h68++;
        o->f64 = o->f64 + D_L11_00162078_313D00 * D_L11_00162120_313D00;
    }
    if (D_E1_L11_00161F84 < o->h68) {
        float h20, h21;
        int bits = ((PadState *)g_controller_state)->held;
        float sx = ((Hero *)g_hero)->stick_x;
        float sy = ((Hero *)g_hero)->stick_y;
        h20 = D_L11_00162134_313D00;
        h21 = D_L11_00162144_313D00;
        pv[0] = sx;
        pv[1] = sy;
        if (bits & 0x200) {
            pv[0] = sx / D_E1_L11_00161F98 * D_E1_L11_00161FA0;
            pv[1] = sy / D_E1_L11_00161F94 * D_E1_L11_00161F9C;
            h20 = D_L11_00162138_313D00;
            h21 = D_L11_00162148_313D00;
        }
        stick_313D00(moby, &pv[0], &pv[1]);
        Spring(&o->f6C, &o->f90, pv[0], D_L11_0016212C_313D00, D_L11_00162130_313D00, h20);
        Spring(&o->f70, &o->f94, pv[1], D_L11_0016213C_313D00, D_L11_00162140_313D00, h21);
        spring_angle(&o->f30, o->f6C * D_L11_0016210C_313D00, &o->f98, 0, D_L11_0016214C_313D00, D_L11_00162150_313D00,
                     D_L11_00162154_313D00);
        {
            float p = FastSubRots(o->f34, o->f70 * D_E1_L11_00161F94);
            o->f34 = p;
            if (1.3962634f < p) {
                o->f34 = 1.3962634f;
            } else if (p < -1.3962634f) {
                o->f34 = -1.3962634f;
            }
        }
        o->f38 = FastSubRots(o->f38, o->f6C * D_E1_L11_00161F98);
    }
    spherical_313D00(ofs.f, o->f64, o->f38, -o->f34);
    vec_scale_to_length(&o->f10, ofs.f, 1.0f);
    qcopy(start.f, ((float *)&moby->pos));
    FastVecAdd(((float *)&moby->pos), ((float *)&moby->pos), ofs.f);
    {
        float den = D_E1_L11_00161F78 - D_L11_00162084_313D00;
        float t = o->f64 - D_L11_00162084_313D00;
        if (t < 0.0f) {
            f8 = 0.0f / den;
        } else {
            f8 = t / den;
        }
    }
    goal[0].q = 0;
    goal[1].q = 0;
    o->fC4 = f8;
    goal[0].f[0] =
        ((D_E1_L11_00161FA4 - o->f70 * D_L11_001621B8_313D00) - f8 * D_L11_001621C0_313D00) * D_L11_0016211C_313D00;
    goal[0].f[1] = o->f6C * D_L11_00162110_313D00 * D_L11_0016211C_313D00;
    goal[0].f[2] = (D_E1_L11_00161FA8 + o->f70 * D_L11_001621BC_313D00) * D_L11_0016211C_313D00;
    goal[1].f[0] = D_E1_L11_00161FAC * D_L11_0016211C_313D00;
    goal[1].f[2] = D_E1_L11_00161FB0 * D_L11_0016211C_313D00;
    want[0] = goal[0];
    want[1] = goal[1];
    Spring(&o->fB4, &o->fC8, want[*(unsigned char *)((char *)o + 0x60)].f[0], D_L11_00162194_313D00,
           D_L11_00162198_313D00, D_L11_0016219C_313D00);
    Spring(&o->fB8, &o->fCC, want[*(unsigned char *)((char *)o + 0x60)].f[1], D_L11_001621A0_313D00,
           D_L11_001621A4_313D00, D_L11_001621A8_313D00);
    Spring(&o->fBC, &o->fD0, want[*(unsigned char *)((char *)o + 0x60)].f[2], D_L11_001621AC_313D00,
           D_L11_001621B0_313D00, D_L11_001621B4_313D00);
    lag.f[0] = o->fB4;
    lag.f[1] = o->fB8;
    lag.f[2] = o->fBC;
    lag.f[3] = 0.0f;
    euler_rotation_basis(&mat, &o->f30);
    transform_vector_by_basis(cam.f, lag.f, &mat);
    FastVecAdd(goal[0].f, ((float *)&moby->pos), cam.f);
    set_camera_position(goal[0].f);
    goal[1].f[0] = FastAddRots(o->f30 * D_L11_001620C8_313D00, o->f6C * D_L11_001620C4_313D00);
    goal[1].f[1] = FastSubRots(FastAddRots(o->f34, D_L11_001620CC_313D00), o->f70 * D_E1_L11_00161F94);
    goal[1].f[2] = o->f38;
    spring_angle(&o->f20, goal[1].f[0], &o->fA0, 0, D_L11_00162170_313D00, D_L11_00162174_313D00,
                 D_L11_00162178_313D00);
    spring_angle(&o->f24, goal[1].f[1], &o->fA4, 0, D_L11_0016217C_313D00, D_L11_00162180_313D00,
                 D_L11_00162184_313D00);
    spring_angle(&o->f28, goal[1].f[2], &o->fA8, 0, D_L11_00162188_313D00, D_L11_0016218C_313D00,
                 D_L11_00162190_313D00);
    rot.f[0] = o->f20;
    rot.f[2] = FastSubRots(o->f28, o->fD4);
    rot.f[1] = FastSubRots(o->f24, o->fD8);
    set_camera_rotation(rot.f);
    rate = (((PadState *)g_controller_state)->raw_held & boost) ? D_L11_001621C8_313D00 : D_L11_001621C4_313D00;
    approach_313D00(&o->fB0, &o->fC0, rate, g_frame_time_sq_r + g_frame_time_sq_r,
                    g_frame_time_sq_r + g_frame_time_sq_r, g_frame_time_r + g_frame_time_r);
    {
        float h = o->fB0 * 0.5f;
        float r = FastSin(h) / FastCos(h);
        pokitaru_view_cot_half_fov_r = r;
    }
    UpdateViewContext();
    moby->rot.x = FastAddRots(o->f30, o->f6C * D_L11_001621E0_313D00);
    moby->rot.y = FastAddRots(o->f34, -o->f70 * D_L11_001621E4_313D00);
    moby->rot.z = FastAddRots(o->f38, -o->f6C * D_L11_001621E8_313D00);
    if (coll_sphere(2.0f, ((float *)&moby->pos), 0, moby)) {
        if (jet_fighter_collision_query_result.moby == 0) {
            burst_313D00(moby, pokitaru_zero_vec_r, 0, 8.0f, 100.0f, 30, 10, 24, 16.0f, 8.0f, 9.0f, -1, 1.0f, 50.0f, 1,
                         1, -1, 0);
            moby->visible = 0;
            moby->state = 7;
            moby->flags |= 1;
            o->tF8 = scale_ticks(90);
        } else {
            short t = jet_fighter_collision_query_result.moby->oclass;
            if (t != 0 && t != 0x192 && t != 0x4C2 && t != 0x4C3) {
                float f;
                qcopy(((float *)&moby->pos), jet_fighter_collision_query_result.pos.f);
                reflect_vector(ofs.f, ofs.f, jet_fighter_collision_query_result.dir.f);
                f = FastSubRots(fast_atan2(ofs.f[0], ofs.f[1]), o->f38);
                o->f38 = FastAddRots(o->f38, f * D_E1_L11_00161F88_f);
                f = FastSubRots(-fast_atan2(vector_length_xy(ofs.f), ofs.f[2]), o->f34);
                o->f34 = FastAddRots(o->f34, f * D_E1_L11_00161F8C_f);
                o->f64 = o->f64 * D_L11_001620DC_313D00;
                if (o->tDC == 0) {
                    ((Hero *)g_hero)->ship_hp = ((Hero *)g_hero)->ship_hp - D_L11_001620E0_313D00;
                    if (((Hero *)g_hero)->ship_hp < 0.0f) {
                        FastVecSub(dir.f, jet_fighter_collision_query_result.a.f,
                                   jet_fighter_collision_query_result.b.f);
                        vec_scale_to_length(dir.f, dir.f, 1.0f);
                        reflect_vector(back.f, ofs.f, jet_fighter_collision_query_result.dir.f);
                        vec_scale_to_length(back.f, back.f, g_frame_time_r + g_frame_time_r);
                        vec_scale_to_length(up.f, jet_fighter_collision_query_result.dir.f, 1.0f);
                        burst_313D00(moby, back.f, 0, 8.0f, 100.0f, 30, 10, 24, 16.0f, 8.0f, 9.0f, -1, 1.0f, 50.0f, 1,
                                     1, -1, 0);
                        moby->state = 7;
                    }
                    o->tDC = D_L11_001621DC_313D00;
                }
            }
        }
    }
    if (*(unsigned short *)(help_stats_base_r + 0x5F5) == 0) {
        help_request(0x2AFC, 0x55);
    }
    AddDrawCallback(draw_313D00, moby);
    exhaust_313D00(moby, o);
    r = FastAddRots(o->f28, -o->f6C / 7.0f);
    wings_313D00(moby, o, r, -FastAddRots(o->f24, -(o->f70 / 7.0f) - 0.1f));
}

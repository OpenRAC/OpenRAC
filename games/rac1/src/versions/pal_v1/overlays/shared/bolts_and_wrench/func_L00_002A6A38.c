void vendor_update(char *moby) {
    struct VendorVars *p = (struct VendorVars *)((Moby *)moby)->pvars;
    char *c;
    char *q;
    int i;
    float f;
    float t2;
    float t3;
    struct Moby *m;

    vendor_glow_pulse_phase += 0.05f;
    if (3.14f < vendor_glow_pulse_phase)
        vendor_glow_pulse_phase = -3.14f;
    f = FastSin(vendor_glow_pulse_phase);
    ((Moby *)moby)->glow_color = (((int)(f * 48.0f) + 0x60) * 0x10101) | 0x80000000;

    switch (((Moby *)moby)->state) {
        case 0:
            ((Moby *)moby)->state = 1;
            p->state = 0;
            p->unk_04 = 0;
            i = 0;
            q = (char *)p + 0x10;
            p->ring = CreateMoby(0x477);
            *(unsigned short *)(p->ring + 0x34) |= 2;
            *(short *)(p->ring + 0x32) = 0x40;
            *(float *)(p->ring + 0x10) = ((Moby *)moby)->pos.x;
            *(float *)(p->ring + 0x14) = ((Moby *)moby)->pos.y;
            *(float *)(p->ring + 0x18) = ((Moby *)moby)->pos.z + 2.95f;
            c = p->ring;
            *(unsigned char *)(c + 0x73) = 0x20;
            *(int *)&p->glow = 0;
            for (; i < 2; i++, q += 0x40) {
                AttachManipulator(p->ring, i, q);
            }
            moby_build_matrix(p->ring);
            break;
        case 1:
            if (p->glow > 0.0f)
                p->glow -= 0.1f;
            if (p->glow <= 0.0f)
                *(unsigned short *)(p->ring + 0x34) |= 1;
            if ((level_frame_counter & 7) == 0) {
                char *g = &((struct Hero *)g_hero)->motion.pos;
                if (vec_distance_xy(moby + 0x10, g) <= 16.0f) {
                    if (absolute_float(((Moby *)moby)->pos.z - *(float *)(g + 8)) <= 8.0f) {
                        ((Moby *)moby)->state = 2;
                        if (((Moby *)moby)->prev_seq != 1)
                            MobyBlendAnimation(moby, 1, 0, 10);
                        *(unsigned short *)(p->ring + 0x34) &= 0xFFFE;
                    }
                }
            }
            AddDrawCallback(draw_vendor_effects, moby);
            build_axis_rotation_quaternion(p->ring_a_rot, 2, vendor_ring_a_angle);
            build_axis_rotation_quaternion(p->ring_b_rot, 2, vendor_ring_b_angle);
            vendor_ring_a_angle = FastAddRots(vendor_ring_a_angle, 0.01f);
            t2 = FastAddRots(vendor_ring_b_angle, -0.02f);
            m = (struct Moby *)p->ring;
            t3 = p->glow * *(float *)(*(char **)&m->pclass + 0x24) * 2.5f;
            vendor_ring_b_angle = t2;
            m->scale = t3;
            moby_build_matrix(p->ring);
            break;
        case 2: {
            float dist;
            float dz;
            char *g;
            int a;
            int b;
            int r;
            if (p->glow < 1.0f)
                p->glow += 0.1f;
            g = &((struct Hero *)g_hero)->motion.pos;
            dist = vec_distance_xy(moby + 0x10, g);
            dz = absolute_float(((Moby *)moby)->pos.z - *(float *)(g + 8));
            if ((level_frame_counter & 7) == 0) {
                if (18.0f < dist || 10.0f < dz) {
                    ((Moby *)moby)->state = 1;
                    if (((Moby *)moby)->prev_seq != 0)
                        MobyBlendAnimation(moby, 0, 0, 10);
                    return;
                }
            }
            AddDrawCallback(draw_vendor_effects, moby);
            a = 1;
            b = 1;
            build_axis_rotation_quaternion(p->ring_a_rot, 2, vendor_ring_a_angle);
            build_axis_rotation_quaternion(p->ring_b_rot, 2, vendor_ring_b_angle);
            vendor_ring_a_angle = FastAddRots(vendor_ring_a_angle, 0.01f);
            t2 = FastAddRots(vendor_ring_b_angle, -0.02f);
            m = (struct Moby *)p->ring;
            t3 = p->glow * *(float *)(*(char **)&m->pclass + 0x24) * 2.5f;
            vendor_ring_b_angle = t2;
            m->scale = t3;
            moby_build_matrix(p->ring);
            if (G_pp.i208C != 0 && G_pp.i2084 != 3 && G_pp.i208C != 1) {
                a = 0;
                b = 0;
            }
            if (G_pp.i2084 == 0x1D || G_pp.i2084 == 0x32) {
                a = 0;
                b = 0;
            }
            if (G_pp.b20A4) {
                a = 0;
                b = 0;
            }
            if (((Moby *)moby)->seq != 1)
                a = 0;
            if (4.0f < dist || 2.0f < dz) {
                a = 0;
                b = 0;
            }
            if (FastDiffRots(G_pp.f98, fast_atan2(((Moby *)moby)->pos.x - G_pp.f80, ((Moby *)moby)->pos.y - G_pp.f84)) >
                1.5707964f) {
                a = 0;
                b = 0;
            }
            r = 0;
            if (b != 0)
                r = try_set_help_message(1, 0x53E3) != 0;
            if (*(int *)pad0_pressed & 0x10) {
                if (a != 0 && r != 0) {
                    open_vendor_menu(moby);
                    ((Moby *)moby)->state = 3;
                }
            }
            break;
        }
        case 3:
            m = (struct Moby *)p->ring;
            if (m != 0) {
                *(int *)&p->glow = 0;
                m->scale = *(float *)(*(char **)&m->pclass + 0x24) * p->glow * 2.5f;
                moby_build_matrix(p->ring);
            }
            break;
    }
}

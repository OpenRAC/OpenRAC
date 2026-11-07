void falling_moby_group_update(unsigned char *moby) {
    VData *d = ((VData *)((Moby *)moby)->pvars);
    if (d) {
        float a1 = veldin2_moby_group_accel * g_frame_time_sq;
        float a2 = veldin2_moby_group_decel * g_frame_time_sq;
        float a3 = veldin2_moby_group_max_speed * g_frame_time;
        float b1 = veldin2_moby_group_late_accel * g_frame_time_sq;
        float b2 = veldin2_moby_group_late_decel * g_frame_time_sq;
        float b3 = veldin2_moby_group_late_max_speed * g_frame_time;
        switch (((Moby *)moby)->state) {
            case 0: {
                short **tp;
                short *t;
                int j;
                qcopy(d->v10, moby + 0x10);
                qcopy(d->v20, moby + 0x40);
                tp = veldin2_moby_groups + ((Moby *)moby)->group;
                j = 0;
                t = *tp;
                if (t == 0)
                    return;
                while (j < 16) {
                    int idx = (unsigned short)*t & 0x7FFF;
                    if (*(short *)((idx << 8) + (int)veldin2_moby_table + 0xA6) == 0x630) {
                        d->ent[j].idx = idx;
                        j++;
                    }
                    if (*t++ < 0)
                        break;
                }
                d->f348 = 0;
                d->f34C = scale_ticks(30);
                d->cnt = j;
                capture_linked_moby_relative_transforms(moby, (char *)d + 0x30, j);
                ((Moby *)moby)->state = 1;
                break;
            }
            case 2: {
                int v9;
                int i;
                unsigned char *p;
                if (veldin2_moby_group_move_enabled != 0) {
                    if (d->f330 < 1.5f) {
                        ApproachWithVelocity_v(1.5f, a1, a2, a3, &d->f330, &d->f334);
                    } else {
                        ApproachWithVelocity_v(2.0f, b1, b2, b3, &d->f330, &d->f334);
                    }
                }
                if (d->f330 > veldin2_moby_group_debris_progress && d->f344 == 0.0f) {
                    d->f344 = veldin2_moby_group_debris_start_rate;
                }
                if (d->f330 > veldin2_moby_group_fall_progress && d->i8 != 0) {
                    ((Moby *)moby)->state = 4;
                }
                v9 = 0;
                for (i = 0; i < d->cnt; i++) {
                    unsigned char *e = veldin2_moby_table + (d->ent[i].idx << 8);
                    if (((VG *)(&g_hero))->p2FC == e) {
                        d->f348 = 1;
                        v9 = 1;
                        break;
                    }
                }
                p = ((VG *)(&g_hero))->p2FC;
                if (p != 0) {
                    short s = *(short *)(p + 0xA6);
                    if (s == 0x764)
                        v9 = 1;
                }
                if (d->f348 != 0 && v9 == 0 && ((VG *)(&g_hero))->s30E == 0) {
                    if (fast_dec_timer(&d->f34C))
                        ((Moby *)moby)->state = 4;
                }
                break;
            }
            case 4: {
                float t = g_frame_time_sq * 5.0f;
                ((Moby *)moby)->pos.z += d->f340;
                d->f340 = d->f340 - t;
                if (((Moby *)moby)->pos.z < 20.0f) {
                    int i;
                    for (i = 0; i < d->cnt; i++) {
                        remove_child_carrier_and_child(veldin2_moby_table + (d->ent[i].idx << 8));
                    }
                    DeleteMoby(moby);
                    return;
                }
                break;
            }
        }
        if ((unsigned char)(((Moby *)moby)->state - 1) < 2) {
            lerp_vector(moby + 0x10, d->v10, veldin2_arena_nodes + (d->i4 << 7) + 0x30, d->f330 * 0.5f);
            lerp_vector(moby + 0x40, d->v20, veldin2_arena_nodes + (d->i4 << 7) + 0x70, d->f330 * 0.5f);
        }
        apply_linked_moby_relative_transforms_v(moby, (char *)d + 0x30, d->cnt);
        if (d->f344 != 0.0f) {
            if (((Moby *)moby)->state != 4) {
                float v[4];
                float s, z, f20;
                int n;
                d->f344 = d->f344 + 0.25f;
                if (random_integer_below(truncate_float_to_s32(d->f344)) == 0) {
                    s = random_float_between(-1.0471976f, 1.0471976f);
                    f20 = FastAddRots(s, veldin2_camera_yaw);
                    z = random_float_between(0.0f, 20.0f);
                    v[0] = FastCos(f20) * z;
                    v[1] = FastSin(f20) * z;
                    v[2] = 15.0f;
                    FastVecAdd(v, v, &g_hero.motion.pos);
                    n = scale_ticks(240);
                    spawn_debris_moby_v(v, veldin2_zero_vec, 0x2B8, n, veldin2_moby_group_debris_scale, 2.0f, 1.0f,
                                        0.75f, 0);
                }
            }
        }
    }
}

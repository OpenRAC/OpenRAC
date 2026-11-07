void spawner_1885_update(char *moby) {
    struct SpawnerVars *d = (struct SpawnerVars *)((Moby *)moby)->pvars;
    float v[4];
    float rot[4];
    float pos2[4];
    float arr[4];

    process_spawner_hit_record_r(moby, ((char *)d));
    switch ((unsigned char)moby[0x20]) {
        case 0: {
            int h = 0;
            while (get_first_moby_in_group_r2(&h, d->group, 0, 0) == 0) {
                DeleteMoby((void *)h);
            }
            moby[0x20] = 1;
            return;
        }
        case 1: {
            process_spawner_hit_record_r(moby, ((char *)d));
            if (!FastDecTimer(&d->spawn_timer))
                return;
            if (random_integer_below(7) == 0) {
                char *a6 = 0;
                int a7 = 0;
                char *a8 = 0;
                int a9 = 0;
                if (d->path_a != -1) {
                    a6 = (char *)&d->path_a;
                    a7 = 1;
                }
                if (d->path_b != -1) {
                    char *q = gaspar_paths_r2[d->path_b];
                    a9 = *(int *)q;
                    a8 = q + 0x10;
                }
                FindNearestEligibleMoby_r2(moby, &d->target_pos, d->range, a6, a7, a8, a9);
            } else {
                char *p = d->target;
                if (p != 0 && (unsigned char)p[0x20] != 0xFE && (unsigned char)p[0x20] != 0xFD) {
                    qcopy(&d->target_pos, p + 0x10);
                } else {
                    *(int *)&d->target = 0;
                    d->target_state = 2;
                }
            }
            if (d->target_state == 2)
                return;
            {
                int last = refresh_spawner_population_and_find_free_slot_r((int)moby, ((char *)d));
                char *sel;
                if ((unsigned char)*(char *)&d->max_eggs <= (unsigned char)*(char *)&d->egg_count)
                    return;
                if (last == -1)
                    return;
                sel = 0;
                if (d->nearest_first != 0) {
                    int i;
                    float best = 100000.0f;
                    for (i = 0; i < 4; i++) {
                        int idx = *(int *)(((char *)d) + 0xA0 + i * 4);
                        if (idx != -1) {
                            char *q = D_L09_0016016C_30BF88 + (idx << 7);
                            float f = fast_vec_dist(&g_hero.motion.pos, q + 0x30);
                            if (f < best) {
                                sel = q;
                                best = f;
                            }
                        }
                    }
                    if (sel == 0) {
                        FastVecSub(v, &d->target_pos, moby + 0x10);
                        FastVecScale(v, v, 0.5f);
                        if (3.0f < d->range)
                            clamp_vec_length(v, v, 3.0f);
                        if (FastVecLength(v) < 3.0f)
                            vec_scale_to_length(v, v, 3.0f);
                        FastVecAdd(v, v, moby + 0x10);
                    } else {
                        qcopy(v, sel + 0x30);
                    }
                } else {
                    int nn = random_int_between(0, 3);
                    int i;
                    for (i = 0; i < 4; i++) {
                        int k = (i + nn) % 4;
                        int idx = *(int *)(((char *)d) + (k << 2) + 0xA0);
                        if (idx != -1) {
                            sel = D_L09_0016016C_30BF88 + (idx << 7);
                            break;
                        }
                    }
                    if (sel == 0) {
                        float s = d->range;
                        if (s < 3.0f)
                            s = 3.0f;
                        generate_random_scaled_vector2(v, s);
                        FastVecAdd(v, v, moby + 0x10);
                    } else {
                        qcopy(v, sel + 0x30);
                    }
                }
                if (sel != 0) {
                    FastVecScale(rot, sel, random_float_between(-1.0f, 1.0f));
                    FastVecAdd(v, v, rot);
                    FastVecScale(rot, sel + 0x10, random_float_between(-1.0f, 1.0f));
                    FastVecAdd(v, v, rot);
                }
                {
                    float r = probe_ground_height(v, 0, 0.5f);
                    if (r != 0.0f)
                        v[2] = r;
                }
                FastVecSub(rot, v, moby + 0x10);
                {
                    float len = FastVecLength(rot);
                    float f20 = len * (g_frame_time_r2 * 0.6f);
                    vec_scale_to_length(rot, rot, f20);
                    moby_point_position(moby, 0, pos2);
                    vec_scale_to_length(arr, rot, 0.05f);
                    add_random_offset_to_vector_xyz(arr, 0.025f);
                    FastVecAdd(pos2, pos2, arr);
                    rot[2] = jump_speed_4318(pos2, v, 0, f20, -(g_frame_time_sq_r2 * 9.8f));
                }
                ((Sp *)((char *)d))->slots[last] = spawn_spawner_egg_r(
                    (int)moby, pos2, rot, (unsigned char)*(char *)&d->egg_kind, (unsigned char)last);
                d->spawn_timer = scale_ticks(d->spawn_delay);
                d->egg_count = (char)(*(char *)&d->egg_count + 1);
            }
            return;
        }
        case 2:
            spawn_explosion(moby, d->explosion_pos, 0, 0.0f, 0.0f, 30, 8, 20, 8.0f, 4.0f, 18.0f, 1.0f, 1, 15.0f, 1, 1,
                            -1, 0);
            SpawnBreakFragment(moby, 0x6C5, moby + 0x10, moby + 0x40, 0, 0, 0.0f, gaspar_zero_vec, gaspar_zero_vec,
                               gaspar_zero_vec);
            SpawnBreakFragment(moby, 0x6C6, moby + 0x10, moby + 0x40, 0, 0, 0.0f, gaspar_zero_vec, gaspar_zero_vec,
                               gaspar_zero_vec);
            SpawnBreakFragment(moby, 0x6C7, moby + 0x10, moby + 0x40, 0, 0, 0.0f, gaspar_zero_vec, gaspar_zero_vec,
                               gaspar_zero_vec);
            DeleteMoby(moby);
            return;
        default:
            return;
    }
}

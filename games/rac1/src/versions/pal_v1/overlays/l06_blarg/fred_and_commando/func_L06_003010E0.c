#define breakup_v1(m) (((void (*)(char *))func_L06_003016B8)(m))
void breakable_window_update(char *moby) {
    int *data = ((int *)((Moby *)moby)->pvars);
    float pos[4], rot[4], from[4], target[4], offset[4];
    int base;
    int state;
    ((Moby *)moby)->scale = *(float *)(*(char **)&((struct Moby *)moby)->pclass + 0x24) * D_L06_001620D8_ff;
    switch ((unsigned char)*(char *)&((struct Moby *)moby)->state) {
        case 0:
            data[1] = 1;
            ((struct Moby *)moby)->state = 1;
            break;
        case 1:
            if ((data[2] < 0 || (unsigned char)D_L06_0015FFD8_ffcb0[(data[2] << 8) + 0xbc] == 0) && data[8] != -1) {
                break;
            }
            if (vector_distance_ffcb0(&((struct Moby *)moby)->pos, D_0013F3D0_ffcb0 + 0xE9D) < 32.0f) {
                ((struct Moby *)moby)->state = 2;
                data[9] = scale_frames_ffcb0(30);
            } else {
                breakup_v1(moby);
                ((struct Moby *)moby)->state = 4;
            }
            break;
        case 2:
            if (count_down_ffcb0(data + 9)) {
                {
                    int state;
                    int base;
                    ((struct Moby *)moby)->state = 3;
                    D_L06_0015F404_ff = 1;
                    D_L06_0015F408_ff = 0x12;
                    state = ((struct {
                                int p[5];
                                int s;
                            } *)data)
                                ->s;
                    state <<= 7;
                    base = (int)D_L06_001600EC_ffcb0;
                    qcopy(pos, (void *)(state + base + 0x30));
                    qcopy(rot, (void *)(state + base + 0x70));
                }
                FUN_L00_002eaaa0_ff(pos, rot, 1, scale_frames_ffcb0(300), 0);
                FUN_L06_00312b40_ff(0.001f, 1.0f, 1.0f, 0.001f, 1.0f, 1.0f);
                FUN_L06_00312a78_ff(pos);
                FUN_L06_00312ad0_ff(rot);
                {
                    int base;
                    int state;
                    float facing;
                    {
                        int base;
                        base = data[4];
                        base <<= 7;
                        base += (int)D_L06_001600EC_ffcb0;
                        data[6] = 0;
                        facing = angle_atan2_ffcb0(((Moby *)moby)->pos.x - *(float *)(base + 0x30),
                                                   ((Moby *)moby)->pos.y - *(float *)(base + 0x34));
                    }
                    {
                        int state;
                        state = data[4];
                        state <<= 7;
                        base = (int)D_L06_001600EC_ffcb0;
                        *(float *)(state + base + 0x78) = facing;
                    }
                    state = data[4];
                    state <<= 7;
                    FUN_L00_00216f90_ff((void *)(base + state + 0x30), (void *)(base + state + 0x70), 0x72, 0);
                }
            }
            if (data[9] < D_L06_001620D4_ff)
                D_L06_0015F3FC_ffcb0 = 1.0f - normalize_timer_ffcb0(data[9]) / (float)D_L06_001620D4_ff;
            break;
        case 3:
            if (data[9] < D_L06_001620D4_ff) {
                data[9]++;
                D_L06_0015F3FC_ffcb0 = 1.0f - normalize_timer_ffcb0(data[9]) / (float)D_L06_001620D4_ff;
            }
            if (*(float *)(data + 6) >= 0.99999f) {
                ((struct Moby *)moby)->state = 5;
                hero_set_state_ff(0, 0);
                FUN_L06_00312ce8_ff(2);
                D_L06_0015F404_ff = 0;
            } else {
                if (*(float *)(data + 6) >= 0.05f && data[3] >= 0) {
                    {
                        int state;
                        int base;
                        state = data[3];
                        state <<= 7;
                        base = (int)D_L06_001600EC_ffcb0;
                        qcopy((void *)(state + base + 0x30), D_0013F3D0_ffcb0 + 0xE9D);
                        *(float *)(state + base + 0x38) -= 0.5f;
                    }
                    data[3] = -1;
                }
                if (data[1] == 1 && vector_distance_ffcb0(&((struct Moby *)moby)->pos,
                                                          D_L06_0015FFD8_ffcb0 + (data[8] << 8) + 0x10) > 0.1f) {
                    allocate_voice_for_target_entry_ffcb0(0, 0, moby);
                    breakup_v1(moby);
                }
                approach_two_stage_ffcb0((float *)(data + 6), (float *)(data + 7), 1.0f, frame_time_sq_ff,
                                         frame_time_sq_ff, frame_time_ff * 1.5f);
                {
                    int state;
                    int base;
                    state = data[5];
                    state <<= 7;
                    base = (int)D_L06_001600EC_ffcb0;
                    qcopy(pos, (void *)(state + base + 0x30));
                    qcopy(from, (void *)(state + base + 0x70));
                }
                from[0] = D_L06_001620E0_ff * 0.017453292f;
                qcopy(rot, D_0013F3D0_ffcb0 + 0xE9D);
                qcopy(target, D_0013F3D0_ffcb0 + 0xE9D + 16);
                target[1] = from[1];
                normalize_2fd1a0_ff(offset, D_0013F3D0_ffcb0 + 0xE9D - 0x80, -2.0f);
                vec_add_ffcb0(rot, rot, offset);
                rot[2] += 0.85f;
                lerp_vector_ffcb0(pos, pos, rot, *(float *)(data + 6));
                lerp_vector_ffcb0(from, from, target, *(float *)(data + 6));
                FUN_L00_002ea9d8_ff(pos);
                FUN_L00_002eaa30_ff(from);
            }
            break;
        case 4:
            if (vector_distance_ffcb0(&((struct Moby *)moby)->pos, D_0013F3D0_ffcb0 + 0xE9D) < 16.0f) {
                state = data[3];
                state <<= 7;
                base = (int)D_L06_001600EC_ffcb0;
                qcopy((void *)(state + base + 0x30), D_0013F3D0_ffcb0 + 0xE9D);
                *(float *)(state + base + 0x38) -= 0.5f;
                data[3] = -1;
                ((struct Moby *)moby)->state = 5;
            }
            break;
    }
    if (data[1] != 0) {
        if (data[0] != 0)
            enqueue_callback_list_1_ff((int)FUN_L06_00300720_ff, moby);
        else
            enqueue_callback_list_1_ff((int)FUN_L06_00300958_ff, moby);
    } else {
        if (data[0] != 0)
            enqueue_callback_list_1_ff((int)FUN_L06_003003f0_ff, moby);
        else
            enqueue_callback_list_1_ff((int)FUN_L06_00300588_ff, moby);
    }
}

extern char D_E1_L14_00161FA8[];
void sam_update(char *m) {
    struct SamVars *d = (struct SamVars *)((Moby *)m)->pvars;
    attach_big_head_cheat_manipulator_to_matching_moby(m);
    switch (((Moby *)m)->state) {
        case 0: {
            unsigned short id;
            if (gemlik_available[2] != 0 ||
                oltanis_collected_flags_base_r.collected[(short)(id = ((Moby *)m)->save_id)] != 0 ||
                (*(int *)(help_stats_base + 0xAB75 + (((short)id >> 5) * 4 + (g_current_level_index << 8))) >>
                 (id & 0x1F)) &
                    1 ||
                d->clip_start == -1 || d->clip_end == -1) {
                DeleteMoby(m);
                return;
            }
            d->dialogue_name = D_E1_L14_00161FA8;
            DialogRequest_Init(m, ((char *)d));
            ((Moby *)m)->state = 1;
            d->started = 1;
            break;
        }
        case 1:
            if (d->started == 1 &&
                is_point_inside_clip_volume(&((struct Hero *)((char *)&hero_globals_base + 0xE1D))->motion.pos,
                                            d->clip_volume) != 0) {
                if ((help_stats_base + 0xAA35)[((Moby *)m)->mission + (g_current_level_index << 4)] != 0xFF) {
                    char *t;
                    jet_clear_persistent((unsigned char)*(char *)&((Moby *)m)->mission);
                    t = l14_cuboids + (d->clip_start << 7);
                    jet_reset_camera(t + 0x30, t + 0x70);
                }
                d->talk_range = 255.0f;
            } else {
                d->talk_range = 3.0f;
            }
            if (Npc_TryStartDialog(m, ((char *)d)) != 0) {
                char *g = oltanis_cutscene_state_r;
                char *t = l14_cuboids + (d->clip_end << 7);
                *(short *)(g + 0x46) = 1;
                qcopy(g + 0x10, t + 0x30);
                clear_u64_value(g + 0x20);
                *(float *)(g + 0x28) = *(float *)(t + 0x78);
                ((Moby *)m)->state = 2;
            }
            break;
        case 2:
            if (oltanis_game_mode == 2)
                break;
            ((Moby *)m)->state = 1;
            if (d->dialog_state != 4)
                break;
            unlock_planet(0xF);
            queue_planet_unlock_message(0xF);
            *(int *)(help_stats_base + 0xAB75 +
                     (((short)((Moby *)m)->save_id >> 5) * 4 + (g_current_level_index << 8))) |=
                1 << (((Moby *)m)->save_id & 0x1F);
            oltanis_moby_killed_bits[(short)((Moby *)m)->save_id >> 5] |= 1 << (((Moby *)m)->save_id & 0x1F);
            memcard_Save(0, -1);
            /* fall through */
        case 3:
            DeleteMoby(m);
            return;
    }
    {
        float b[4];
        float sx = 0.02f;
        float sy = 0.3f;
        int doit = 0;
        if (((Moby *)m)->prev_seq == 0) {
            char *g = &((struct Hero *)((char *)&hero_globals_base + 0xE1D))->motion.pos;
            char *a_;
            doit = 1;
            if (vec_distance_xy(&((Moby *)m)->pos, g) < 8.0f &&
                FastDiffRots(((Moby *)m)->rot.z, fast_atan2(*(float *)((a_ = g - 0x80) + 0xD0) - ((Moby *)m)->pos.x,
                                                            *(float *)(a_ + 0xD4) - ((Moby *)m)->pos.y)) < 1.5707964f) {
                if (0.01f < FastVecLength(g + 0x80)) {
                    d->aim_timer = scale_ticks(0x78);
                } else {
                    fast_dec_timer_r(&d->aim_timer);
                }
            } else if (d->aim_timer != 0) {
                d->aim_timer = 0;
                qcopy(&d->aim_target, &((struct Hero *)((char *)&hero_globals_base + 0xE1D))->motion.body_center);
            }
            if (fast_dec_timer_r(&d->aim_retarget_timer) != 0) {
                float y;
                float z;
                float x = random_float_between(180.0f, 300.0f);
                d->aim_retarget_timer = truncate_float_to_s32_r(multiply_global_factor(x));
                y = FastAddRots(((Moby *)m)->rot.z, random_float_between(-90.0f, 90.0f) * 0.017453292f);
                z = random_float_between(0.0f, 30.0f) * 0.017453292f;
                BuildSphericalOffset(&d->aim_target, 6.0f, y, z);
                FastVecAdd(&d->aim_target, &d->aim_target, &((Moby *)m)->pos);
            }
            if (d->aim_timer != 0) {
                qcopy(b, &((struct Hero *)((char *)&hero_globals_base + 0xE1D))->motion.body_center);
                sx = 0.04f;
                sy = 0.3f;
            } else {
                qcopy(b, &d->aim_target);
            }
        }
        if (doit != 0) {
            float w[4];
            float u[4];
            float f20;
            float f2;
            qcopy(w, &((Moby *)m)->pos);
            w[2] = w[2] + 1.0f;
            FastVecSub(u, b, w);
            f20 = FastSubRots(fast_atan2(u[0], u[1]), ((Moby *)m)->rot.z);
            f2 = -fast_atan2(vector_length_xy(u), u[2]);
            if (f20 > 1.5707964f)
                f20 = 1.5707964f;
            else if (f20 < -1.5707964f)
                f20 = -1.5707964f;
            if (f2 > 0.5235988f)
                f2 = 0.5235988f;
            else if (f2 < -0.5235988f)
                f2 = -0.5235988f;
            d->unk_B4 = f2;
            d->unk_B8 = f20 * 0.6f;
            d->unk_138 = f20 * 0.4f;
        }
        if (gCheats[0] != 0)
            d->unk_C0 = 2.75f;
        update_joint_animation(m, d->head_manip, 0, sx * g_frame_scale_sq, sy * g_frame_scale_sq);
        update_joint_animation(m, d->body_manip, 1, sx * g_frame_scale_sq, sy * g_frame_scale_sq);
    }
}

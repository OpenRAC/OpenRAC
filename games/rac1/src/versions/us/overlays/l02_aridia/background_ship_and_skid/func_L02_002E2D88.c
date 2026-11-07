void SkidMcMarxAridiaUpdate(char *m) {
    struct SkidMcMarxVars *d;
    char *p;
    char *pl;
    float rate;
    float head_rate;
    int tracking;
    float s_target[4];
    float s_eye[4];
    float s_delta[4];

    d = (struct SkidMcMarxVars *)((Moby *)m)->pvars;
    attach_big_head_cheat_manipulator_to_matching_moby(m);
    update_skid_mcmarx_relocation_sequence(m);
    switch (((Moby *)m)->state) {
        case 0:
            if (race_gates_base[9] != 0) {
                DeleteMoby(m);
                return;
            }
            d->state = 0;
            d->dialogue_name = skid_mcmarx_dialogue_name;
            ((Moby *)m)->state = 1;
            ((Moby *)m)->update_dist = 0xFF;
            DialogRequest_Init(m, ((char *)d));
            initialize_moby_grounding(m);
            break;
        case 1:
            if (d->unk_08 == 1 && is_point_inside_clip_volume(&g_hero.motion.pos, d->clip_volume) != 0) {
                d->talk_range = 255.0f;
            } else {
                d->talk_range = 2.5f;
            }
            if (Npc_TryStartDialog(m, ((char *)d))) {
                begin_dialogue_camera_transition(2.5f, m);
                ((Moby *)m)->state = 2;
            }
            if (d->dialog_result == 2) {
                help_request(0x7D0, 0xA);
                grant_inventory_item(0x1E, 1);
                jet_clear_persistent(((Moby *)m)->mission);
                jet_reset_camera(&g_hero.motion.pos, &g_hero.motion.rot);
                memcard_Save(0, -1);
                DeleteMoby(m);
                return;
            }
            if (((Moby *)m)->prev_seq == 0 && FastDecTimer((int *)&d->blink_timer)) {
                int mode;
                d->blink_timer = truncate_float_to_s32(multiply_global_factor(random_float_between(1200.0f, 2400.0f)));
                mode = 1;
                if (random_integer_below(2) == 0)
                    mode = 2;
                if (((Moby *)m)->prev_seq != mode)
                    MobyBlendAnimation(m, mode, 0, scale_ticks(10));
            } else if ((((Moby *)m)->anim_flags & 2) && ((Moby *)m)->prev_seq != 0) {
                MobyBlendAnimation(m, 0, 0, scale_ticks(10));
            }
            break;
        case 2: {
            char *w = talk_camera;
            if (*(short *)(w + 0x46) == 0) {
                vec_scale_to_length(w + 0x10, m + 0xC0, 2.5f);
                FastVecAdd(w + 0x10, w + 0x10, m + 0x10);
                *(float *)(w + 0x18) += 10.0f;
                *(float *)(w + 0x18) = probe_ground_height(w + 0x10, 0, 0.5f);
                clear_u64_value(w + 0x20);
                *(float *)(w + 0x28) = FastAddRots(((Moby *)m)->rot.z, 3.14159f);
                *(short *)(w + 0x46) = 1;
            }
            if (aridia_cinematic_mode != 2)
                ((Moby *)m)->state = 1;
        } break;
    }
    rate = 0.02f;
    head_rate = 0.3f;
    tracking = 0;
    if (((Moby *)m)->prev_seq == 0) {
        p = &g_hero.motion.pos;
        tracking = 1;
        if (vec_distance_xy(m + 0x10, p) < 8.0f &&
            FastDiffRots(((Moby *)m)->rot.z, fast_atan2(*(float *)((pl = p - 0x80) + 0xD0) - ((Moby *)m)->pos.x,
                                                        *(float *)(pl + 0xD4) - ((Moby *)m)->pos.y)) < 1.5707964f) {
            if (FastVecLength(p + 0x80) > 0.01f)
                d->notice_timer = scale_ticks(120);
            else
                fast_dec_timer(&d->notice_timer);
        } else if (d->notice_timer != 0) {
            d->notice_timer = 0;
            qcopy(&d->look_target, &g_hero.motion.body_center);
        }
        if (fast_dec_timer(&d->look_timer)) {
            float heading;
            d->look_timer = truncate_float_to_s32(multiply_global_factor(random_float_between(180.0f, 300.0f)));
            heading = FastAddRots(((Moby *)m)->rot.z, random_float_between(-90.0f, 90.0f) * 0.017453292f);
            BuildSphericalOffset(&d->look_target, 6.0f, heading, random_float_between(0.0f, 30.0f) * 0.017453292f);
            FastVecAdd(&d->look_target, &d->look_target, m + 0x10);
        }
        if (d->notice_timer != 0) {
            qcopy(s_target, &g_hero.motion.body_center);
            rate = 0.04f;
            head_rate = 0.3f;
        } else {
            qcopy(s_target, &d->look_target);
        }
    }
    if (tracking) {
        float yaw;
        float pitch;
        qcopy(s_eye, m + 0x10);
        s_eye[2] += 2.0f;
        FastVecSub(s_delta, s_target, s_eye);
        yaw = FastSubRots(fast_atan2(s_delta[0], s_delta[1]), ((Moby *)m)->rot.z);
        pitch = -fast_atan2(vector_length_xy(s_delta), s_delta[2]);
        if (yaw > 1.5707964f)
            yaw = 1.5707964f;
        else if (yaw < -1.5707964f)
            yaw = -1.5707964f;
        if (pitch > 0.5235988f)
            pitch = 0.5235988f;
        else if (pitch < -0.5235988f)
            pitch = -0.5235988f;
        d->body_pitch = pitch;
        d->body_yaw = yaw * 0.6f;
        d->head_yaw = yaw * 0.4f;
    }
    if (gCheats[0] != 0)
        d->unk_B0 = 2.75f;
    EaseManipulatorOffsets_r2(m, d->body_manip, 0, rate * g_frame_scale_sq, head_rate * g_frame_scale_sq);
    EaseManipulatorOffsets_r2(m, d->head_manip, 1, rate * g_frame_scale_sq, head_rate * g_frame_scale_sq);
}

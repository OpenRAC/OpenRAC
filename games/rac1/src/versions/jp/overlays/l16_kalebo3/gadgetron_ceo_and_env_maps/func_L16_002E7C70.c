void gadgetron_ceo_update(Moby *m) {
    L16ChallengeScratch scratch;
    L16ChallengeData *d = (L16ChallengeData *)m->pvars;
    L16ChallengeActivation *activation = (L16ChallengeActivation *)d->activation;
    float rate, head_rate;
    int tracking;
    if (m->visible && FastVecDist(&m->pos, kalebo3_camera_pos) < 28.0f) {
        set_ground_height_band(m);
        m->shadow_range = 22;
    }
    attach_big_head_cheat_manipulator_to_matching_moby(m);
    switch (m->state) {
    case 0:
        m->update_dist = 255;
        m->state = 1;
        DialogRequest_Init(m, activation);
        *(float *)((char *)d + 0x2C) = 3.7f;
        d->initial_yaw = m->rot.z;
        if (*(int *)(race_gates_base + 0x71F) == 0) m->rot.z = FastAddRots(m->rot.z, 3.14159265f);
        restore_gadgetron_ceo_non_challenge_environment((char *)m);
        break;
    case 1: {
        if (((L16ChallengePlayer *)((char *)&hero_globals_base_challenge + 0xE1D))->mode != 15)
            *(float *)kalebo3_hero_fall_state_height_view = 115.0f;
        else
            *(float *)kalebo3_hero_fall_state_height_view = 100.0f;
        if (((L16ChallengePlayer *)((char *)&hero_globals_base_challenge + 0xE1D))->state != 29 && Npc_TryStartDialog(m, (char *)d + 0x20)) {
            m->rot.z = d->initial_yaw;
            PlaceMarkerAheadOfMoby(3.5f, m);
            m->state = 2;
        }
        if (!gHaveMorphORay[9] && vec_distance_xy(hero_globals_base + 0xE9D, &m->pos) < 3.5f) {
            int elapsed, period;
            char *statistics;
            elapsed = scale_ticks(gameplay_event_frame_count);
            period = 600;
            statistics = help_stats_base + 0x34D;
            elapsed -= *(unsigned short *)(statistics + 0x43A) * period;
            if (elapsed > (int)((float)scale_ticks(18) * 60.0f) || *(unsigned short *)(statistics + 0x43A) * period == 0) {
                Help_Request(0x3E83, 0x87);
            } else if (scale_ticks(gameplay_event_frame_count) / 600 > *(unsigned short *)(statistics + 0x43A)) {
                *(unsigned short *)(statistics + 0x43A) = scale_ticks(gameplay_event_frame_count) / 600;
            }
        }
        if (activation->mode == 3) {
            char *camera;
            char *pose;
            int pending;
            pending = kalebo3_frame_count;
            activation->previous = (unsigned short)activation->mode;
            activation->mode = 2;
            activation->pending = pending;
            m->command = 0;
            pose = gadgetron_ceo_hero_and_camera_pose;
            Hero_ResetToPosition(pose, pose + 0x10, 0, 1);
            camera = hero_globals_base + 0x2B1D;
            qcopy(camera, pose + 0x20);
            qcopy(camera + 0x10, pose + 0x30);
            camera[0x3AC] = 1;
            m->state = 3;
            activate_gadgetron_ceo_challenge_environment((char *)m);
        }
        if (((L16ChallengePlayer *)((char *)&hero_globals_base_challenge + 0xE1D))->mode == 22) {
            m->state = 3;
            activate_gadgetron_ceo_challenge_environment((char *)m);
        }
        break;
    }
    case 2:
        if (kalebo3_game_mode != 2) {
            int previous;
            m->state = 1;
            previous = activation->previous;
            if (previous == 4) {
                PlayMovieEntryIfIdle(0);
                m->state = previous;
            }
        }
        break;
    case 4:
        if (kalebo3_game_mode == 0) {
            m->state = 1;
            grant_inventory_item(31, 1);
            memcard_Save(0, -1);
        }
        break;
    case 3:
    default: {
        L16ChallengePlayer *player = (L16ChallengePlayer *)((char *)&hero_globals_base_challenge + 0xE1D);
        unsigned char event;
        if (player->points >= 4500) {
            unsigned char *flags = (unsigned char *)menu_item_enabled_115 + 1;
            if (!flags[0x1A]) {
                flags[0x1A] = 1;
                PlayBankSound(1, 0, 0);
                ShowBanner(0x542D, -1);
            }
            if (race_best_records_challenge[1] < 4500) race_best_records_challenge[1] = player->points;
        }
        if (((L16ChallengePlayer *)((char *)&hero_globals_base_challenge + 0xE1D))->mode == 0) {
            m->state = 1;
            restore_gadgetron_ceo_non_challenge_environment((char *)m);
        }
        event = m->command;
        if (event == 1) {
            m->state = event;
            Hero_ResetToPosition(gadgetron_ceo_hero_return_pose, gadgetron_ceo_hero_return_pose + 0x10, 0, 1);
            m->command = 0;
            if (!race_gates_base[0xA]) {
                int pending;
                unsigned short previous;
                previous = activation->mode;
                pending = kalebo3_frame_count;
                activation->previous = previous;
                activation->mode = 4;
                activation->pending = pending;
                activation->flag = event;
            }
            restore_gadgetron_ceo_non_challenge_environment((char *)m);
        }
        if (m->command == 2) {
            m->state = 1;
            Hero_ResetToPosition(gadgetron_ceo_hero_return_pose, gadgetron_ceo_hero_return_pose + 0x10, 0, 1);
            m->command = 0;
            restore_gadgetron_ceo_non_challenge_environment((char *)m);
        }
        if (m->command == 3) {
            char *camera;
            char *pose = gadgetron_ceo_hero_and_camera_pose;
            m->command = 0;
            Hero_ResetToPosition(pose, pose + 0x10, 0, 1);
            camera = hero_globals_base + 0x2B1D;
            qcopy(camera, pose + 0x20);
            qcopy(camera + 0x10, pose + 0x30);
        }
        break;
    }
    }
    rate = 0.02f;
    head_rate = 0.3f;
    tracking = 0;
    if (m->prev_seq == 0) {
        char *player = (char *)&hero_globals_base_challenge + 0xE9D;
        tracking = 1;
        if (vec_distance_xy(&m->pos, player) < 8.0f &&
            FastDiffRots(m->rot.z,
                fast_atan2(((L16ChallengePlayer *)((char *)&hero_globals_base_challenge + 0xE1D))->aim[0] - m->pos.x,
                    ((L16ChallengePlayer *)((char *)&hero_globals_base_challenge + 0xE1D))->aim[1] - m->pos.y)) < 1.5707964f) {
            if (FastVecLength(player + 0x80) > 0.01f) d->moving_timer = scale_ticks(120);
            else fast_dec_timer(&d->moving_timer);
        } else if (d->moving_timer) {
            d->moving_timer = 0;
            qcopy(d->target, (char *)&hero_globals_base_challenge + 0xEED);
        }
        if (fast_dec_timer(&d->target_timer)) {
            float heading;
            d->target_timer = truncate_float_to_s32(multiply_global_factor(random_float_between(180.0f, 300.0f)));
            heading = FastAddRots(m->rot.z, random_float_between(-90.0f, 90.0f) * 0.017453292f);
            BuildSphericalOffset(d->target, 6.0f, heading, random_float_between(0.0f, 30.0f) * 0.017453292f);
            FastVecAdd(d->target, d->target, &m->pos);
        }
        if (d->moving_timer) {
            qcopy(scratch.target, (char *)&hero_globals_base_challenge + 0xEED);
            rate = 0.04f;
            head_rate = 0.3f;
        } else {
            qcopy(scratch.target, d->target);
        }
    }
    if (tracking) {
        float yaw;
        float pitch;
        qcopy(scratch.eye, &m->pos);
        scratch.eye[2] += 1.0f;
        FastVecSub(scratch.delta, scratch.target, scratch.eye);
        yaw = FastSubRots(fast_atan2(scratch.delta[0], scratch.delta[1]), m->rot.z);
        pitch = -fast_atan2(vec_length_xy(scratch.delta), scratch.delta[2]);
        if (yaw > 1.5707964f) yaw = 1.5707964f;
        else if (yaw < -1.5707964f) yaw = -1.5707964f;
        if (pitch > 0.5235988f) pitch = 0.5235988f;
        else if (pitch < -0.5235988f) pitch = -0.5235988f;
        d->pitch = pitch;
        d->yaw = yaw * 0.7f;
        d->head_yaw = yaw * 0.3f;
    }
    if (*(unsigned char *)&gCheats_challenge) *(float *)((char *)d + 0x150) = 2.75f;
    EaseManipulatorOffsets(rate * g_frame_scale_sq, head_rate * g_frame_scale_sq, m, d->body, 0);
    EaseManipulatorOffsets(rate * g_frame_scale_sq, head_rate * g_frame_scale_sq, m, d->head, 1);
}

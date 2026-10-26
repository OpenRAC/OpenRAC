void update_blarg_elite_commando_damage_and_pursuit(char *m) {
    L16DamageScratch scratch;
    L16DamageData *d = *(L16DamageData **)(m + 0x78);
    char *hit;
    unsigned int response;
    float range;
    float time_scale;
    *(float *)(m + 0x2C) = *(float *)(*(char **)(m + 0x24) + 0x24);
    scratch.amount = 0.0f;
    hit = MobyGetHitMessage(m, 0x330000, 0);
    if (hit) {
        if (*(char **)(hit + 0x20)) {
            char *source = *(char **)(hit + 0x20);
            hit = *(short *)(source + 0xA6) != 0x31 ? hit : 0;
        }
    }
    response = ComputeHitResponse(m, hit, &d->health, 0, &scratch.status, &scratch.amount, 0, 4);
    if (scratch.status != 1 && ((unsigned char *)m)[0x20] != 9) {
        randomize_blarg_elite_commando_group_pursuit_timers(m, 0x12C, 0x168);
        if (hit && *(char **)(hit + 0x20) &&
            *(short *)(*(char **)(hit + 0x20) + 0xA6) != 0x47) {
            d->reward_hits = 0;
        }
        d->health -= scratch.amount;
        if (d->health <= 0.0f) response = 1;
        d->sound_timer = truncate_float_to_s32(512.0f);
        d->impulse_time = g_frame_time_sq * 20.0f;
        d->damping = 0.5f;
        d->vertical_impulse = 0.0f;
        d->active = 1;
        d->pending = 0;
        switch (response) {
        case 9:
        case 10:
            d->animation = 0xFA;
            break;
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8: {
            float impulse = g_frame_time * 6.0f;
            char *source;
            d->blend_start = -1.0f;
            d->blend_end = -1.0f;
            d->forward_impulse = impulse;
            d->lateral_impulse = impulse;
            source = *(char **)(hit + 0x20);
            {
                float heading = fast_atan2(*(float *)(m + 0x10) - *(float *)(source + 0x10),
                *(float *)(m + 0x14) - *(float *)(source + 0x14));
                scratch.vector = *(unsigned long long *)(hit + 0x10);
                scratch.angle = heading;
            }
            HeadingFromVector(&scratch.vector, &scratch.angle, &d->forward_impulse, &d->lateral_impulse);
            InitKnockback(scratch.angle, m, d->knockback, 6, 1, 0);
            m[0x20] = 8;
            d->animation = 0x78;
            break;
        }
        case 1:
        case 2: {
            char *source;
            if (g_current_level_index == 18 && d->reward_hits) {
                ++*(int *)&elite_commando_reward_hit_count;
                if (*(int *)&elite_commando_reward_hit_count >= 10) {
                    if (((L16DamageReward *)(menu_item_enabled_115 + 1))->awarded == 0) {
                        ((L16DamageReward *)(menu_item_enabled_115 + 1))->awarded = 1;
                        PlayBankSound(1, 0, 0);
                        ShowBanner(0x53D6, -1);
                    }
                }
            }
            time_scale = g_frame_time_sq * 20.0f;
            *(unsigned short *)(m + 0x34) &= 0xEFFF;
            d->impulse_time = time_scale;
            if (((L16DamageResident *)((char *)&D_L16DamageState + 0xE1D))->difficulty == 2) {
                float impulse = g_frame_time;
                *(float *)(m + 0x58) = 0.5f;
                d->forward_impulse = impulse * 24.0f;
                d->lateral_impulse = impulse * 8.0f;
            } else {
                float impulse = g_frame_time * 8.0f;
                d->forward_impulse = impulse;
                d->lateral_impulse = impulse;
            }
            d->blend_start = -1.0f;
            d->blend_end = -1.0f;
            source = *(char **)(hit + 0x20);
            {
                float heading = fast_atan2(*(float *)(m + 0x10) - *(float *)(source + 0x10),
                *(float *)(m + 0x14) - *(float *)(source + 0x14));
                scratch.vector = *(unsigned long long *)(hit + 0x10);
                scratch.fatal_angle = heading;
            }
            HeadingFromVector(&scratch.vector, &scratch.fatal_angle, &d->forward_impulse, &d->lateral_impulse);
            InitKnockback(scratch.fatal_angle, m, d->knockback, 7, 1, 0);
            m[0x20] = 9;
            d->animation = 0xF0;
            SetDeathBits(m, 0, -1);
            break;
        }
        case 0:
        case 11:
        default:
            break;
        }
        RerollEffectCounter(m, d->damage);
    }
    ((unsigned char *)m)[0xA4] = 0xFF;
    StepColorFade(m, d->damage);
    fast_dec_timer(&d->pursuit_timer);
    if (d->reset_timer) {
        d->pursuit_timer = truncate_float_to_s32(multiply_global_factor(random_float_between(180.0f, 240.0f)));
    }
    d->reset_timer = 0;
    range = 255.0f;
    if (d->pursuit_timer == 0) range = d->pursuit_range;
    if (((L16DamageResident *)((char *)&D_L16DamageState + 0xE1D))->mode == 22) {
        d->pursuit_mode = 2;
    } else if (d->path_index >= 0 && d->pursuit_timer == 0) {
        char *path = kalebo3_path_table[d->path_index];
        FindNearestEligibleMoby(24.0f, m, d->target, 0, 0, path + 0x10, *(int *)path);
    } else {
        FindNearestTargetMoby(24.0f, m, d->target);
    }
    if (range < vec_distance_xy(m + 0x10, d->target)) d->pursuit_mode = 2;
    if (d->owner == 0) d->owner = ((L16DamageResident *)((char *)&D_L16DamageState + 0xE1D))->owner;
}

void MorphorayUpdate(Moby *m) {
    Vq v0;
    Vq v[3];
    Vq mt[4];
    Ray *p;
    int flag;
    int near;
    Moby *hit;
    int r;

    flag = 0;
    if (((Hero *)g_hero_r)->items[0].state == 3) {
        flag = 1;
    }
    p = (struct Ray *)m->pvars;
    if (m->state == 0) {
        m->state = 1;
        morphoray_light_slot_r = -1;
        morphoray_beam_voice_r = -1;
        morphoray_light_fade_timer_r = 0;
    }
    near = 1;
    if (m->prev_seq == 0 && MobyGetFrameValue(m) < 6.0f) {
        near = 0;
    }
    FastDecTimer(&p->timer);
    if ((((unsigned int)((Hero *)g_hero_r)->state.control_mode < 2 ||
          ((Hero *)g_hero_r)->state.control_mode == HERO_MODE_CROUCH ||
          ((Hero *)g_hero_r)->state.control_mode == HERO_MODE_JUMP ||
          ((Hero *)g_hero_r)->state.control_mode == HERO_MODE_LAND)) &&
        (*(int *)pad_held & ((Hero *)g_hero_r)->items[0].button_mask) != 0 && !flag && near) {
        if (((Hero *)g_hero_r)->item_anim_active == 0) {
            start_equipped_item_animation();
        }
        if (p->seen == 0) {
            p->seen = 1;
            if (((Stats *)item_usage)->kills <= 0xFFFE) {
                ((Stats *)item_usage)->kills++;
            }
            r = scale_ticks(gameplay_event_frame_count_r) / 600;
            if (((Stats *)item_usage)->best < r) {
                ((Stats *)item_usage)->best = scale_ticks(gameplay_event_frame_count_r) / 600;
            }
            ((Stats *)item_usage)->levels = ((Stats *)item_usage)->levels | (1 << g_current_level_index_r) | 0x80000000;
        }
        if (p->timer == 0) {
            p->timer = scale_ticks(15);
        }
        Moby_GetJointWorldPos(m, 0, p);
        if (((Hero *)g_hero_r)->state.current == HERO_STATE_LOOK) {
            HeroEnterMoveState(HERO_STATE_LOOK_RELEASE_WAIT, 1);
        }
        *(u128_2BDAF0 *)&v0 = 0;
        if (((Hero *)g_hero_r)->state.current == HERO_STATE_LOOK_RELEASE_WAIT &&
            (*(unsigned short *)((char *)((Hero *)g_hero_r)->moby + 0x34) & 1)) {
            matrix_copy_rows(v, player_camera_r);
            matrix_from_basis_rows(mt, v);
            v0.z = 1.0f;
            transform_vector(&v0, &v0, mt);
        } else if (((Hero *)g_hero_r)->unk22C8 == 1) {
            FastVecScale(&v0, &m->rot_row1, -1.0f);
        } else {
            v0.x = 1.0f;
            matrix_mul_vec3(&v0, &v0, ((Hero *)g_hero_r)->basis[0].f);
        }
        p->yaw = fast_atan2(v0.x, v0.y);
        p->pitch = fast_atan2(vec_length_xy(&v0), v0.z);
        *(u128_2BDAF0 *)&v[0] = *(u128_2BDAF0 *)p->pos;
        hit = select_morphoray_target_and_adjust_aim_r(m, &v[0], &p->yaw, &p->pitch);
        fast_dec_timer(&p->delay);
        if (p->target != 0) {
            if (p->target->state == 0xFE) {
                p->target = 0;
                p->delay = 0;
            } else if (p->target->state == 0xFD) {
                p->target = 0;
                p->delay = 0;
            }
        }
        if (hit != 0 || p->delay != 0) {
            if (hit != 0) {
                float *s = GetFlaggedPvarWord_r(hit);
                if (hit != p->target && p->delay == 0) {
                    p->target = hit;
                    p->total = IntToFloat(((short *)s)[2]);
                    p->left = s[0];
                    p->delay = scale_ticks(30);
                    if (p->total <= 1.0f) {
                        p->left = 0.0f;
                    }
                    p->snd = queue_animation_update_r(4, 0x7533, hud_init_particles, hud_update_particles,
                                                      draw_uniform_size_hud_stars, &D_L00_001617BC_2BDAF0, 10000);
                }
            }
            {
                float a = IntToFloat(hero_globals_base_r[2]) * 0.33f + 1.0f;
                float d = p->left - D_L00_001617B0_2BDAF0 * g_frame_time_r * a;
                p->left = d;
                if (d < 0.0f) {
                    p->left = 0.0f;
                }
            }
            D_L00_001617BC_2BDAF0 = 10000 - truncate_float_to_s32_r2(p->left * 10000.0f / p->total);
        }
        if (p->target != 0 && p->left <= 0.0f) {
            if (g_current_level_index_r == LEVEL_RILGAR && p->target->oclass == MOBY_CLASS_EXTERMITANK &&
                ((Skill_2BDAF0 *)gSkillPoints_r)->f8 == 0) {
                ((Skill_2BDAF0 *)gSkillPoints_r)->f8 = 1;
                PlayBankSound(1, 0, 0);
                ShowBanner(0x542D, -1);
            }
            spawn_morphoray_chicken(p->target);
            p->target = 0;
        }
        update_beam_ring_rail_and_pulse_effects_r(m);
        if (morphoray_light_slot_r == -1) {
            morphoray_light_slot_r = allocate_packed_color_point_light_slot((float *)&m->pos, 7.5f, 0.0f, 0x207F7F);
        }
        if (morphoray_light_slot_r >= 0) {
            morphoray_light_fade_timer_r = scale_ticks(20);
            vec_scale_to_length(&v[0], &m->rot_row1, -0.5f);
            FastVecAdd(&v[1], p, &v[0]);
            ((Light *)level_point_lights_r)[morphoray_light_slot_r].pos[0] = v[1].x;
            ((Light *)level_point_lights_r)[morphoray_light_slot_r].pos[1] = v[1].y;
            ((Light *)level_point_lights_r)[morphoray_light_slot_r].pos[2] = v[1].z;
            ((Light *)level_point_lights_r)[morphoray_light_slot_r].radius = 7.5f;
        }
        if (Sound_IsPlaying(m, morphoray_beam_voice_r) == 0) {
            morphoray_beam_voice_r = Moby_PlaySound_r(0, 4, m);
        }
    } else {
        p->target = 0;
        p->seen = 0;
        p->delay = 0;
        if (p->snd != -1) {
            remove_hud_item(p->snd);
            p->snd = -1;
        }
        D_L00_0016188C_2BDAF0 = 1;
        if (morphoray_light_slot_r >= 0) {
            float f;
            vec_scale_to_length(&v0, &m->rot_row1, -0.5f);
            FastVecAdd(&v[0], p, &v0);
            ((Light *)level_point_lights_r)[morphoray_light_slot_r].pos[0] = v[0].x;
            ((Light *)level_point_lights_r)[morphoray_light_slot_r].pos[1] = v[0].y;
            ((Light *)level_point_lights_r)[morphoray_light_slot_r].pos[2] = v[0].z;
            ((Light *)level_point_lights_r)[morphoray_light_slot_r].radius = 7.5f;
            f = IntToFloat(morphoray_light_fade_timer_r) * 0.05f;
            ((Light *)level_point_lights_r)[morphoray_light_slot_r].col[0] = f;
            ((Light *)level_point_lights_r)[morphoray_light_slot_r].col[1] = f;
            ((Light *)level_point_lights_r)[morphoray_light_slot_r].col[2] = f * 0.25f;
            if (fast_dec_timer(&morphoray_light_fade_timer_r) != 0) {
                release_point_light_slot(morphoray_light_slot_r);
                morphoray_light_slot_r = -1;
            }
        }
        if (morphoray_light_slot_r >= 0 && morphoray_light_fade_timer_r == 0) {
            release_point_light_slot(morphoray_light_slot_r);
            morphoray_light_slot_r = -1;
        }
        if (morphoray_beam_voice_r >= 0) {
            if (morphoray_beam_voice_r != -1 &&
                ((VoicePoolRow *)(voice_pool_r + morphoray_beam_voice_r * 0x70))->owner == m &&
                ((VoicePoolRow *)(voice_pool_r + morphoray_beam_voice_r * 0x70))->state != 0) {
                Sound_StopSlot(morphoray_beam_voice_r);
            }
            morphoray_beam_voice_r = -1;
        }
        cancel_hero_item_animation();
    }
    if (flag && morphoray_light_slot_r >= 0) {
        release_point_light_slot(morphoray_light_slot_r);
        morphoray_light_slot_r = -1;
    }
}

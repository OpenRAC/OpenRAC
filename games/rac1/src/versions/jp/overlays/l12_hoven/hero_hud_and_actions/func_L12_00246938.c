void HeroL12_UpdateStateTransitions(void) {
    unsigned char *moby;
    int prevSub;
    int prevState;
    int prevAnim;
    float v0[4] __attribute__((aligned(16)));
    float v1[4] __attribute__((aligned(16)));
    int sp20;
    float sp24;
    float sp28;
    float sp2C;
    float sp30;
    int sp34;
    int sp38;

    moby = (unsigned char *)g_hero.moby;
    prevSub = g_hero.state.control_mode;
    prevState = g_hero.state.step;
    prevAnim = moby[0x53];
    if (HeroL01_TryTakeHit(1) == 0) {
        if (g_hero.health.hp <= 0 && g_hero.state.control_mode != HERO_MODE_DYING &&
            (g_hero.state.current == HERO_STATE_IDLE || g_hero.state.current == HERO_STATE_RUN)) {
            HeroL12_SetMoveState(HERO_STATE_DYING, 1);
        } else if (process_hero_item_action_input() == 0 && try_enter_hero_water_state() == 0 &&
                   try_transition_hero_water_state() == 0 && try_start_hero_weapon_fire_state() == 0) {
            if ((g_controller_state.pressed & PAD_TRIANGLE) || (((int)g_controller_state.pressed_unmasked) & 0x10)) {
                if (g_hero.items[0].switch_lock != 2 && item_selector_hud_activation_blocked == 0 &&
                    (g_hero.item_use_locked == 0 || g_hero.vehicle_mode == 1 || g_hero.unk1B2 != 0) &&
                    (g_hero.item_select_blocked == 0 || g_hero.vehicle_mode == 1 || g_hero.unk1B2 != 0) &&
                    (g_hero.vehicle_mode == 0 || g_hero.vehicle_mode == 3)) {
                    int id;

                    if (g_hero.quick_swap_tap_window != 0) {
                        g_hero.item_selector_timer = scale_ticks(0x1B);
                    } else {
                        g_hero.item_selector_timer = scale_ticks(0xF);
                    }
                    queue_animation_update(3, 0, (int)initialize_eight_slot_hud_selector,
                                           (int)update_hud_item_selector_selection_and_visibility,
                                           (int)draw_hud_item_selector_panel, 0, 0);
                    id = show_health_hud();
                    hud_keep_visible(id, scale_ticks(0xB4));
                    id = queue_animation_update(2, 0x754E, (int)initialize_bolt_counter_hud,
                                                (int)update_bolt_counter_hud_animation_and_visibility,
                                                (int)draw_bolt_counter_hud_panel, (int)&gBolts, 0x98967F);
                    hud_keep_visible(id, scale_ticks(0xB4));
                }
            }
            FastDecTimer(&g_hero.quick_swap_tap_timer);
            FastDecTimer(&g_hero.quick_swap_tap_window);
            if (g_controller_state.pressed & PAD_TRIANGLE) {
                g_hero.quick_swap_tap_timer = scale_ticks(0x14);
            }
            if ((g_hero.quick_swap_tap_timer != 0 || g_hero.quick_swap_tap_window != 0) &&
                (g_controller_state.raw_released & 0x10)) {
                if (g_hero.quick_swap_tap_window == 0) {
                    g_hero.quick_swap_tap_window = scale_ticks(0x14);
                } else {
                    int next = quick_swap_item;
                    if (g_hero.items[0].item_id == 8) {
                        next = last_weapon_id;
                    }
                    if (next != 0 && g_hero.items[0].item_id != quick_swap_item) {
                        if (usage_stats.e[31].count < 0xFFFF) {
                            usage_stats.e[31].count++;
                        }
                        if (scale_ticks(gameplay_event_frame_count) / 600 > usage_stats.e[31].best_minutes) {
                            usage_stats.e[31].best_minutes = scale_ticks(gameplay_event_frame_count) / 600;
                        }
                        usage_stats.e[31].level_mask =
                            usage_stats.e[31].level_mask | (1 << g_current_level_index) | 0x80000000;
                        g_hero.pending_item[0] = next;
                        g_hero.quick_swap_tap_timer = 0;
                        g_hero.quick_swap_tap_window = 0;
                    }
                }
            }
            if (hoven_hero_fall_state_height > g_hero.motion.pos.f[2] && g_hero.ground_distance > 2.0f &&
                g_hero.vehicle_mode != 2 && g_hero.state.current != HERO_STATE_FATAL_FALL) {
                HeroL12_SetMoveState(HERO_STATE_FATAL_FALL, 1);
            } else {
                switch (g_hero.state.current) {
                    case HERO_STATE_IDLE:
                        if (g_controller_state.held & (PAD_L2 | PAD_L1)) {
                            HeroL12_SetMoveState(HERO_STATE_LOOK, 1);
                            break;
                        }
                        clear_hero_queued_animation();
                        if (g_hero.unk22CA != 0) {
                            if (g_hero.unk308 == 0 || item_available[0x1C] == 0) {
                                HeroL12_SetMoveState(HERO_STATE_DELAYED_JUMP_OR_R1_REPEAT_EXIT_WAIT, 1);
                                break;
                            }
                        }
                        {
                            int flag;
                            unsigned char *m;
                            int r;

                            flag = 1;
                            if (hero_slot_item_id(0) == 0x1B) {
                                if (((unsigned char *)g_hero.moby)[0x53] != hero_get_gadget_anim_id(0)) {
                                    set_hero_animation(hero_get_gadget_anim_id(0), 0, -2.0f);
                                }
                                flag = 0;
                            }
                            if (g_hero.on_ice_ground != 0) {
                                flag = 0;
                            }
                            if (g_hero.items[3].state == 2) {
                                m = (unsigned char *)g_hero.items[3].moby;
                                if (m != 0 && (m[0x70] & 2) && m[0x53] != 1) {
                                    MobyBlendAnimation(m, 1, 0, scale_ticks(7));
                                }
                                m = (unsigned char *)g_hero.items[3].moby2;
                                if (m != 0 && (m[0x70] & 2) && m[0x53] != 1) {
                                    MobyBlendAnimation(m, 1, 0, scale_ticks(7));
                                }
                            }
                            if (0.61086524f < hoven_hero_bones.f258 || hoven_hero_bones.f254 > 0.43633232f) {
                                m = (unsigned char *)g_hero.items[3].moby;
                                if (m != 0 && m[0x53] != 1) {
                                    MobyBlendAnimation(m, 1, 0, scale_ticks(0xC));
                                }
                                m = (unsigned char *)g_hero.items[3].moby2;
                                if (m != 0 && m[0x53] != 1) {
                                    MobyBlendAnimation(m, 1, 0, scale_ticks(0xC));
                                }
                            }
                            r = return_hero_animation_id_unchanged(moby[0x53]);
                            if (r == hero_get_gadget_anim_id(0) && flag) {
                                if (pick_idle_fidget(&sp20)) {
                                    HeroIdleFidget *rec;

                                    g_hero.fidget_index = sp20;
                                    rec = &l12_idle_fidget_entries[sp20];
                                    g_hero.fidget_cooldown = scale_ticks(0x78);
                                    rec->f60 = multiply_global_factor(rec->f5C) * 60.0f;
                                    if (rec->f44 != 0) {
                                        HeroL12_SetMoveState(HERO_STATE_FIDGET, 1);
                                    } else {
                                        set_hero_animation(rec->f48, 0, scale_ticks(0xC));
                                    }
                                }
                            } else if (g_hero.anim_flags & 2) {
                                int keep = 0;
                                if (g_hero.item_anim_active != 0 && g_hero.unk20AA != 0) {
                                    if (((unsigned char *)g_hero.moby)[0x53] == g_hero.item_hold_anim ||
                                        ((int)g_hero.held_item_anim_blend[0]) != 0) {
                                        keep = 1;
                                    }
                                }
                                if (!keep) {
                                    g_hero.item_anim_active = 0;
                                    if (hero_get_gadget_anim_id(0) == 0x54) {
                                        set_hero_animation(hero_get_gadget_anim_id(0), 0, scale_ticks(0x12));
                                    } else {
                                        set_hero_animation(hero_get_gadget_anim_id(0), 0, -2.0f);
                                    }
                                }
                            }
                        }
                        if (scale_ticks(0xA) < g_hero.state_timer) {
                            if (((unsigned char *)g_hero.items[3].moby2) != 0 &&
                                ((unsigned char *)g_hero.items[3].moby2)[0x53] == 1) {
                                if (fast_dec_timer(&g_hero.slot3_anim_timer)) {
                                    if (0.55850536f > hoven_hero_bones.f258 && hoven_hero_bones.f254 < 0.34906584f &&
                                        g_hero.items[3].state == 2) {
                                        g_hero.slot3_anim_timer =
                                            random_int_between(scale_ticks(0x6E), scale_ticks(0x10E));
                                        blend_hero_slot3_animation_pair();
                                    }
                                }
                            }
                        }
                        if (g_hero.item_anim_active != 0 && g_hero.anim_changing == 0 && g_hero.unk20AA != 0 &&
                            g_hero.unk22C8 == 0) {
                            int id = hero_slot_item_id(0);
                            if (id != g_hero.item_anim_item_id) {
                                cancel_hero_item_animation();
                            } else {
                                int anim = hoven_item_defs[id].f24;
                                if (((unsigned char *)g_hero.moby)[0x53] != anim) {
                                    if (((int)g_hero.held_item_anim_blend[0]) != 0) {
                                        g_hero.unkD14 = 1;
                                    }
                                    g_hero.item_hold_anim = anim;
                                    set_hero_animation(g_hero.item_hold_anim, 0, scale_ticks(0xB));
                                }
                            }
                        }
                        stick_target_v(0, 1.0f);
                        if (g_hero.on_slip_ground != 0 && g_hero.frames_off_ground.s == 0) {
                            HeroL12_SetMoveState(HERO_STATE_SLIP, 1);
                            break;
                        }
                        if (g_hero.air_frames.s > scale_ticks(4)) {
                            if (g_hero.ground_distance > 0.4f) {
                                if (g_hero.state.prev != HERO_STATE_LAND) {
                                    HeroL12_SetMoveState(HERO_STATE_FALL, 1);
                                    break;
                                }
                                if (scale_ticks(0xF) > g_hero.state_timer) {
                                    break;
                                }
                                HeroL12_SetMoveState(HERO_STATE_FALL, 1);
                                break;
                            }
                        }
                        if (g_hero.move_lock_timer != 0) {
                            break;
                        }
                        {
                            int ok = 1;
                            if (g_hero.state.prev == HERO_STATE_LOOK ||
                                g_hero.state.prev == HERO_STATE_LOOK_RELEASE_WAIT) {
                                if (scale_ticks(5) > g_hero.state_timer) {
                                    ok = 0;
                                }
                            }
                            if (ok) {
                                if (g_hero.unk308 != 1) {
                                    int t = scale_ticks(7);
                                    if (g_hero.jump_buffer_lockout != 0) {
                                        t = 1;
                                    }
                                    if (try_start_hero_jump(t, 0)) {
                                        break;
                                    }
                                }
                            }
                        }
                        if (g_hero.unk308 == 1) {
                            if (pad_pressed_within(0x40, scale_ticks(7), 0)) {
                                HeroL12_SetMoveState(HERO_STATE_ANIMATION_FRAME_EXIT_WAIT, 1);
                                break;
                            }
                        }
                        if ((g_controller_state.held & (PAD_R2 | PAD_R1)) && g_hero.grounded_frames != 0) {
                            if (((unsigned char *)g_hero.moby)[0x53] != 0x1A || !(g_hero.anim_frame < 108.0f)) {
                                HeroL12_SetMoveState(HERO_STATE_CROUCH, 1);
                                break;
                            }
                        }
                        if (g_hero.on_ice_ground != 0 && g_hero.ground_slope.f > 0.08726646f) {
                            HeroL12_SetMoveState(HERO_STATE_RUN_CLEAR_VELOCITY_Z, 1);
                            break;
                        }
                        {
                            int ok = 1;
                            if (g_hero.state.prev == HERO_STATE_LOOK ||
                                g_hero.state.prev == HERO_STATE_LOOK_RELEASE_WAIT) {
                                if (scale_ticks(0x18) > g_hero.state_timer && g_hero.unk308 == 0) {
                                    stick_target_v(0, 1.0f);
                                    if (FastDiffRots(g_hero.motion.move_yaw, hoven_camera_yaw) > 1.2217305f) {
                                        ok = 0;
                                    }
                                }
                            }
                            if (ok && 0.22f < g_hero.stick_magnitude) {
                                if (g_current_level_index == LEVEL_HOVEN) {
                                    int slot = g_hero.ground_sound_class;
                                    emit_hero_animation_voice_and_ground_particle(slot, 0, 0);
                                    emit_hero_animation_voice_and_ground_particle(slot, 1, 0);
                                }
                                HeroL12_SetMoveState(HERO_STATE_RUN, 1);
                            }
                        }
                        break;
                    case HERO_STATE_FIDGET:
                        if (g_controller_state.held & (PAD_L2 | PAD_L1)) {
                            HeroL12_SetMoveState(HERO_STATE_LOOK, 1);
                            break;
                        }
                        if (g_hero.air_frames.s != 0) {
                            HeroL12_SetMoveState(HERO_STATE_FIDGET_ALT, 1);
                            break;
                        }
                        if (g_controller_state.held & PAD_CROSS) {
                            HeroL12_SetMoveState(HERO_STATE_FIDGET_ALT, 1);
                            break;
                        }
                        if (g_controller_state.held & (PAD_R2 | PAD_R1)) {
                            HeroL12_SetMoveState(HERO_STATE_FIDGET_ALT, 1);
                            break;
                        }
                        update_hero_target_speed_from_stick();
                        if (g_hero.target_speed > 0.0f) {
                            HeroL12_SetMoveState(HERO_STATE_FIDGET_ALT, 1);
                            break;
                        }
                        if (g_hero.anim_flags & 2) {
                            remove_hero_cached_moby_and_clear_slot_three_moby_flags();
                            HeroL12_SetMoveState(HERO_STATE_IDLE, 1);
                        }
                        break;
                    case HERO_STATE_FIDGET_ALT:
                        if (scale_ticks(8) < g_hero.state_timer) {
                            remove_hero_cached_moby_and_clear_slot_three_moby_flags();
                            if (g_hero.air_frames.s != 0) {
                                HeroL12_SetMoveState(HERO_STATE_FALL, 1);
                                break;
                            }
                            if (try_start_hero_jump(scale_ticks(7), 0)) {
                                break;
                            }
                            if (g_controller_state.held & (PAD_R2 | PAD_R1)) {
                                HeroL12_SetMoveState(HERO_STATE_CROUCH, 1);
                                break;
                            }
                            update_hero_target_speed_from_stick();
                            if (g_hero.target_speed > 0.0f) {
                                HeroL12_SetMoveState(HERO_STATE_RUN, 1);
                                break;
                            }
                            HeroL12_SetMoveState(HERO_STATE_IDLE, 1);
                        }
                        break;
                    case HERO_STATE_DYING:
                    case HERO_STATE_VEHICLE_MODE_1_DYING:
                    case HERO_STATE_VEHICLE_MODE_3_DYING:
                    case 98:
                    case HERO_STATE_DROWN:
                    case HERO_STATE_DYING_SPEED_LIMITED:
                    case HERO_STATE_DYING_ZERO_HEALTH:
                        if (g_hero.anim_flags & 2) {
                            hero_respawn();
                            return;
                        }
                        break;
                    case HERO_STATE_SINK:
                        if (g_hero.state_timer > scale_ticks(0xDC)) {
                            hero_respawn();
                            return;
                        }
                        break;
                    case HERO_STATE_SWIM:
                    case 53:
                        if (g_hero.state.current == HERO_STATE_SWIM) {
                            g_hero.anim_speed = g_hero.ground_speed * 15.0f;
                            if (g_hero.anim_speed < 0.25f) {
                                g_hero.anim_speed = 0.25f;
                            }
                            if (g_hero.anim_speed > 0.75f) {
                                g_hero.anim_speed = 0.75f;
                            }
                        }
                        if (g_hero.state.current == HERO_STATE_SWIM) {
                            float lim = g_frame_time * 2.0f;
                            if (!(g_controller_state.held & (PAD_CROSS | PAD_SQUARE)) && g_hero.anim_frame > 9.0f &&
                                g_hero.anim_frame < 19.0f && g_hero.state.step != 1 && g_hero.stick_magnitude < 0.1f &&
                                g_hero.ground_speed < lim) {
                                HeroL12_SetMoveState(HERO_STATE_SWIM_IDLE, 1);
                                break;
                            }
                        }
                        if (item_available[4] != 0 && (g_controller_state.held & (PAD_R2 | PAD_R1))) {
                            if (g_hero.state.current == HERO_STATE_SWIM && scale_ticks(0x14) < g_hero.state_timer) {
                                HeroL12_SetMoveState(0x35, 1);
                                break;
                            }
                        } else if (g_hero.state.current == 0x35 && g_hero.stick_magnitude > 0.5f) {
                            HeroL12_SetMoveState(HERO_STATE_SWIM, 1);
                            break;
                        }
                        if (g_hero.state.current == 0x35 && !(g_controller_state.held & (PAD_CROSS | PAD_SQUARE)) &&
                            g_hero.state.step != 1 && g_hero.stick_magnitude < 0.7f &&
                            !(g_controller_state.held & (PAD_R2 | PAD_R1))) {
                            HeroL12_SetMoveState(HERO_STATE_SWIM_IDLE, 1);
                        }
                        break;
                    case HERO_STATE_SWIM_IDLE:
                        if (g_hero.anim_flags & 2) {
                            if (((unsigned char *)g_hero.moby)[0x53] != 0x3C) {
                                set_hero_animation(0x3C, 0, scale_ticks(0xB));
                            }
                        }
                        if (0.2f < g_hero.stick_magnitude || (g_controller_state.held & (PAD_CROSS | PAD_SQUARE)) ||
                            (item_available[4] != 0 && (g_controller_state.held & (PAD_R2 | PAD_R1)))) {
                            if (item_available[4] != 0 && (g_controller_state.held & (PAD_R2 | PAD_R1))) {
                                HeroL12_SetMoveState(0x35, 1);
                                break;
                            }
                            HeroL12_SetMoveState(HERO_STATE_SWIM, 1);
                        }
                        break;
                    case HERO_STATE_WATER_SURFACE_ENTRY:
                        if (g_hero.jump_lockout_timer == 0) {
                            if (pad_pressed_within(0x40, scale_ticks(8), 0)) {
                                if (scale_ticks(0xA) < g_hero.state_timer) {
                                    HeroL12_SetMoveState(0x12, 1);
                                    break;
                                }
                            }
                        }
                        g_hero.item_select_blocked = 1;
                        if (g_hero.unk1DE == 0) {
                            if (pad_pressed_within(0x80, scale_ticks(7), 0) ||
                                (item_available[4] != 0 && pad_pressed_within(0xA, scale_ticks(7), 0))) {
                                if (item_available[4] != 0 && (g_controller_state.held & (PAD_R2 | PAD_R1))) {
                                    HeroL12_SetMoveState(0x35, 1);
                                    break;
                                }
                                HeroL12_SetMoveState(HERO_STATE_SWIM, 1);
                                break;
                            }
                        }
                        if (g_hero.wading != 0 && g_hero.water_depth < 0.4f) {
                            if (HeroL12_SetMoveState(HERO_STATE_IDLE, 0)) {
                                set_hero_animation(hero_get_gadget_anim_id(0), 0, scale_ticks(0x12));
                            }
                        }
                        if (0.2f < g_hero.stick_magnitude) {
                            if (scale_ticks(0xA) < g_hero.state_timer) {
                                HeroL12_SetMoveState(HERO_STATE_WATER_SURFACE_MOVE, 1);
                            }
                        }
                        break;
                    case HERO_STATE_WATER_SURFACE_MOVE:
                        if (g_hero.jump_lockout_timer == 0) {
                            if (pad_pressed_within(0x40, scale_ticks(8), 0)) {
                                HeroL12_SetMoveState(0x12, 1);
                                break;
                            }
                        }
                        if (g_hero.unk1DE == 0) {
                            if (pad_pressed_within(0x80, scale_ticks(7), 0)) {
                                if (item_available[4] != 0 && (g_controller_state.held & (PAD_R2 | PAD_R1))) {
                                    HeroL12_SetMoveState(0x35, 1);
                                    break;
                                }
                                HeroL12_SetMoveState(HERO_STATE_SWIM, 1);
                                break;
                            }
                        }
                        if (g_hero.wading != 0 && g_hero.water_depth < 0.7f) {
                            HeroL12_SetMoveState(0x73, 0);
                            set_hero_animation(0x60, 0, scale_ticks(0x11));
                            g_hero.motion.velocity.f[2] = 0.0f;
                            g_hero.motion.displacement.f[2] = 0.0f;
                        }
                        if (0.1f > g_hero.stick_magnitude && g_hero.ground_speed == 0.0f && g_hero.unk698 == 0) {
                            HeroL12_SetMoveState(HERO_STATE_WATER_SURFACE_ENTRY, 1);
                        }
                        break;
                    case 118:
                        if (g_hero.health.hp <= 0) {
                            if (g_current_level_index == LEVEL_QUARTU || g_current_level_index == LEVEL_FLEET) {
                                HeroL12_SetMoveState(HERO_STATE_DYING_ZERO_HEALTH, 1);
                                break;
                            }
                            HeroL12_SetMoveState(HERO_STATE_DROWN, 1);
                            set_hero_animation(0x5F, 0x28, scale_ticks(0x12));
                            break;
                        }
                        if (g_hero.anim_flags & 2) {
                            HeroL12_SetMoveState(HERO_STATE_SWIM_IDLE, 1);
                            break;
                        }
                        if (scale_ticks(0x32) < g_hero.state_timer) {
                            HeroL12_SetMoveState(HERO_STATE_SWIM_IDLE, 0);
                        }
                        break;
                    case HERO_STATE_WATER_SURFACE_HURT:
                        if (g_hero.health.hp <= 0) {
                            HeroL12_SetMoveState(HERO_STATE_DROWN, 1);
                            set_hero_animation(0x5F, 0x25, scale_ticks(0x12));
                            break;
                        }
                        if (scale_ticks(0x32) < g_hero.state_timer) {
                            HeroL12_SetMoveState(HERO_STATE_WATER_SURFACE_ENTRY, 0);
                        }
                        break;
                    case 56:
                    case 57:
                        if (g_hero.items[0].item_id != 0x16) {
                            HeroL12_SetMoveState(HERO_STATE_IDLE, 1);
                        }
                        break;
                    case HERO_STATE_ANIMATION_WAIT_TO_IDLE:
                        if (g_hero.anim_flags & 2) {
                            HeroL12_SetMoveState(HERO_STATE_IDLE, 1);
                        }
                        break;
                    case HERO_STATE_LOOK:
                        if (g_hero.vehicle_mode == 0) {
                            if (g_hero.unk20AA == 0 || g_hero.unk20A5 != 0) {
                                if (g_controller_state.held & (PAD_R2 | PAD_R1)) {
                                    if (((unsigned char *)g_hero.moby)[0x53] != 0xD) {
                                        set_hero_animation(0xD, 0, scale_ticks(8));
                                    }
                                } else {
                                    int a = hero_get_gadget_anim_id(1);
                                    if (((unsigned char *)g_hero.moby)[0x53] != a) {
                                        set_hero_animation(a, 0, scale_ticks(8));
                                    }
                                }
                            }
                        }
                        {
                            unsigned char *m = (unsigned char *)g_hero.items[0].moby;
                            int ok = 1;
                            if (m != 0 && is_wrench_moby(m) && m[0x20] != 0) {
                                ok = 0;
                            }
                            if (g_controller_state.held & (PAD_L2 | PAD_L1)) {
                                break;
                            }
                            if (!ok) {
                                break;
                            }
                        }
                        if (g_hero.vehicle_mode != 0) {
                            request_hero_idle_state_for_vehicle_mode();
                            break;
                        }
                        if (g_controller_state.held & (PAD_R2 | PAD_R1)) {
                            HeroL12_SetMoveState(HERO_STATE_CROUCH, 1);
                            break;
                        }
                        HeroL12_SetMoveState(HERO_STATE_IDLE, 0);
                        {
                            int a = hero_get_gadget_anim_id(1);
                            if (((unsigned char *)g_hero.moby)[0x53] != a) {
                                set_hero_animation(a, 0, scale_ticks(8));
                            }
                        }
                        break;
                    case HERO_STATE_ITEM_BUTTON_RELEASE_WAIT:
                        if (g_controller_state.held & g_hero.items[0].button_mask) {
                            break;
                        }
                        if (g_hero.unk1D0 != 0) {
                            break;
                        }
                        HeroL12_SetMoveState(HERO_STATE_IDLE, 1);
                        break;
                    case HERO_STATE_WAIT_FOR_ITEM_RELEASE_THEN_IDLE:
                        if (g_controller_state.held & g_hero.items[0].button_mask) {
                            break;
                        }
                        if (g_hero.unk1D0 != 0) {
                            break;
                        }
                        if (((unsigned char *)g_hero.moby)[0x53] == 0x36) {
                            HeroL12_SetMoveState(HERO_STATE_IDLE, 0);
                            break;
                        }
                        HeroL12_SetMoveState(HERO_STATE_IDLE, 1);
                        break;
                    case HERO_STATE_ITEM_BUTTON_HOLD_ANIMATION_WAIT: {
                        int mask = g_hero.items[0].button_mask;
                        if (((unsigned char *)g_hero.moby)[0x52] == 0x2E && g_hero.anim_frame > 28.0f) {
                            if (g_controller_state.held & mask) {
                                HeroL12_SetMoveState(HERO_STATE_ITEM_BUTTON_RELEASE_WAIT, 1);
                                break;
                            }
                            HeroL12_SetMoveState(HERO_STATE_IDLE, 1);
                        }
                    } break;
                    case HERO_STATE_WRENCH_COMBO:
                    case HERO_STATE_WRENCH_SLAM:
                    case HERO_STATE_CROUCH_SWING:
                        if (g_hero.state.current == HERO_STATE_WRENCH_SLAM) {
                            int r = truncate_float_to_s32(
                                predict_hero_trajectory_intercept_time(60.0f, g_frame_time_sq * 130.0f, -1.0f));
                            int n = 0x1A;
                            if (g_hero.slam_descending == 0.0f) {
                                g_hero.anim_speed = 0.33f;
                            } else if (g_hero.slam_descending != 0.0f && g_hero.air_frames.s != 0 && r != 0 &&
                                       (float)n - g_hero.anim_frame > 1.0f) {
                                adjust_hero_animation_speed_for_target_frame(n, r, 0.5f, -1.0f);
                                if (g_hero.ground_distance < 0.7f && g_hero.anim_speed < 0.75f) {
                                    g_hero.anim_speed = 0.75f;
                                }
                            } else if (g_hero.air_frames.s == 0 && g_hero.anim_frame < (float)n) {
                                g_hero.anim_speed = 2.0f;
                            } else {
                                g_hero.anim_speed = 1.0f;
                            }
                        }
                        {
                            HeroAttackRecord *rec = &hoven_attack_records[g_hero.attack_record];
                            int t = (truncate_float_to_s32(g_hero.anim_frame) - rec->fC) * 2;
                            if ((float)rec->f14 < g_hero.anim_frame && g_hero.anim_changing == 0 &&
                                !(g_hero.state.current == HERO_STATE_CROUCH_SWING && g_hero.unk308 != 0)) {
                                if (pad_pressed_within(0x40, t, 0)) {
                                    if (g_controller_state.held & (PAD_R2 | PAD_R1)) {
                                        if (hero_handle_jump_press()) {
                                            break;
                                        }
                                    } else if (g_hero.jump_lock_timer == 0) {
                                        HeroL12_SetMoveState(HERO_STATE_JUMP, 1);
                                        break;
                                    }
                                }
                            }
                            if (g_hero.anim_changing == 0 && (float)rec->f18 < g_hero.anim_frame) {
                                HeroL12_SetMoveState(HERO_STATE_IDLE, 0);
                                break;
                            }
                        }
                        if (g_hero.state.current == HERO_STATE_CROUCH_SWING) {
                            if (g_hero.items[0].item_id != 8) {
                                HeroL12_SetMoveState(HERO_STATE_IDLE, 1);
                                break;
                            }
                            if (scale_ticks(0x64) < g_hero.state_timer && g_hero.queued_anim_frame < 0x1A) {
                                hero_set_attack_ticks(2);
                                g_hero.queued_anim_frame = 0x1A;
                            }
                        }
                        break;
                    case HERO_STATE_WAIT_FOR_ANIMATION_THEN_IDLE:
                    case HERO_STATE_WALL_BONK:
                        if (g_hero.anim_flags & 2) {
                            HeroL12_SetMoveState(HERO_STATE_IDLE, 1);
                        }
                        break;
                    case HERO_STATE_RUN:
                    case HERO_STATE_RUN_CLEAR_VELOCITY_Z:
                    case 115:
                    case 126:
                        if (g_hero.run_start_blending != 0 && (*(float *)(moby + 0x54) > 0.9f || moby[0x52] != 0xFF)) {
                            g_hero.run_start_blending = 0;
                        }
                        if (g_hero.on_slip_ground != 0 && g_hero.frames_off_ground.s == 0) {
                            HeroL12_SetMoveState(HERO_STATE_SLIP, 1);
                            break;
                        }
                        if (!(g_hero.air_frames.s < scale_ticks(5)) &&
                            !(g_hero.state.prev_control_mode == HERO_MODE_WATER_SURFACE && g_hero.wading != 0) &&
                            g_hero.ground_distance > 1.35f) {
                            if (check_hero_forward_collision_height() == 0) {
                                HeroL12_SetMoveState(HERO_STATE_FALL, 1);
                                break;
                            }
                        }
                        {
                            int t = scale_ticks(9);
                            if (g_hero.jump_buffer_lockout != 0) {
                                t = 1;
                            }
                            if (pad_pressed_within(0x40, t, 0)) {
                                if (!(g_hero.air_frames.s > scale_ticks(4)) || g_hero.still_frames != 0) {
                                    stick_target_v(0, 1.0f);
                                    if (try_start_hero_jump(t, 1)) {
                                        break;
                                    }
                                    if (g_hero.state.step <= 0) {
                                        HeroL12_SetMoveState(HERO_STATE_JUMP, 1);
                                        break;
                                    }
                                    if (g_hero.target_speed > D_L12_X_f.fC &&
                                        FastDiffRots(g_hero.motion.move_yaw, g_hero.motion.rot.f[2]) < 1.0471976f &&
                                        D_L12_X_f.f18 * g_frame_time * 0.85f < g_hero.motion.flat_speed) {
                                        HeroL12_SetMoveState(HERO_STATE_RUN_JUMP, 1);
                                        break;
                                    }
                                    HeroL12_SetMoveState(HERO_STATE_JUMP, 1);
                                    break;
                                }
                            }
                        }
                        if (g_hero.air_frames.s != 0) {
                            break;
                        }
                        if (g_hero.on_ice_ground == 0 ||
                            (g_frame_time * 2.0f > g_hero.motion.flat_speed && g_hero.ground_slope.f < 0.08726646f)) {
                            if (g_controller_state.held & (PAD_L2 | PAD_L1)) {
                                HeroL12_SetMoveState(HERO_STATE_LOOK, 1);
                                break;
                            }
                        }
                        if (g_controller_state.held & (PAD_R2 | PAD_R1)) {
                            HeroL12_SetMoveState(HERO_STATE_CROUCH, 1);
                            break;
                        }
                        if (g_hero.on_ice_ground != 0 && g_hero.state.current == HERO_STATE_RUN_CLEAR_VELOCITY_Z) {
                            float len = vector_length_xy(g_hero.motion.displacement.f);
                            if (g_hero.stick_magnitude < 0.17f &&
                                (((unsigned char *)g_hero.moby)[0x53] == 0x37 ||
                                 (((unsigned char *)g_hero.moby)[0x53] == hero_get_gadget_anim_id(0) &&
                                  g_hero.ground_slope.f > 0.08726646f)) &&
                                scale_ticks(8) < g_hero.stick_idle_frames && g_hero.anim_changing == 0) {
                                set_hero_animation(0x6D, 0, scale_ticks(0xE));
                            } else if (0.3f < g_hero.stick_magnitude && ((unsigned char *)g_hero.moby)[0x53] != 0x37 &&
                                       g_hero.anim_changing == 0) {
                                set_hero_animation(0x37, 0, scale_ticks(0xE));
                            } else if (((unsigned char *)g_hero.moby)[0x53] == 0x6D && g_hero.anim_changing == 0 &&
                                       g_hero.stick_magnitude < 0.17f && g_hero.ground_slope.f < 0.08726646f &&
                                       len < g_frame_time * 1.7f) {
                                set_hero_animation(hero_get_gadget_anim_id(0), 0, scale_ticks(0x14));
                            }
                        }
                        if (g_hero.state.current == HERO_STATE_RUN || g_hero.state.current == 0x7E) {
                            if (g_hero.on_ice_ground != 0) {
                                HeroL12_SetMoveState(HERO_STATE_RUN_CLEAR_VELOCITY_Z, 1);
                            }
                        } else if (g_hero.state.current == HERO_STATE_RUN_CLEAR_VELOCITY_Z) {
                            if (g_hero.on_ice_ground == 0) {
                                HeroL12_SetMoveState(HERO_STATE_RUN, 1);
                            }
                        }
                        if (g_hero.state.current == HERO_STATE_RUN) {
                            if (g_hero.wading != 0) {
                                HeroL12_SetMoveState(0x73, 1);
                            }
                        } else if (g_hero.state.current == 0x73) {
                            if (g_hero.wading == 0) {
                                HeroL12_SetMoveState(HERO_STATE_RUN, 1);
                            }
                        }
                        if (g_hero.state.current == HERO_STATE_RUN && g_hero.unk308 != 0) {
                            HeroL12_SetMoveState(0x3F, 1);
                        }
                        if (g_hero.state_timer > scale_ticks(0xF) ||
                            (g_hero.state.prev_control_mode != HERO_MODE_IDLE &&
                             g_hero.state.prev_control_mode != HERO_MODE_RUN)) {
                            if ((g_controller_state.pressed & 0x10000) && g_hero.unk20AA == 0) {
                                g_hero.unk3BE = 1;
                            }
                        }
                        if (g_hero.state.current == HERO_STATE_RUN) {
                            if (g_hero.stick_magnitude < 0.17f) {
                                if (g_frame_time * 2.7f < g_hero.motion.speed_xy && g_hero.unk20AA == 0) {
                                    if (g_hero.motion.speed_xy < g_frame_time * 4.4f ||
                                        read_pad_float_history(4) < 0.8f) {
                                        if (HeroL12_SetMoveState(HERO_STATE_STOP, 0) == 0) {
                                            break;
                                        }
                                        if (hero_get_gadget_anim_id(0) == 0x54) {
                                            set_hero_animation(hero_get_gadget_anim_id(0), 0, scale_ticks(0x12));
                                        } else {
                                            set_hero_animation(0x14, 7, scale_ticks(9));
                                        }
                                        break;
                                    }
                                    HeroL12_SetMoveState(HERO_STATE_STOP, 1);
                                    break;
                                }
                                if (!(g_hero.state_timer > scale_ticks(0x19))) {
                                    break;
                                }
                                if (try_start_hero_item_idle_animation()) {
                                    break;
                                }
                                if (HeroL12_SetMoveState(HERO_STATE_IDLE, 0) == 0) {
                                    break;
                                }
                                if (hero_get_gadget_anim_id(0) == 0x54) {
                                    set_hero_animation(hero_get_gadget_anim_id(0), 0, scale_ticks(0x12));
                                } else {
                                    set_hero_animation(hero_get_gadget_anim_id(0), 0, -1.0f);
                                }
                                break;
                            }
                            update_hero_animation_for_movement_speed();
                            break;
                        } else if (g_hero.state.current == HERO_STATE_RUN_CLEAR_VELOCITY_Z) {
                            if (g_controller_state.no_direction != 0) {
                                if (vector_length_xy(g_hero.motion.velocity.f) < g_frame_time * 0.05f &&
                                    g_hero.ground_slope.f < 0.08726646f) {
                                    if (HeroL12_SetMoveState(HERO_STATE_IDLE, 0)) {
                                        set_hero_animation(hero_get_gadget_anim_id(0), 0, scale_ticks(0x11));
                                    }
                                }
                            }
                            if (moby[0x53] == 0x37) {
                                g_hero.anim_speed = (1.0f - g_hero.unk3B0) * 2.0f;
                                if (g_hero.anim_speed > 1.7f) {
                                    g_hero.anim_speed = 1.7f;
                                }
                                if (g_hero.anim_speed < 0.75f) {
                                    g_hero.anim_speed = 0.75f;
                                }
                            } else if (moby[0x53] == 0x6D) {
                                g_hero.anim_speed = g_hero.motion.speed_xy * 10.0f;
                                if (g_hero.anim_speed > 1.1f) {
                                    g_hero.anim_speed = 1.1f;
                                }
                                if (g_hero.anim_speed < 0.3f) {
                                    g_hero.anim_speed = 0.3f;
                                }
                            }
                        } else if (g_hero.state.current == 0x73) {
                            if (g_controller_state.no_direction != 0) {
                                if (vector_length_xy(g_hero.motion.velocity.f) < g_frame_time * 3.2f) {
                                    if (HeroL12_SetMoveState(HERO_STATE_IDLE, 0)) {
                                        set_hero_animation(hero_get_gadget_anim_id(0), 0, scale_ticks(0x11));
                                    }
                                }
                            }
                            g_hero.anim_speed = g_hero.motion.speed_xy * 35.0f;
                            if (g_hero.anim_speed < 0.6f) {
                                g_hero.anim_speed = 0.6f;
                            }
                            if (g_hero.anim_speed > 1.05f) {
                                g_hero.anim_speed = 1.05f;
                            }
                        }
                        break;
                    case HERO_STATE_APPROACH_TARGET_POSE:
                        if (vec_distance_xy(g_hero.motion.pos.f, g_hero.target_pose.f) < 0.2f) {
                            if (g_frame_time * 2.7f > g_hero.motion.speed_xy) {
                                if (FastDiffRots(g_hero.motion.rot.f[2], g_hero.target_pose.f[3]) < 0.034906585f) {
                                    HeroL12_SetMoveState(HERO_STATE_AT_TARGET_POSE, 1);
                                    break;
                                }
                            }
                            if (g_hero.vehicle_mode != 0 && g_frame_time * 2.7f <= g_hero.motion.speed_xy) {
                                HeroL12_SetMoveState(HERO_STATE_BRAKE_AT_TARGET_POSITION, 1);
                            }
                            break;
                        }
                        update_hero_animation_for_movement_speed();
                        break;
                    case HERO_STATE_BRAKE_AT_TARGET_POSITION:
                        if (g_hero.anim_flags & 2) {
                            HeroL12_SetMoveState(HERO_STATE_APPROACH_TARGET_POSE, 1);
                        }
                        break;
                    case HERO_STATE_AT_TARGET_POSE:
                        if ((g_hero.anim_flags & 2) || g_hero.unk1650 != 0) {
                            HeroL12_SetMoveState(HERO_STATE_IDLE, 1);
                        }
                        if (0.2f < vec_distance_xy(g_hero.motion.pos.f, g_hero.target_pose.f)) {
                            HeroL12_SetMoveState(HERO_STATE_APPROACH_TARGET_POSE, 1);
                        }
                        break;
                    case 63:
                        g_hero.anim_speed = g_hero.ground_speed * 27.0f;
                        if (g_hero.anim_speed < 0.5f) {
                            g_hero.anim_speed = 0.5f;
                        }
                        if (g_controller_state.held & (PAD_R2 | PAD_R1)) {
                            HeroL12_SetMoveState(HERO_STATE_CROUCH, 1);
                            break;
                        }
                        if (pad_pressed_within(0x40, scale_ticks(7), 0)) {
                            HeroL12_SetMoveState(HERO_STATE_ANIMATION_FRAME_EXIT_WAIT, 1);
                            break;
                        }
                        if (!(g_hero.air_frames.s < scale_ticks(5)) && g_hero.ground_distance > 0.8f) {
                            HeroL12_SetMoveState(HERO_STATE_FALL, 1);
                            break;
                        }
                        g_hero.unk1F8 = scale_ticks(0x28);
                        if (absolute_float(g_hero.motion.rot.f[1]) > 0.7853982f ||
                            absolute_float(g_hero.motion.rot.f[0]) > 0.7853982f) {
                            g_hero.ledge_probe_cooldown = scale_ticks(0x28);
                        }
                        if (g_hero.grounded_frames != 0 && g_hero.on_magnet_ground == 0 &&
                            absolute_float(g_hero.motion.rot.f[1]) < 0.7853982f &&
                            absolute_float(g_hero.motion.rot.f[0]) < 0.7853982f) {
                            HeroL12_SetMoveState(HERO_STATE_RUN, 1);
                            break;
                        }
                        if (((unsigned char *)g_hero.moby)[0x53] != 0x5B) {
                            set_hero_animation(0x5B, 0, scale_ticks(8));
                        }
                        if (g_controller_state.no_direction != 0 && !(g_hero.state_timer < scale_ticks(0x1E))) {
                            if (HeroL12_SetMoveState(HERO_STATE_IDLE, 0)) {
                                clear_u64_value(g_hero.motion.momentum.f);
                                set_hero_animation(hero_get_gadget_anim_id(0), 0, scale_ticks(0xF));
                            }
                        }
                        break;
                    case HERO_STATE_ATTACK_RECORD_FRAME_EXIT_WAIT:
                        if (g_hero.anim_frame > (float)hoven_attack_records[g_hero.attack_record].f18) {
                            HeroL12_SetMoveState(HERO_STATE_IDLE, 1);
                        }
                        break;
                    case HERO_STATE_ANIMATION_FRAME_EXIT_WAIT:
                        if (25.0f < g_hero.anim_frame) {
                            HeroL12_SetMoveState(HERO_STATE_IDLE, 1);
                        }
                        break;
                    case HERO_STATE_STOP:
                        if (g_controller_state.held & (PAD_L2 | PAD_L1)) {
                            HeroL12_SetMoveState(HERO_STATE_LOOK, 1);
                            break;
                        }
                        if (g_hero.move_lock_timer == 0) {
                            if (g_controller_state.pressed & PAD_CROSS) {
                                HeroL12_SetMoveState(HERO_STATE_JUMP, 1);
                                break;
                            }
                            if (g_controller_state.held & (PAD_R2 | PAD_R1)) {
                                HeroL12_SetMoveState(HERO_STATE_CROUCH, 1);
                                break;
                            }
                        }
                        if (g_hero.air_frames.s > scale_ticks(8) && g_hero.ground_distance > 0.3f) {
                            HeroL12_SetMoveState(HERO_STATE_FALL, 1);
                            break;
                        }
                        if (g_hero.anim_flags & 2) {
                            HeroL12_SetMoveState(HERO_STATE_IDLE, 1);
                            break;
                        }
                        if (!(g_hero.stick_magnitude > 0.2f)) {
                            break;
                        }
                        if (hero_get_gadget_anim_id(0) != 0x54) {
                            if (g_hero.ground_speed == 0.0f) {
                                HeroL12_SetMoveState(HERO_STATE_IDLE, 0);
                                break;
                            }
                        } else if (g_hero.anim_frame > 10.0f) {
                            if (HeroL12_SetMoveState(HERO_STATE_IDLE, 0)) {
                                set_hero_animation(hero_get_gadget_anim_id(0), 0, scale_ticks(0x11));
                            }
                        }
                        if (0.5f < g_hero.stick_magnitude) {
                            HeroL12_SetMoveState(HERO_STATE_RUN, 1);
                        }
                        break;
                    case HERO_STATE_CROUCH:
                        if (g_controller_state.held & (PAD_L2 | PAD_L1)) {
                            HeroL12_SetMoveState(HERO_STATE_LOOK, 1);
                            break;
                        }
                        if (g_hero.state_timer > scale_ticks(0xB) && !(g_controller_state.held & (PAD_R2 | PAD_R1))) {
                            HeroL12_SetMoveState(HERO_STATE_IDLE, 0);
                            set_hero_animation(hero_get_gadget_anim_id(0), 0, -2.0f);
                            break;
                        }
                        if (g_hero.on_slip_ground != 0 && g_hero.frames_off_ground.s == 0) {
                            HeroL12_SetMoveState(HERO_STATE_SLIP, 1);
                            break;
                        }
                        if (g_hero.unk308 == 1) {
                            if (pad_pressed_within(0x40, scale_ticks(7), 0)) {
                                HeroL12_SetMoveState(HERO_STATE_ANIMATION_FRAME_EXIT_WAIT, 1);
                                break;
                            }
                        } else if (hero_handle_jump_press()) {
                            break;
                        }
                        if (pad_pressed_within(0x80, scale_ticks(0xF), 0)) {
                            if (g_hero.items[0].item_id == 8) {
                                HeroL12_SetMoveState(HERO_STATE_CROUCH_SWING, 1);
                            }
                        }
                        if (g_hero.air_frames.s > scale_ticks(8)) {
                            if (g_hero.state.prev != HERO_STATE_LAND) {
                                HeroL12_SetMoveState(HERO_STATE_FALL, 1);
                                break;
                            }
                            if (scale_ticks(0xF) > g_hero.state_timer) {
                                break;
                            }
                            HeroL12_SetMoveState(HERO_STATE_FALL, 1);
                        }
                        break;
                    case HERO_STATE_GRAPPLE_TARGET_ENTRY:
                        if (g_hero.items[0].item_id != 0xC) {
                            HeroL12_SetMoveState(HERO_STATE_IDLE, 1);
                        }
                        break;
                    case HERO_STATE_APPROACH_TARGET_MOBY:
                        adjust_hero_animation_speed_for_target_frame(10.0f, g_hero.target_arrival_frames, 0.5f, -1.0f);
                        if ((float)scale_ticks(0x16) > g_hero.target_arrival_frames &&
                            (float)scale_ticks(0x1E) > g_hero.target_arrival_frames) {
                            set_hero_animation(0xA, 0, scale_ticks(0x10));
                        }
                        {
                            int go = 0;
                            if (!(g_controller_state.held & g_hero.items[0].button_mask)) {
                                if ((float)scale_ticks(0x19) < g_hero.target_arrival_frames) {
                                    g_hero.unk988 = 1;
                                }
                                go = 1;
                            }
                            if (g_hero.state.step == 1 && g_frame_time * 4.0f > g_hero.grapple_pull_speed) {
                                go = 1;
                            }
                            if (!go) {
                                break;
                            }
                        }
                        g_hero.ground_speed = g_hero.grapple_pull_speed;
                        if (g_frame_time * 7.0f < g_hero.ground_speed) {
                            g_hero.ground_speed = g_frame_time * 7.0f;
                        }
                        if (g_hero.ground_distance < 0.3f) {
                            HeroL12_SetMoveState(HERO_STATE_IDLE, 0);
                            set_hero_animation(hero_get_gadget_anim_id(0), 0, scale_ticks(0x1E));
                            break;
                        }
                        if (((unsigned char *)g_hero.moby)[0x53] == 0xA) {
                            HeroL12_SetMoveState(HERO_STATE_FALL, 0);
                        } else {
                            HeroL12_SetMoveState(HERO_STATE_FALL, 0);
                            set_hero_animation(0xA, 0, scale_ticks(0x11));
                        }
                        g_hero.unk2284 = 0;
                        g_hero.stickless_decel = g_frame_time_sq * 42.0f;
                        g_hero.fall_state_gravity = g_frame_time_sq * 29.0f;
                        break;
                    case HERO_STATE_CONSTRAIN_TO_TARGET_RADIUS:
                        if (g_controller_state.held & g_hero.items[0].button_mask) {
                            break;
                        }
                        g_hero.ground_speed = g_hero.grapple_pull_speed;
                        if (g_frame_time * 7.0f < g_hero.ground_speed) {
                            g_hero.ground_speed = g_frame_time * 7.0f;
                        }
                        if (g_hero.ground_distance < 0.4f) {
                            HeroL12_SetMoveState(HERO_STATE_IDLE, 1);
                            break;
                        }
                        HeroL12_SetMoveState(HERO_STATE_FALL, 1);
                        g_hero.unk2284 = 0;
                        break;
                    case HERO_STATE_GRAPPLE_SWING_ENTRY:
                        if (g_controller_state.held & g_hero.items[0].button_mask) {
                            break;
                        }
                        if (scale_ticks(0xA) < g_hero.state_timer) {
                            if (scale_ticks(0x28) > g_hero.state_timer) {
                                g_hero.unk988 = 1;
                            }
                            HeroL12_SetMoveState(HERO_STATE_ITEM_RELEASE_FALL, 1);
                            g_hero.stickless_decel = g_frame_time_sq * 7.0f;
                            g_hero.fall_state_gravity = g_frame_time_sq * 27.0f;
                        }
                        break;
                    case HERO_STATE_RISE_THEN_FALL_WITH_AIRBORNE_HIT_QUERY:
                        if (g_hero.state_timer < scale_ticks(0xC) &&
                            (g_hero.state.prev2 != HERO_STATE_PACK_LONG_JUMP ||
                             (g_hero.state.prev != HERO_STATE_LAND && g_hero.state.prev != HERO_STATE_FALL)) &&
                            g_hero.state.prev != 0xF && g_hero.state.prev != HERO_STATE_PACK_JUMP &&
                            g_hero.state.prev != HERO_STATE_PACK_LONG_JUMP && g_hero.state.prev != 0x12 &&
                            g_hero.state.prev != 0x3C && (g_controller_state.pressed & PAD_CROSS)) {
                            HeroL12_SetMoveState(HERO_STATE_PACK_LONG_JUMP, 1);
                        }
                        {
                            int r = truncate_float_to_s32(
                                predict_hero_trajectory_intercept_time(60.0f, g_hero.rise_fall_gravity, -1.0f));
                            g_hero.predicted_land_frames = r;
                            if (g_hero.past_apex != 0.0f && g_hero.frames_off_ground.s != 0 && r != 0) {
                                if (r == 2) {
                                    scale_ticks(0xA);
                                }
                                adjust_hero_animation_speed_for_target_frame(g_hero.rise_fall_target_frame,
                                                                             g_hero.predicted_land_frames, 0.5f, -1.0f);
                            } else {
                                g_hero.anim_speed = 1.0f;
                            }
                        }
                        if (g_hero.anim_flags & 2) {
                            HeroL12_SetMoveState(HERO_STATE_IDLE, 1);
                        }
                        break;
                    case HERO_STATE_LEDGE_HANG_ENTRY:
                        if (13.5f < g_hero.anim_frame) {
                            HeroL12_SetMoveState(HERO_STATE_LEDGE_HANG, 0);
                        }
                        break;
                    case HERO_STATE_LEDGE_HANG:
                        if ((g_hero.anim_flags & 2) && ((unsigned char *)g_hero.moby)[0x53] == 0x20) {
                            set_hero_animation(0x21, 0, scale_ticks(6));
                        }
                        stick_target_v(0, 1.0f);
                        qcopy(v0, g_hero.motion.pos.f);
                        sp24 = g_hero.motion.rot.f[2];
                        if (test_hero_ledge_ahead(v0, &sp24) == 0) {
                            HeroL12_SetMoveState(HERO_STATE_FALL, 1);
                            g_hero.ledge_probe_cooldown = scale_ticks(0xA);
                            break;
                        }
                        if ((g_hero.stick_magnitude > 0.5f && pad_pressed_within(0x40, scale_ticks(9), 0) &&
                             2.3561945f < FastDiffRots(g_hero.motion.move_yaw, g_hero.motion.rot.f[2])) ||
                            (g_controller_state.pressed & (PAD_R2 | PAD_R1))) {
                            HeroL12_SetMoveState(HERO_STATE_FALL, 1);
                            g_hero.ledge_probe_cooldown = scale_ticks(0xA);
                            break;
                        }
                        if (pad_pressed_within(0x40, scale_ticks(0x11), 0)) {
                            HeroL12_SetMoveState(HERO_STATE_LEDGE_JUMP, 1);
                            break;
                        }
                        if (0.5f < g_hero.stick_magnitude &&
                            FastDiffRots(g_hero.motion.move_yaw, FastAddRots(g_hero.motion.rot.f[2], 1.5707964f)) <
                                0.7853982f) {
                            qcopy(v1, g_hero.motion.pos.f);
                            v1[0] += FastCos(FastAddRots(g_hero.motion.rot.f[2], 1.5707964f)) * 0.3f;
                            v1[1] += FastSin(FastAddRots(g_hero.motion.rot.f[2], 1.5707964f)) * 0.3f;
                            sp28 = g_hero.motion.rot.f[2];
                            if (test_hero_ledge_ahead(v1, &sp28)) {
                                HeroL12_SetMoveState(HERO_STATE_LEDGE_SHIMMY_POSITIVE_YAW, 1);
                            }
                            break;
                        }
                        if (0.5f < g_hero.stick_magnitude &&
                            FastDiffRots(g_hero.motion.move_yaw, FastAddRots(g_hero.motion.rot.f[2], -1.5707964f)) <
                                0.7853982f) {
                            qcopy(v1, g_hero.motion.pos.f);
                            v1[0] += FastCos(FastAddRots(g_hero.motion.rot.f[2], -1.5707964f)) * 0.3f;
                            v1[1] += FastSin(FastAddRots(g_hero.motion.rot.f[2], -1.5707964f)) * 0.3f;
                            sp2C = g_hero.motion.rot.f[2];
                            if (test_hero_ledge_ahead(v1, &sp2C)) {
                                HeroL12_SetMoveState(HERO_STATE_LEDGE_SHIMMY_NEGATIVE_YAW, 1);
                            }
                        }
                        break;
                    case HERO_STATE_LEDGE_SHIMMY_POSITIVE_YAW:
                    case HERO_STATE_LEDGE_SHIMMY_NEGATIVE_YAW: {
                        float ang;

                        qcopy(v0, g_hero.motion.pos.f);
                        ang = 1.5707964f;
                        if (g_hero.state.current == HERO_STATE_LEDGE_SHIMMY_NEGATIVE_YAW) {
                            ang = -1.5707964f;
                        }
                        v0[0] += FastCos(FastAddRots(g_hero.motion.rot.f[2], ang)) * 0.3f;
                        v0[1] += FastSin(FastAddRots(g_hero.motion.rot.f[2], ang)) * 0.3f;
                        sp30 = g_hero.motion.rot.f[2];
                        stick_target_v(0, 1.0f);
                        if (g_hero.stick_magnitude < 0.5f ||
                            0.7853982f <
                                FastDiffRots(g_hero.motion.move_yaw, FastAddRots(g_hero.motion.rot.f[2], ang)) ||
                            test_hero_ledge_ahead(v0, &sp30) == 0) {
                            if (g_hero.anim_flags & 2) {
                                HeroL12_SetMoveState(HERO_STATE_LEDGE_HANG, 1);
                                break;
                            }
                        }
                    }
                        if (0.5f < g_hero.stick_magnitude && pad_pressed_within(0x40, scale_ticks(9), 0) &&
                            2.3561945f < FastDiffRots(g_hero.motion.move_yaw, g_hero.motion.rot.f[2])) {
                            HeroL12_SetMoveState(HERO_STATE_FALL, 1);
                            g_hero.ledge_probe_cooldown = scale_ticks(0x28);
                            break;
                        }
                        if (pad_pressed_within(0x40, scale_ticks(9), 0)) {
                            HeroL12_SetMoveState(HERO_STATE_LEDGE_JUMP, 1);
                        }
                        break;
                    case HERO_STATE_SLIP:
                        if (scale_ticks(0x12C) < g_hero.state_timer) {
                            hero_respawn();
                        }
                        if (g_hero.on_slip_ground != 0) {
                            break;
                        }
                        if (g_hero.air_frames.s != 0) {
                            HeroL12_SetMoveState(HERO_STATE_FALL, 1);
                            break;
                        }
                        HeroL12_SetMoveState(HERO_STATE_IDLE, 1);
                        break;
                    case HERO_STATE_FALL:
                    case HERO_STATE_ITEM_RELEASE_FALL:
                        if (g_hero.state.step == 0) {
                            if (!(scale_ticks(0x12) > g_hero.state_timer) || g_hero.ground_distance > 1.75f) {
                                set_hero_animation(0xB, 0, scale_ticks(0x10));
                                g_hero.state.step = 1;
                                if (g_hero.state.current == HERO_STATE_ITEM_RELEASE_FALL) {
                                    g_hero.unk2284 = 7;
                                } else {
                                    g_hero.unk2284 = 0;
                                }
                            }
                        }
                        if (g_hero.air_frames.s == 0) {
                            int first;
                            int one;

                            emit_hero_animation_voice_and_ground_particle(g_hero.ground_sound_class, 0, 1);
                            emit_hero_animation_voice_and_ground_particle(g_hero.ground_sound_class, 1, 1);
                            if (g_frame_time * -9.0f > g_hero.motion.displacement.f[2]) {
                                g_hero.motion.displacement.f[2] = g_frame_time * -9.0f;
                            }
                            if (g_frame_time * -9.0f > g_hero.motion.velocity.f[2]) {
                                g_hero.motion.velocity.f[2] = g_frame_time * -9.0f;
                            }
                            if (g_hero.health.hp <= 0) {
                                HeroL12_SetMoveState(HERO_STATE_DYING, 1);
                                break;
                            }
                            first = ((int)g_hero.ground_moby) == 0;
                            one = 1;
                            if (!(scale_ticks(0x5A) > g_hero.state_timer)) {
                                if (HeroL12_SetMoveState(HERO_STATE_IDLE, 0)) {
                                    set_hero_animation(0xC, 4, scale_ticks(6));
                                }
                                if (first) {
                                    g_hero.move_lock_timer = scale_ticks(0x16);
                                }
                                configure_hero_point_effect_emission(0.022f, 0.004f, 0xA, 4, 0, 1);
                                break;
                            }
                            if (g_hero.state.step == one) {
                                if (HeroL12_SetMoveState(HERO_STATE_IDLE, 0)) {
                                    set_hero_animation(0xC, 9, -1.0f);
                                }
                                gravity_set_z(g_hero.motion.momentum.f, g_hero.motion.momentum.f, 0.0f);
                                ClampVectorLength(g_hero.motion.momentum.f, g_frame_time * 7.0f);
                                g_hero.move_lock_timer = scale_ticks(7);
                                if (g_hero.state.prev == HERO_STATE_ITEM_RELEASE_FALL) {
                                    g_hero.move_lock_timer = scale_ticks(0xA);
                                }
                                if (first) {
                                    if (scale_ticks(0x32) < g_hero.state_timer) {
                                        g_hero.move_lock_timer = scale_ticks(0x12);
                                    }
                                }
                                configure_hero_point_effect_emission(0.019f, 0.002f, 0xA, 2, 0, 1);
                                break;
                            }
                            if (g_hero.stick_magnitude > 0.5f && g_hero.move_lock_timer == 0) {
                                HeroL12_SetMoveState(HERO_STATE_RUN, 1);
                                configure_hero_point_effect_emission(0.016f, 0.002f, 7, 2, 0, 1);
                                break;
                            }
                            if (g_frame_time * 3.0f < g_hero.motion.flat_speed) {
                                if (HeroL12_SetMoveState(HERO_STATE_STOP, 0) == 0) {
                                    break;
                                }
                                set_hero_animation(6, 0, scale_ticks(0xC));
                                ClampVectorLength(g_hero.motion.momentum.f, g_frame_time * 7.7f);
                                break;
                            }
                            HeroL12_SetMoveState(HERO_STATE_IDLE, 1);
                            configure_hero_point_effect_emission(0.016f, 0.002f, 5, 2, 0, 1);
                            break;
                        }
                        if (g_hero.health.hp <= 0) {
                            break;
                        }
                        if ((g_controller_state.held & PAD_CROSS) && item_available[2] != 0 &&
                            hero_slot_item_id(3) != 4 && g_hero.pack_disabled == 0 && g_hero.move_lock_timer == 0 &&
                            g_hero.ground_distance > 0.6f) {
                            HeroL12_SetMoveState(HERO_STATE_LAND, 1);
                            break;
                        }
                        if (g_hero.still_frames > scale_ticks(0x14)) {
                            int t = scale_ticks(6);
                            if (g_hero.jump_buffer_lockout != 0) {
                                t = 1;
                            }
                            if (try_start_hero_jump(t, 0)) {
                                break;
                            }
                        }
                        if (hero_slot_item_id(3) == 3 && g_hero.pack_disabled == 0) {
                            if (check_pad_press_separation(0x40, 0xA, scale_ticks(9), scale_ticks(8))) {
                                if ((g_controller_state.held & (PAD_R2 | PAD_R1)) &&
                                    (g_controller_state.pressed & PAD_CROSS)) {
                                    if (g_hero.past_jump_apex == 0 || g_hero.ground_distance > 1.5f) {
                                        HeroL12_SetMoveState(HERO_STATE_PACK_LONG_JUMP, 1);
                                        break;
                                    }
                                }
                            }
                        }
                        if (g_hero.ledge_found != 0) {
                            HeroL12_SetMoveState(HERO_STATE_LEDGE_HANG_ENTRY, 1);
                        }
                        break;
                    case HERO_STATE_FATAL_FALL:
                        if ((g_current_level_index == LEVEL_KERWAN && 5.0f > g_hero.motion.pos.f[2]) ||
                            (g_current_level_index == LEVEL_BLARG && 50.0f > g_hero.motion.pos.f[2]) ||
                            (g_current_level_index == LEVEL_KALEBO3 &&
                             (77.0f > g_hero.motion.pos.f[2] || (g_hero.state_timer > 0x14 && g_hero.wall_hit != 0 &&
                                                                 g_hero.motion.displacement.f[2] >= 0.0f))) ||
                            g_hero.state_timer > scale_ticks(0x78)) {
                            hero_respawn();
                            return;
                        }
                        break;
                    case HERO_STATE_DELAYED_JUMP_OR_R1_REPEAT_EXIT_WAIT:
                        if (g_hero.on_slip_ground != 0 && g_hero.ground_distance < 0.5f) {
                            HeroL12_SetMoveState(HERO_STATE_SLIP, 1);
                            break;
                        }
                        {
                            int both = 0;

                            fast_dec_timer(&hoven_hero_double_tap_timer);
                            if (hoven_hero_double_tap_timer != 0) {
                                both = 1;
                                if (!((g_controller_state.held & PAD_R1) && (g_controller_state.pressed & PAD_R1))) {
                                    both = 0;
                                }
                            }
                            if (g_controller_state.pressed & PAD_R1) {
                                hoven_hero_double_tap_timer = scale_ticks(0x1E);
                            }
                            if (!(g_hero.state_timer > scale_ticks(0x14))) {
                                break;
                            }
                            if (!(g_controller_state.pressed & PAD_CROSS) && !both) {
                                break;
                            }
                        }
                        g_hero.unk22CA = 0;
                        g_hero.move_lock_timer = scale_ticks(0x19);
                        if (g_hero.air_frames.s != 0) {
                            if (HeroL12_SetMoveState(HERO_STATE_IDLE, 0) == 0) {
                                break;
                            }
                            set_hero_animation(7, 0x17, scale_ticks(9));
                            configure_hero_point_effect_emission(0.016f, 0.002f, 5, 2, 0, 1);
                            break;
                        }
                        if (HeroL12_SetMoveState(HERO_STATE_IDLE, 0) == 0) {
                            break;
                        }
                        set_hero_animation(hero_get_gadget_anim_id(0), 0, -2.0f);
                        configure_hero_point_effect_emission(0.016f, 0.002f, 5, 2, 0, 1);
                        break;
                    case HERO_STATE_LAND:
                        if (g_hero.wall_jump_window != 0) {
                            if (scale_ticks(9) > g_hero.state_timer) {
                                int hit = 0;
                                if (g_hero.state.prev == HERO_STATE_JUMP || g_hero.state.prev == HERO_STATE_RUN_JUMP ||
                                    g_hero.state.prev == HERO_STATE_DOUBLE_JUMP || g_hero.state.prev == 0x11) {
                                    hit = 1;
                                }
                                if (hit) {
                                    HeroL12_SetMoveState(0x11, 1);
                                }
                                break;
                            }
                        }
                        if (g_hero.state_timer > scale_ticks(0x1E) && !(g_controller_state.held & PAD_CROSS) &&
                            !(((int)g_controller_state.held_unmasked) & 0x40)) {
                            HeroL12_SetMoveState(HERO_STATE_FALL, 1);
                            break;
                        }
                        if (g_hero.ledge_found != 0) {
                            HeroL12_SetMoveState(HERO_STATE_LEDGE_HANG_ENTRY, 1);
                            break;
                        }
                        if (g_hero.air_frames.s != 0 && !(g_hero.motion.pos.f[2] - g_hero.ground_z.f < 0.4f)) {
                            break;
                        }
                        if (scale_ticks(0xA) > g_hero.state_timer) {
                            g_hero.jump_buffer_lockout = scale_ticks(0xC) - g_hero.state_timer;
                        }
                        stick_target_v(0, 1.0f);
                        if (g_hero.state.prev == HERO_STATE_PACK_LONG_JUMP && scale_ticks(4) > g_hero.state_timer) {
                            if (HeroL12_SetMoveState(HERO_STATE_STOP, 0) == 0) {
                                break;
                            }
                            set_hero_animation(5, 0, -1.0f);
                            g_hero.move_lock_timer = scale_ticks(0x22);
                            break;
                        }
                        if (g_controller_state.held & (PAD_R2 | PAD_R1)) {
                            HeroL12_SetMoveState(HERO_STATE_CROUCH, 1);
                            configure_hero_point_effect_emission(0.016f, 0.002f, 5, 2, 0, 1);
                            break;
                        }
                        if (g_hero.target_speed > D_L12_X_f.fC &&
                            FastDiffRots(g_hero.motion.move_yaw, g_hero.motion.rot.f[2]) < 1.5707964f) {
                            if (g_hero.on_ice_ground != 0 || g_hero.wading != 0) {
                                HeroL12_SetMoveState(HERO_STATE_RUN, 1);
                                break;
                            }
                            HeroL12_SetMoveState(HERO_STATE_RUN, 0);
                            if (g_hero.state.current == HERO_STATE_RUN) {
                                g_hero.state.step = 1;
                                set_hero_animation(4, 2, scale_ticks(8));
                                g_hero.run_start_blending = 1;
                            }
                            configure_hero_point_effect_emission(0.016f, 0.002f, 5, 2, 1, 0);
                            break;
                        }
                        if (D_L12_X_f.fC < g_hero.target_speed) {
                            if (g_hero.on_ice_ground != 0 || g_hero.wading != 0) {
                                HeroL12_SetMoveState(HERO_STATE_RUN, 1);
                                break;
                            }
                            HeroL12_SetMoveState(HERO_STATE_RUN, 0);
                            if (g_hero.state.current == HERO_STATE_RUN) {
                                g_hero.state.step = 1;
                                set_hero_animation(4, l12_hero_run_start_frame, scale_ticks(0xC));
                            }
                            configure_hero_point_effect_emission(0.016f, 0.002f, 5, 2, 1, 0);
                            break;
                        }
                        if (g_hero.air_frames.s == 0) {
                            if (HeroL12_SetMoveState(HERO_STATE_IDLE, 0) == 0) {
                                break;
                            }
                            set_hero_animation(hero_get_gadget_anim_id(0), 0, -2.0f);
                            configure_hero_point_effect_emission(0.016f, 0.002f, 5, 2, 0, 1);
                            break;
                        }
                        if (HeroL12_SetMoveState(HERO_STATE_IDLE, 0) == 0) {
                            break;
                        }
                        set_hero_animation(7, 0x17, scale_ticks(9));
                        configure_hero_point_effect_emission(0.016f, 0.002f, 5, 2, 0, 1);
                        break;
                    case HERO_STATE_JUMP:
                    case HERO_STATE_RUN_JUMP:
                    case HERO_STATE_PACK_JUMP:
                    case HERO_STATE_HIGH_JUMP:
                    case 12:
                    case 13:
                    case HERO_STATE_DOUBLE_JUMP:
                    case 15:
                    case HERO_STATE_PACK_LONG_JUMP:
                    case 17:
                    case 18:
                    case HERO_STATE_LEDGE_JUMP:
                    case 60:
                    case HERO_STATE_JUMP_FROM_RESPAWN_WAIT:
                        if (g_hero.can_chain_high_jump != 0) {
                            int t = get_hero_high_jump_input_window_frames();
                            if (t > 0 && (g_hero.state_timer < t || g_hero.rail_land_frames != 0) &&
                                g_hero.grounded_frames != 0) {
                                if (g_hero.state.current != HERO_STATE_HIGH_JUMP ||
                                    pad_pressed_within(0x40, scale_ticks(7), 0)) {
                                    int ok = 1;
                                    if (g_hero.state.current == HERO_STATE_LEDGE_JUMP || g_hero.rail_land_frames != 0) {
                                        if (pad_pressed_within(0x40, scale_ticks(7), 0) == 0) {
                                            ok = 0;
                                        }
                                    }
                                    if (g_hero.state.current == HERO_STATE_HIGH_JUMP) {
                                        if (g_hero.rail_land_frames == 0) {
                                            ok = 0;
                                        }
                                    }
                                    if (ok) {
                                        if (pad_pressed_within(0x1F000, scale_ticks(5), 0)) {
                                            if (g_controller_state.held & (PAD_R2 | PAD_R1)) {
                                                HeroL12_SetMoveState(HERO_STATE_HIGH_JUMP, 1);
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        if (hero_slot_item_id(3) == 3 && g_hero.pack_disabled == 0 &&
                            g_hero.state.current != HERO_STATE_PACK_LONG_JUMP && g_hero.state.current != 0x11 &&
                            g_hero.state.current != 0x12 && g_hero.state.current != 0x3C &&
                            g_hero.state.current != 0xF && g_hero.state.current != HERO_STATE_PACK_JUMP &&
                            g_hero.state.current != HERO_STATE_JUMP_FROM_RESPAWN_WAIT &&
                            (g_hero.state.current == HERO_STATE_DOUBLE_JUMP || scale_ticks(9) < g_hero.state_timer) &&
                            (g_controller_state.held & (PAD_R2 | PAD_R1))) {
                            if (pad_pressed_within(0x40, scale_ticks(8), 0)) {
                                HeroL12_SetMoveState(HERO_STATE_PACK_LONG_JUMP, 1);
                                break;
                            }
                        }
                        if (scale_ticks(7) > g_hero.state_timer && pad_pressed_within(0xA, 2, 0) &&
                            g_hero.state.current != HERO_STATE_PACK_LONG_JUMP &&
                            g_hero.state.current != HERO_STATE_PACK_JUMP && g_hero.state.current != 0x12 &&
                            g_hero.state.current != 0x3C && g_hero.state.current != 0x11 &&
                            g_hero.state.current != HERO_STATE_DOUBLE_JUMP &&
                            g_hero.state.current != HERO_STATE_JUMP_FROM_RESPAWN_WAIT) {
                            if (hero_slot_item_id(3) == 2 && item_available[2] != 0 && g_hero.pack_disabled == 0 &&
                                g_frame_time * 0.9f < g_hero.motion.forward_speed) {
                                HeroL12_SetMoveState(HERO_STATE_PACK_JUMP, 1);
                            } else if (hero_slot_item_id(3) == 3 && g_hero.pack_disabled == 0 &&
                                       g_frame_time * 0.9f < g_hero.motion.forward_speed &&
                                       g_hero.long_jump_lockout == 0) {
                                HeroL12_SetMoveState(HERO_STATE_PACK_LONG_JUMP, 1);
                            }
                        }
                        if (g_hero.takeoff_frame + scale_ticks(8) < g_hero.state_timer &&
                            (g_hero.past_jump_apex == 0 || scale_ticks(0xA) < g_hero.frames_to_land) &&
                            g_hero.stick_magnitude < 0.3f &&
                            (g_hero.state.current == HERO_STATE_JUMP || g_hero.state.current == HERO_STATE_RUN_JUMP ||
                             g_hero.state.current == 0xD || g_hero.state.current == 0xF ||
                             g_hero.state.current == HERO_STATE_DOUBLE_JUMP)) {
                            if (pad_pressed_within(0xA, scale_ticks(9), 0)) {
                                if (hero_slot_item_id(3) != 3) {
                                    break;
                                }
                                if (g_hero.pack_disabled != 0) {
                                    break;
                                }
                                HeroL12_SetMoveState(HERO_STATE_RISE_THEN_FALL_WITH_AIRBORNE_HIT_QUERY, 1);
                                break;
                            }
                        }
                        if (g_hero.jump_max_frames < g_hero.state_timer) {
                            if (g_hero.rail_land_frames == 0) {
                                HeroL12_SetMoveState(HERO_STATE_FALL, 1);
                                break;
                            }
                        } else if (g_hero.rail_land_frames == 0 &&
                                   (g_hero.state.current == HERO_STATE_PACK_JUMP ||
                                    g_hero.state.current == HERO_STATE_PACK_LONG_JUMP) &&
                                   g_hero.wall_probe_distance < 0.9f && g_hero.wall_hit != 0) {
                            if (FastVecLength(g_hero.motion.velocity.f) > g_frame_time * 7.0f) {
                                float len = FastVecLength(g_hero.motion.displacement.f);
                                if (len < FastVecLength(g_hero.motion.velocity.f) * 0.5f && g_hero.ledge_found == 0 &&
                                    g_hero.ground_distance < 2.0f) {
                                    HeroL12_SetMoveState(HERO_STATE_WALL_BONK, 1);
                                    break;
                                }
                            }
                        }
                        {
                            int isNear;

                            isNear = 0;
                            if (g_hero.state.current == HERO_STATE_DOUBLE_JUMP && g_hero.anim_frame < 35.0f) {
                                isNear = 1;
                            }
                            if (!(g_hero.state_timer > g_hero.takeoff_frame)) {
                                break;
                            }
                            if (g_hero.state_timer == g_hero.takeoff_frame + 1) {
                                if (g_hero.velocity_keys_active != 0) {
                                    float d = g_hero.unk3D0 - g_hero.state_timer;
                                    if (0.0f < d) {
                                        adjust_hero_animation_speed_for_target_frame(
                                            g_hero.keyed_move_target_frame, d, g_hero.jump_anim_retime_scale, -1.0f);
                                    }
                                    break;
                                }
                                if (g_hero.state.current == HERO_STATE_DOUBLE_JUMP) {
                                    break;
                                }
                                predict_hero_trajectory_z(g_hero.motion.pos.f, g_hero.motion.velocity.f, &sp34,
                                                          g_hero.jump_gravity);
                                adjust_hero_animation_speed_for_target_frame(g_hero.apex_anim_frame, sp34,
                                                                             g_hero.jump_anim_retime_scale, -1.0f);
                                if (1.5f < g_hero.anim_speed) {
                                    g_hero.anim_speed = 1.5f;
                                }
                                break;
                            }
                            if (g_hero.velocity_keys_active != 0 && !(g_hero.state_timer < g_hero.unk3D0) &&
                                g_hero.state_timer < g_hero.velocity_keys_end_frame) {
                                float d = g_hero.keyed_anim_target_frame - g_hero.unk3D0;
                                if (0.0f < d) {
                                    adjust_hero_animation_speed_for_target_frame(g_hero.apex_anim_frame, d,
                                                                                 g_hero.jump_anim_retime_scale, -1.0f);
                                }
                                break;
                            }
                            if (g_hero.in_water_frames == 1) {
                                allocate_hero_voice_for_entry(0x11, 0);
                                emit_hero_surface_effect_burst(3, 0x18, 1);
                            }
                            if (g_hero.rail_land_frames != 0) {
                                g_hero.rail_land_frames = g_hero.rail_land_frames + 1;
                            } else if (g_hero.air_frames.s == 0 && g_hero.past_jump_apex != 0 && !isNear) {
                                int fr;

                                g_hero.rail_land_frames = scale_ticks(1);
                                configure_hero_point_effect_emission(0.016f, 0.002f, 5, 2, 0, 1);
                                fr = truncate_float_to_s32(g_hero.jump_land_anim_frame) + 2;
                                if (g_hero.target_anim_frame - 1.5f > g_hero.anim_frame) {
                                    set_hero_animation(return_hero_animation_id_unchanged(moby[0x53]), fr,
                                                       scale_ticks(7));
                                }
                            }
                            if (g_hero.rail_land_frames != 0) {
                                g_hero.anim_speed = 1.0f;
                            } else if (g_hero.past_jump_apex != 0) {
                                if (moby[0x51] < truncate_float_to_s32(g_hero.jump_land_anim_frame)) {
                                    if (g_hero.state.current == HERO_STATE_DOUBLE_JUMP && g_hero.anim_frame < 37.0f) {
                                        ApproachValue(&g_hero.anim_speed, 1.0f, g_frame_scale_sq * 0.15f);
                                    } else {
                                        int r = truncate_float_to_s32(
                                            predict_hero_trajectory_intercept_time(60.0f, g_hero.jump_gravity, -1.0f));
                                        g_hero.frames_to_land = r;
                                        adjust_hero_animation_speed_for_target_frame(g_hero.target_anim_frame, r,
                                                                                     g_hero.jump_anim_retime_scale,
                                                                                     g_frame_scale_sq * 0.2f);
                                    }
                                    if (!isNear) {
                                        int fr = truncate_float_to_s32(g_hero.jump_land_anim_frame) + 2;
                                        if (g_hero.anim_speed > 1.8f) {
                                            set_hero_animation(return_hero_animation_id_unchanged(moby[0x53]), fr,
                                                               g_hero.frames_to_land + 3);
                                        }
                                    }
                                    if (0.2f > g_hero.anim_speed) {
                                        g_hero.anim_speed = 0.2f;
                                    }
                                    if (g_hero.anim_speed > 2.7f) {
                                        g_hero.anim_speed = 2.7f;
                                    }
                                }
                                if (g_hero.state.current == HERO_STATE_RUN_JUMP &&
                                    g_hero.target_anim_frame - g_hero.anim_frame > 6.0f &&
                                    (return_hero_animation_id_unchanged(moby[0x52]) == 8 ||
                                     return_hero_animation_id_unchanged(moby[0x52]) == 9) &&
                                    moby[0x52] == moby[0x53] &&
                                    D_L12_X_f.f18 * g_frame_time * 0.1f > g_hero.target_speed &&
                                    g_frame_time * 2.5f > g_hero.motion.flat_speed) {
                                    float sp = (float)scale_ticks(9) / g_hero.anim_speed;
                                    if ((float)scale_ticks(0xA) < sp) {
                                        sp = scale_ticks(0xA);
                                    }
                                    set_hero_animation(7, (truncate_float_to_s32(g_hero.anim_frame) + 4) / 2, sp);
                                    g_hero.state.current = HERO_STATE_JUMP;
                                }
                                if (truncate_float_to_s32(g_hero.anim_frame) ==
                                    truncate_float_to_s32(g_hero.target_anim_frame)) {
                                    g_hero.anim_speed = 0.0f;
                                }
                            }
                        }
                        if (g_hero.rail_land_frames != 0) {
                            if (g_hero.state.current == HERO_STATE_PACK_JUMP) {
                                if (HeroL12_SetMoveState(HERO_STATE_STOP, 0) == 0) {
                                    break;
                                }
                                set_hero_animation(5, 0, -2.0f);
                                FastVecScale(g_hero.motion.momentum.f, g_hero.motion.momentum.f, 0.8f);
                                break;
                            }
                            if (g_hero.on_slip_ground == 0 && g_hero.move_lock_timer == 0) {
                                if (pad_pressed_within(0x40, scale_ticks(8), 0)) {
                                    if (g_hero.air_frames.s < scale_ticks(4)) {
                                        if (g_hero.target_speed > D_L12_X_f.fC &&
                                            FastDiffRots(g_hero.motion.move_yaw, g_hero.motion.rot.f[2]) < 1.0471976f &&
                                            D_L12_X_f.f18 * g_frame_time * 0.85f < g_hero.motion.flat_speed) {
                                            HeroL12_SetMoveState(HERO_STATE_RUN_JUMP, 1);
                                            break;
                                        }
                                        HeroL12_SetMoveState(HERO_STATE_JUMP, 1);
                                        break;
                                    }
                                }
                            }
                        }
                        if (g_hero.rail_land_frames > scale_ticks(1) && g_hero.state.current != HERO_STATE_PACK_JUMP) {
                            update_hero_locomotion_state();
                        } else if (g_hero.past_jump_apex != 0 && !(g_hero.fall_delay_frames > g_hero.state_timer) &&
                                   g_hero.ground_distance > g_hero.fall_start_distance) {
                            HeroL12_SetMoveState(HERO_STATE_FALL, 1);
                        }
                        if (g_hero.state.current == HERO_STATE_PACK_LONG_JUMP &&
                            scale_ticks(0x3C) < g_hero.state_timer && g_hero.ground_distance > 1.5f) {
                            HeroL12_SetMoveState(HERO_STATE_FALL, 1);
                            break;
                        }
                        {
                            int hit = 0;
                            if (g_hero.state.current == HERO_STATE_JUMP ||
                                g_hero.state.current == HERO_STATE_RUN_JUMP || g_hero.state.current == 0x11) {
                                hit = 1;
                            }
                            if (g_hero.wall_jump_window != 0 && g_hero.rail_land_frames == 0 && hit) {
                                if (g_hero.state.current != 0x11 || scale_ticks(0x19) < g_hero.state_timer) {
                                    int a = scale_ticks(7);
                                    if (g_hero.state.current == 0x11) {
                                        a = scale_ticks(7);
                                    }
                                    if (pad_pressed_within(0x40, a, &sp38)) {
                                        if (sp38 < g_hero.state_timer) {
                                            HeroL12_SetMoveState(0x11, 1);
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        if (g_hero.state.current == HERO_STATE_PACK_JUMP ||
                            g_hero.state.current == HERO_STATE_PACK_LONG_JUMP) {
                            if (scale_ticks(0x19) < g_hero.state_timer && g_hero.on_flagged_ground_moby != 0) {
                                g_hero.unk4A6 = scale_ticks(0x14);
                            }
                        }
                        {
                            int delay;
                            int n;

                            delay = scale_ticks(0x14);
                            if (g_hero.state.current == HERO_STATE_PACK_LONG_JUMP) {
                                delay = scale_ticks(8);
                            }
                            if (g_hero.state.current == HERO_STATE_DOUBLE_JUMP && g_hero.ground_distance > 4.0f) {
                                if (g_hero.past_jump_apex == 0 && g_hero.anim_speed < 1.3f) {
                                    ApproachValue(&g_hero.anim_speed, 1.3f, frame_scale * 0.1f);
                                }
                                delay = scale_ticks(0x11);
                                g_hero.unk4A6 = delay;
                            }
                            n = g_hero.state_timer - 4;
                            if (n < 2) {
                                n = 2;
                            }
                            if (scale_ticks(0x1E) < n) {
                                n = scale_ticks(0x1E);
                            }
                            if (g_hero.rail_land_frames == 0 && g_hero.state.current != 0x12 &&
                                g_hero.state.current != HERO_STATE_LEDGE_JUMP && g_hero.state.current != 0x11 &&
                                g_hero.state.current != HERO_STATE_HIGH_JUMP && g_hero.state.current != 0x3C &&
                                g_hero.state.current != HERO_STATE_JUMP_FROM_RESPAWN_WAIT) {
                                if (pad_pressed_within(0x40, n, 0) || g_hero.state.current == HERO_STATE_DOUBLE_JUMP ||
                                    g_hero.state.current == 0xF || g_hero.state.current == 0xD) {
                                    if (g_hero.state.current != HERO_STATE_DOUBLE_JUMP &&
                                        g_hero.item_anim_active == 0 &&
                                        (g_hero.on_slip_ground == 0 || !(g_hero.ground_distance < 0.4f)) &&
                                        g_hero.can_double_jump != 0 && scale_ticks(0xE) < g_hero.state_timer &&
                                        (g_hero.past_jump_apex == 0 ||
                                         (g_hero.ground_distance > 0.7f &&
                                          absolute_float(gravity_z(g_hero.motion.velocity.f)) < g_frame_time * 5.8f))) {
                                        HeroL12_SetMoveState(HERO_STATE_DOUBLE_JUMP, 1);
                                    } else if (item_available[2] != 0 && hero_slot_item_id(3) != 4 &&
                                               g_hero.pack_disabled == 0 && (g_controller_state.held & PAD_CROSS) &&
                                               delay < g_hero.state_timer && g_hero.unk4A6 < g_hero.state_timer) {
                                        float lim = 0.6f;
                                        if (g_hero.state.current == HERO_STATE_RUN_JUMP ||
                                            g_hero.state.current == HERO_STATE_JUMP) {
                                            lim = 1.0f;
                                        }
                                        if (g_hero.state.current == HERO_STATE_DOUBLE_JUMP) {
                                            lim = 1.5f;
                                        }
                                        if (lim < g_hero.ground_distance) {
                                            HeroL12_SetMoveState(HERO_STATE_LAND, 1);
                                        }
                                    }
                                }
                            }
                        }
                        if (g_hero.ledge_found != 0 && g_hero.state.current != HERO_STATE_HIGH_JUMP) {
                            HeroL12_SetMoveState(HERO_STATE_LEDGE_HANG_ENTRY, 1);
                        }
                        break;
                    case HERO_STATE_HURT:
                        if (12.5f < g_hero.anim_frame && g_hero.air_frames.s != 0 && g_hero.ground_distance > 1.0f) {
                            HeroL12_SetMoveState(HERO_STATE_FALL, 1);
                            break;
                        }
                        if (g_hero.state_timer > scale_ticks(0x1B)) {
                            if (g_hero.health.hp <= 0) {
                                HeroL12_SetMoveState(HERO_STATE_DYING, 1);
                                break;
                            }
                            if (g_hero.state.prev == HERO_STATE_DELAYED_JUMP_OR_R1_REPEAT_EXIT_WAIT) {
                                HeroL12_SetMoveState(HERO_STATE_DELAYED_JUMP_OR_R1_REPEAT_EXIT_WAIT, 1);
                                break;
                            }
                            HeroL12_SetMoveState(HERO_STATE_IDLE, 0);
                        }
                        break;
                    case HERO_STATE_VEHICLE_MODE_3_IDLE:
                        if (g_hero.anim_flags & 2) {
                            if (((unsigned char *)g_hero.moby)[0x53] != 0) {
                                set_hero_animation(0, 0, scale_ticks(0xC));
                            } else if (random_float_between(0.0f, 1.0f) < 0.5f) {
                                set_hero_animation(5, 0, scale_ticks(8));
                            }
                        }
                        if (g_hero.on_slip_ground != 0) {
                            HeroL12_SetMoveState(HERO_STATE_VEHICLE_MODE_3_SLIP, 1);
                            break;
                        }
                        if (g_controller_state.held & (PAD_L2 | PAD_L1)) {
                            HeroL12_SetMoveState(HERO_STATE_LOOK, 1);
                            break;
                        }
                        if (g_hero.air_frames.s != 0) {
                            HeroL12_SetMoveState(HERO_STATE_VEHICLE_MODE_3_FALL, 1);
                            break;
                        }
                        if (g_controller_state.pressed & PAD_CIRCLE) {
                            HeroL12_SetMoveState(HERO_STATE_DELAYED_CIRCLE_RELEASE_WAIT, 1);
                            break;
                        }
                        if (pad_pressed_within(0x80, scale_ticks(7), 0)) {
                            if (g_hero.vehicle_toggle_timer == 0) {
                                allocate_hero_voice_for_entry(0x19, 0);
                                g_hero.vehicle_toggle_timer = 0x12;
                                g_hero.unk22DC = 1;
                            }
                            hero_switch_item(0);
                            break;
                        }
                        stick_target_v(0, 1.0f);
                        if (g_hero.stick_magnitude > 0.22f && g_hero.target_speed > 0.0f) {
                            HeroL12_SetMoveState(HERO_STATE_VEHICLE_MODE_3_RUN, 1);
                        }
                        break;
                    case HERO_STATE_VEHICLE_MODE_3_SLIP:
                        if (scale_ticks(0x12C) < g_hero.state_timer) {
                            hero_respawn();
                        }
                        if (g_hero.on_slip_ground != 0) {
                            break;
                        }
                        if (g_hero.air_frames.s != 0) {
                            HeroL12_SetMoveState(HERO_STATE_VEHICLE_MODE_3_FALL, 1);
                            break;
                        }
                        HeroL12_SetMoveState(HERO_STATE_VEHICLE_MODE_3_IDLE, 1);
                        break;
                    case HERO_STATE_DELAYED_CIRCLE_RELEASE_WAIT:
                        if (g_hero.on_slip_ground != 0 && g_hero.ground_distance < 0.3f) {
                            HeroL12_SetMoveState(HERO_STATE_VEHICLE_MODE_3_SLIP, 1);
                            break;
                        }
                        if (g_hero.state_timer > scale_ticks(0x2D) && !(g_controller_state.held & PAD_CIRCLE)) {
                            HeroL12_SetMoveState(HERO_STATE_VEHICLE_MODE_3_IDLE, 0);
                            set_hero_animation(0, 0, scale_ticks(0x14));
                        }
                        break;
                    case HERO_STATE_VEHICLE_MODE_3_FALL:
                        if (g_hero.air_frames.s != 0) {
                            break;
                        }
                        if (g_frame_time * -9.0f > g_hero.motion.displacement.f[2]) {
                            g_hero.motion.displacement.f[2] = g_frame_time * -9.0f;
                        }
                        if (g_frame_time * -9.0f > g_hero.motion.velocity.f[2]) {
                            g_hero.motion.velocity.f[2] = g_frame_time * -9.0f;
                        }
                        if (g_hero.health.hp <= 0) {
                            HeroL12_SetMoveState(HERO_STATE_VEHICLE_MODE_3_DYING, 1);
                            break;
                        }
                        if (g_hero.stick_magnitude > 0.5f && scale_ticks(0x28) > g_hero.state_timer &&
                            g_hero.move_lock_timer == 0) {
                            HeroL12_SetMoveState(HERO_STATE_VEHICLE_MODE_3_RUN, 1);
                            break;
                        }
                        HeroL12_SetMoveState(HERO_STATE_VEHICLE_MODE_3_IDLE, 0);
                        set_hero_animation(7, 0, -1.0f);
                        g_hero.anim_speed = 0.35f;
                        g_hero.move_lock_timer = scale_ticks(0x14);
                        break;
                    case HERO_STATE_VEHICLE_MODE_3_RUN:
                        g_hero.anim_speed = g_hero.motion.speed_xy * 54.0f;
                        if (g_hero.on_slip_ground != 0 && g_hero.ground_distance < 0.2f) {
                            HeroL12_SetMoveState(HERO_STATE_VEHICLE_MODE_3_SLIP, 1);
                            break;
                        }
                        if (!(g_hero.air_frames.s < scale_ticks(5))) {
                            if (g_hero.ground_distance > 0.8f || g_hero.ground_slope.f > 0.87266463f) {
                                HeroL12_SetMoveState(HERO_STATE_VEHICLE_MODE_3_FALL, 1);
                                break;
                            }
                        }
                        if (pad_pressed_within(0x80, scale_ticks(7), 0)) {
                            if (g_hero.vehicle_toggle_timer == 0) {
                                allocate_hero_voice_for_entry(0x19, 0);
                                g_hero.vehicle_toggle_timer = 0x12;
                                g_hero.unk22DC = 1;
                            }
                            hero_switch_item(0);
                            break;
                        }
                        if (g_controller_state.pressed & PAD_CIRCLE) {
                            HeroL12_SetMoveState(HERO_STATE_DELAYED_CIRCLE_RELEASE_WAIT, 1);
                            break;
                        }
                        if (g_hero.air_frames.s == 0 && g_hero.stick_magnitude < 0.17f) {
                            HeroL12_SetMoveState(HERO_STATE_VEHICLE_MODE_3_IDLE, 1);
                        }
                        break;
                    case HERO_STATE_VEHICLE_MODE_3_HURT:
                        if (!(g_hero.state_timer < scale_ticks(0x20)) && g_hero.air_frames.s != 0 &&
                            g_hero.ground_distance > 1.0f) {
                            HeroL12_SetMoveState(HERO_STATE_VEHICLE_MODE_3_FALL, 1);
                            break;
                        }
                        if (g_hero.anim_flags & 2) {
                            HeroL12_SetMoveState(HERO_STATE_VEHICLE_MODE_3_IDLE, 1);
                            break;
                        }
                        if (scale_ticks(0x2D) < g_hero.state_timer) {
                            if (g_hero.health.hp <= 0) {
                                HeroL12_SetMoveState(HERO_STATE_VEHICLE_MODE_3_DYING, 1);
                                break;
                            }
                            HeroL12_SetMoveState(HERO_STATE_VEHICLE_MODE_3_IDLE, 0);
                        }
                        break;
                    case HERO_STATE_LOOK_RELEASE_WAIT:
                        if (g_hero.vehicle_mode == 0) {
                            if (g_hero.unk20AA == 0 || g_hero.unk20A5 != 0) {
                                if (g_controller_state.held & (PAD_R2 | PAD_R1)) {
                                    if (((unsigned char *)g_hero.moby)[0x53] != 0xD) {
                                        set_hero_animation(0xD, 0, scale_ticks(8));
                                    }
                                } else {
                                    int a = hero_get_gadget_anim_id(1);
                                    if (((unsigned char *)g_hero.moby)[0x53] != a) {
                                        set_hero_animation(a, 0, scale_ticks(8));
                                    }
                                }
                            }
                        }
                        if (g_controller_state.held & (PAD_L2 | PAD_L1)) {
                            break;
                        }
                        if (g_hero.state.current == HERO_STATE_LOOK_RELEASE_WAIT) {
                            int a;

                            HeroL12_SetMoveState(HERO_STATE_IDLE, 0);
                            a = hero_get_gadget_anim_id(1);
                            if (((unsigned char *)g_hero.moby)[0x53] != a) {
                                set_hero_animation(a, 0, scale_ticks(8));
                            }
                        }
                        hoven_look_release_reset_word = 0;
                        break;
                    case HERO_STATE_HELD_ITEM_ATTACK:
                        if (((unsigned char *)g_hero.items[0].moby) != 0 && g_hero.anim_changing == 0 &&
                            g_hero.anim_frame > 20.0f) {
                            HeroL12_SetMoveState(HERO_STATE_IDLE, 0);
                        }
                        break;
                    case HERO_STATE_ANIMATION_RECOVERY_WITH_JUMP_CANCEL:
                        if (19.0f < g_hero.anim_frame && pad_pressed_within(0x40, scale_ticks(0xB), 0)) {
                            if (g_controller_state.held & (PAD_R2 | PAD_R1)) {
                                hero_handle_jump_press();
                                break;
                            }
                            HeroL12_SetMoveState(HERO_STATE_JUMP, 1);
                            break;
                        }
                        if (21.0f <= g_hero.anim_frame) {
                            HeroL12_SetMoveState(HERO_STATE_IDLE, 0);
                        }
                        break;
                }
            }
        }
    }
    if (prevSub != g_hero.state.control_mode) {
        release_and_clear_hero_voice_slots();
    }
    if ((g_hero.state.prev == HERO_STATE_FIDGET && g_hero.state.current != HERO_STATE_FIDGET_ALT) ||
        (g_hero.state.prev == HERO_STATE_FIDGET_ALT && g_hero.unk1614 != 0)) {
        remove_hero_cached_moby_and_clear_slot_three_moby_flags();
    }
    if (prevState != g_hero.state.step) {
        g_hero.step_frames = 0;
    }
    if (prevAnim != moby[0x53]) {
        g_hero.anim_seq_frames = 0;
    }
}

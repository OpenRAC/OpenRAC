void gold_bolt_update(Moby *moby) {
    Vars_00309078 *vars = (Vars_00309078 *)moby->pvars;
    Hero *h;
    Vec4f at;
    Vec4f look;
    Vec4f at2;
    Vec4f look2;
    int i;

    switch (moby->state) {
        case 0:
            if (vars->id == -1 || gold_bolts_collected_r[vars->id + g_current_level_index_r * 4] != 0) {
                goto remove;
            }
            qcopy(&vars->home, &moby->pos);
            vars->home.w = moby->rot.z;
            moby->rot.x = random_angle_radians();
            moby->rot.y = random_angle_radians();
            moby->rot.z = random_angle_radians();
            moby->rot.w = random_angle_radians();
            vars->unk68 = D_L01_00161FB0_00309078 * 0.017453292f * g_frame_time_r;
            vars->unk64 = D_L01_00161FB4_00309078 * 0.017453292f * g_frame_time_r;
            vars->unk60 = D_L01_00161FB8_00309078 * 0.017453292f * g_frame_time_r;
            moby->state = 1;
            initialize_gold_bolt_data_r(moby);
            return;
        case 1:
            moby->scale = ((Class_00309078 *)moby->pclass)->scale * D_L01_00161FA0_00309078;
            moby->pos.z = vars->home.z + D_L01_00161FA4_00309078 + D_L01_00161FA8_00309078 * FastSin(moby->rot.w);
            moby->rot.x = FastAddRots(moby->rot.x, D_L01_00161FB0_00309078 * 0.017453292f * g_frame_time_r);
            moby->rot.y = FastAddRots(moby->rot.y, D_L01_00161FB4_00309078 * 0.017453292f * g_frame_time_r);
            moby->rot.z = FastAddRots(moby->rot.z, D_L01_00161FB8_00309078 * 0.017453292f * g_frame_time_r);
            moby->rot.w = FastAddRots(moby->rot.w, D_L01_00161FBC_00309078 * 0.017453292f * g_frame_time_r);
            update_gold_bolt_glow_rings_r(moby);
            if (!random_integer_below(D_L01_00161FE0_00309078)) {
                spawn_gold_bolt_particles_r(moby);
            }
            if (!(vec_distance_xy(&moby->pos, &hero_00309078.motion.pos) < 3.0f)) {
                return;
            }
            if (!(absolute_float(moby->pos.z - hero_00309078.motion.pos.f[2]) < 2.0f)) {
                return;
            }
            if (hero_00309078.health.hp == 0) {
                return;
            }
            if (hero_00309078.vehicle_mode != 3 && hero_00309078.vehicle_mode != 0) {
                return;
            }
            if (vars->unkC != 0) {
                novalis_banner_display_ticks_r = scale_ticks(180);
                queue_dialogue_message(0x540A, -1);
                gold_bolts_collected_r[vars->id + g_current_level_index_r * 4] = 1;
                save_00309078(0, -1);
            remove:
                DeleteMoby(moby);
                return;
            }
            at.x = FastCos(vars->home.w) * 2.5f;
            at.y = FastSin(vars->home.w) * 2.5f;
            at.z = 0.0f;
            FastVecAdd(&at, &at, &vars->home);
            clear_u64_value(&look);
            look.z = FastAddRots(vars->home.w, 3.1415927f);
            FadeToBlack(scale_ticks(10));
            if (hero_00309078.vehicle_mode == 3) {
                exit_hero_vehicle_mode_and_restore_moby();
            }
            set_race_camera(&at, &look, 0x72, 0);
            voice_00309078(0, 0, moby);
            at2.x = FastCos(vars->home.w) * 1.25f;
            at2.y = FastSin(vars->home.w) * 1.25f;
            at2.z = 0.0f;
            at2.x += FastCos(FastAddRots(vars->home.w, 1.5707964f)) * 4.0f;
            at2.y += FastSin(FastAddRots(vars->home.w, 1.5707964f)) * 4.0f;
            FastVecAdd(&at2, &at2, &vars->home);
            at2.z += 1.0f;
            clear_u64_value(&look2);
            look2.z = FastSubRots(vars->home.w, 1.5707964f);
            jet_mount(&at2, &look2, 1, 0, 0);
            set_camera_position(&at2);
            set_camera_rotation(&look2);
            qcopy(&moby->pos, &hero_00309078.motion.pos);
            qcopy(&moby->rot, &hero_00309078.motion.rot);
            if (moby->prev_seq != 1) {
                set_seq_00309078(moby, 1, 0, 0);
            }
            set_hero_animation_r(0.0f, 0x82, 0);
            if (draw_map51_r[1]) {
                moby->flags |= 0x8000;
            }
            hero_00309078.unk20AF = 1;
            cutscene_active_r = 1;
            moby->state = 2;
            vars->timer = scale_ticks(60);
            return;
        case 2:
            update_gold_bolt_pickup_camera_r(moby);
            qcopy(&moby->pos, &hero_00309078.motion.pos);
            qcopy(&moby->rot, &hero_00309078.motion.rot);
            if (moby->seq == 1) {
                update_gold_bolt_visuals_r(moby);
                if (!random_integer_below(10)) {
                    spawn_gold_bolt_particles_r(moby);
                }
                if (moby->anim_flags & 2) {
                    if (moby->prev_seq != 0) {
                        set_seq_00309078(moby, 0, 0, 0);
                    }
                    moby->visible = 0;
                    moby->flags |= 1;
                    for (i = 0; i < 4; i++) {
                        spawn_gold_bolt_particles_r(moby);
                    }
                }
            }
            h = &hero_00309078;
            if (h->anim_flags & 2) {
                set_hero_animation_r(scale_ticks(30), 0, 0);
            }
            if (moby->seq != 0 || !countdown_00309078(&vars->timer)) {
                return;
            }
            novalis_banner_display_ticks_r = scale_ticks(180);
            queue_dialogue_message(0x540A, -1);
            gold_bolts_collected_r[vars->id + g_current_level_index_r * 4] = 1;
            cutscene_active_r = 0;
            set_race_camera(&h->motion.pos, &h->motion.rot, 0, 0);
            jet_end_mount(0);
            save_00309078(0, -1);
            DeleteMoby(moby);
            break;
    }
}

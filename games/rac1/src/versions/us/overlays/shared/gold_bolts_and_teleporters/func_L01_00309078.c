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
        moby->rot.x = rand_angle_00309078();
        moby->rot.y = rand_angle_00309078();
        moby->rot.z = rand_angle_00309078();
        moby->rot.w = rand_angle_00309078();
        vars->unk68 = D_L01_00161FB0_00309078 * 0.017453292f * g_frame_time_r;
        vars->unk64 = D_L01_00161FB4_00309078 * 0.017453292f * g_frame_time_r;
        vars->unk60 = D_L01_00161FB8_00309078 * 0.017453292f * g_frame_time_r;
        moby->state = 1;
        initialize_gold_bolt_data_r(moby);
        return;
    case 1:
        moby->scale = ((Class_00309078 *)moby->pclass)->scale * D_L01_00161FA0_00309078;
        moby->pos.z = vars->home.z + D_L01_00161FA4_00309078 + D_L01_00161FA8_00309078 * sin_00309078(moby->rot.w);
        moby->rot.x = add_rot_00309078(moby->rot.x, D_L01_00161FB0_00309078 * 0.017453292f * g_frame_time_r);
        moby->rot.y = add_rot_00309078(moby->rot.y, D_L01_00161FB4_00309078 * 0.017453292f * g_frame_time_r);
        moby->rot.z = add_rot_00309078(moby->rot.z, D_L01_00161FB8_00309078 * 0.017453292f * g_frame_time_r);
        moby->rot.w = add_rot_00309078(moby->rot.w, D_L01_00161FBC_00309078 * 0.017453292f * g_frame_time_r);
        update_gold_bolt_glow_rings_r(moby);
        if (!rand_below_00309078(D_L01_00161FE0_00309078)) {
            spawn_gold_bolt_particles_r(moby);
        }
        if (!(dist_00309078(&moby->pos, &hero_00309078.motion.pos) < 3.0f)) {
            return;
        }
        if (!(fabs_00309078(moby->pos.z - hero_00309078.motion.pos.f[2]) < 2.0f)) {
            return;
        }
        if (hero_00309078.health.hp == 0) {
            return;
        }
        if (hero_00309078.vehicle_mode != 3 && hero_00309078.vehicle_mode != 0) {
            return;
        }
        if (vars->unkC != 0) {
            novalis_banner_display_ticks_r = ticks_00309078(180);
            ShowBanner_r(0x53B3, -1);
            gold_bolts_collected_r[vars->id + g_current_level_index_r * 4] = 1;
            save_00309078(0, -1);
        remove:
            remove_00309078(moby);
            return;
        }
        at.x = cos_00309078(vars->home.w) * 2.5f;
        at.y = sin_00309078(vars->home.w) * 2.5f;
        at.z = 0.0f;
        add_vec_00309078(&at, &at, &vars->home);
        clear_vec_00309078(&look);
        look.z = add_rot_00309078(vars->home.w, 3.1415927f);
        FadeToBlack_r(ticks_00309078(10));
        if (hero_00309078.vehicle_mode == 3) {
            exit_hero_vehicle_mode_and_restore_moby_r();
        }
        Hero_ResetToPosition_r(&at, &look, 0x72, 0);
        voice_00309078(0, 0, moby);
        at2.x = cos_00309078(vars->home.w) * 1.25f;
        at2.y = sin_00309078(vars->home.w) * 1.25f;
        at2.z = 0.0f;
        at2.x += cos_00309078(add_rot_00309078(vars->home.w, 1.5707964f)) * 4.0f;
        at2.y += sin_00309078(add_rot_00309078(vars->home.w, 1.5707964f)) * 4.0f;
        add_vec_00309078(&at2, &at2, &vars->home);
        at2.z += 1.0f;
        clear_vec_00309078(&look2);
        look2.z = sub_rot_00309078(vars->home.w, 1.5707964f);
        CameraScript_r(&at2, &look2, 1, 0, 0);
        set_camera_position_r(&at2);
        set_camera_rotation_r(&look2);
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
        vars->timer = ticks_00309078(60);
        return;
    case 2:
        update_gold_bolt_pickup_camera_r(moby);
        qcopy(&moby->pos, &hero_00309078.motion.pos);
        qcopy(&moby->rot, &hero_00309078.motion.rot);
        if (moby->seq == 1) {
            update_gold_bolt_visuals_r(moby);
            if (!rand_below_00309078(10)) {
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
            set_hero_animation_r(ticks_00309078(30), 0, 0);
        }
        if (moby->seq != 0 || !countdown_00309078(&vars->timer)) {
            return;
        }
        novalis_banner_display_ticks_r = ticks_00309078(180);
        ShowBanner_r(0x53B3, -1);
        gold_bolts_collected_r[vars->id + g_current_level_index_r * 4] = 1;
        cutscene_active_r = 0;
        Hero_ResetToPosition_r(&h->motion.pos, &h->motion.rot, 0, 0);
        CameraScript2_r(0);
        save_00309078(0, -1);
        remove_00309078(moby);
        break;
    }
}

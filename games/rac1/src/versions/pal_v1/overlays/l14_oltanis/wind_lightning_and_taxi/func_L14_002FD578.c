void update_enemy_fighter(struct Moby *moby) {
    Class908Vars_FD578 *data = (Class908Vars_FD578 *)moby->pvars;
    int hit = MobyGetHitMessage_r(moby, 0x800000, 0);
    int index;
    int path_index;
    char *path;
    moby->hit_slot = 0xff;
    if (hit && moby->state != 2) {
        if (g_current_level_index_r == 14) {
            fighter_skill_hit_count_r = fighter_skill_hit_count_r + 1;
            if (fighter_skill_hit_count_r > 2 && D_0013D510_FD578.flag == 0) {
                D_0013D510_FD578.flag = 1;
                PlayBankSound_r(1, 0, 0);
                ShowBanner_r(0x53D6, -1);
            }
        }
        moby->state = 2;
        SpawnExplosionFx_r(5.0f, 13.0f, moby, &moby->pos, -1);
        moby->collision = 0;
        moby->flags |= 0x41;
        index = data->sound;
        if (index != -1) {
            char *entry = voice_pool_r2 + index * 0x70;
            if (*(struct Moby **)(entry + 0x88) == moby && *(unsigned char *)(entry + 0x74) != 0)
                Sound_StopSlot_r2(index);
        }
        data->sound = -1;
    }
    switch (moby->state) {
    case 0:
        path_index = data->path;
        if (path_index == -1) {
            STUB_printf_r2(enemy_fighter_no_path_msg_r, moby->save_id);
            DeleteMoby_r2(moby);
            return;
        }
        if (*(int *)oltanis_paths_r3[path_index] == 0) {
            STUB_printf_r2(enemy_fighter_path_unset_msg_r, moby->save_id);
            DeleteMoby_r2(moby);
            return;
        }
        initialize_flight_path_state_r(moby);
        moby->state = 1;
        moby->update_dist = 0xff;
        moby->draw_dist = 0xff;
        data->sound = -1;
        data->timer = scale_ticks_r2(D_L14_0016202C_FD578);
        break;
    case 1:
        advance_enemy_fighter_along_flight_path_r(moby);
        break;
    case 2:
      if (fast_dec_timer_r(&data->timer)) {
        path = oltanis_paths_r3[data->path];
        moby->flags &= 0xffbe;
        moby->collision = moby->pclass->collision;
        data->timer = scale_ticks_r2(D_L14_0016202C_FD578);
        moby->state = 1;
        data->segment = 0;
        data->along = 0;
        qcopy(&moby->pos, path + 0x10);
        moby->rot.z = fast_atan2_r(*(float *)(path + 0x20) - *(float *)(path + 0x10),
                                              *(float *)(path + 0x24) - *(float *)(path + 0x14));
        data->f80 = data->f84 =
            data->f88 = data->f70 =
            data->f74 = data->f78 = 0.0f;
      }
      break;
    }
    spawn_enemy_fighter_exhaust_r(moby);
    if (moby->state != 0 && moby->state != 2 &&
        Sound_IsPlaying_r2(moby, data->sound) == 0)
        data->sound = Moby_PlaySound_r3(0, 4, moby);
}

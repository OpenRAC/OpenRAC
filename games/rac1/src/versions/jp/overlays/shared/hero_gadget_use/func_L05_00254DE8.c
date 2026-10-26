void update_hero_hoverboard_race_progress(void) {
    float v0[4] __attribute__((aligned(16)));
    float v1[4] __attribute__((aligned(16)));
    Data_254DE8 *d;
    Path_254DE8 *path;
    int near;
    int j;
    int k;
    int m;
    float ang;
    float dir;

    ((Hero *)(hero_globals_base + 0xE1D))->race_time_frames++;
    if (((Moby *)((Hero *)(hero_globals_base + 0xE1D))->hoverboard_moby) == 0) {
        return;
    }
    d = (Data_254DE8 *)((Moby *)((Hero *)(hero_globals_base + 0xE1D))->hoverboard_moby)->pvars;
    if (d->f20 == -1) {
        return;
    }
    path = rilgar_paths_r2[d->f20];
    ((Hero *)(hero_globals_base + 0xE1D))->race_path = (void *)path;
    if (g_current_level_index_r == 0x10) {
        *(u128_254DE8 *)v0 = 0;
        v0[0] = 163.0f;
        v0[1] = 119.0f;
        v0[2] = 80.0f;
        if (FastVecDist_r2(((Hero *)(hero_globals_base + 0xE1D))->motion.pos.f, v0) < 11.0f) {
            if (((Hero *)(hero_globals_base + 0xE1D))->race_path_node < 3 || ((Hero *)(hero_globals_base + 0xE1D))->race_path_node > 0x46) {
                if (((Hero *)(hero_globals_base + 0xE1D))->race_path_node >= 3) {
                    ((Hero *)(hero_globals_base + 0xE1D))->race_lap++;
                }
                ((Hero *)(hero_globals_base + 0xE1D))->race_path_node = 0xD;
            }
        }
        *(u128_254DE8 *)v1 = 0;
        v1[0] = 223.0f;
        v1[1] = 281.0f;
        v1[2] = 93.0f;
        if (FastVecDist_r2(((Hero *)(hero_globals_base + 0xE1D))->motion.pos.f, v1) < 7.0f) {
            if (((Hero *)(hero_globals_base + 0xE1D))->race_path_node < 0x3B) {
                ((Hero *)(hero_globals_base + 0xE1D))->race_path_node = 0x3B;
            }
        }
    }
    {
        int i;
        int n1;
        int next;

        i = ((Hero *)(hero_globals_base + 0xE1D))->race_path_node;
        n1 = path->n + 1;
        next = (i + n1) % path->n;
        ang = fast_atan2_r2(path->p[next].x - path->p[i].x, path->p[next].y - path->p[i].y);
        if (FastDiffRots_r(ang, fast_atan2_r2(((Hero *)(hero_globals_base + 0xE1D))->motion.pos.f[0] - path->p[i].x, ((Hero *)(hero_globals_base + 0xE1D))->motion.pos.f[1] - path->p[i].y)) < 1.5707964f) {
            ((Hero *)(hero_globals_base + 0xE1D))->race_path_node = next;
            if (next == 0) {
                ((Hero *)(hero_globals_base + 0xE1D))->race_lap++;
            }
        }
    }
    near = 0;
    if (g_current_level_index_r == 0x10) {
        if (((Hero *)(hero_globals_base + 0xE1D))->motion.pos.f[2] < path->p[((Hero *)(hero_globals_base + 0xE1D))->race_path_node].z - 2.0f) {
            near = 1;
            if (((Hero *)(hero_globals_base + 0xE1D))->off_course_frames < scale_ticks(0x46)) {
                ((Hero *)(hero_globals_base + 0xE1D))->off_course_frames = scale_ticks(0x46);
            }
        }
    }
    {
        int i;
        int n1;
        int next;

        i = ((Hero *)(hero_globals_base + 0xE1D))->wrong_way_path_point;
        n1 = path->n + 1;
        next = (i + n1) % path->n;
        dir = fast_atan2_r2(path->p[next].x - path->p[i].x, path->p[next].y - path->p[i].y);
    }
    if (near != 0 || (FastDiffRots_r(((Hero *)(hero_globals_base + 0xE1D))->motion.rot.f[2], dir) > 1.5707964f && ((Hero *)(hero_globals_base + 0xE1D))->trick_input == 0)) {
        ((Hero *)(hero_globals_base + 0xE1D))->off_course_frames++;
        if (((Hero *)(hero_globals_base + 0xE1D))->off_course_frames > scale_ticks(0xB4)) {
            if (rilgar_off_course_reset_started == 0) {
                rilgar_off_course_reset_started = 1;
                rilgar_off_course_reset_ready = 0;
                rilgar_off_course_reset_rate = frame_scale_r * 0.08f;
            }
            ((Hero *)(hero_globals_base + 0xE1D))->off_course_reset_pending = 1;
            if (rilgar_off_course_reset_ready == 1) {
                ((Hero *)(hero_globals_base + 0xE1D))->off_course_reset_pending = 0;
                compute_hero_hoverboard_respawn_transform_r(v0, v1);
                ((Hero *)(hero_globals_base + 0xE1D))->off_course_frames = 0;
                ((Hero *)(hero_globals_base + 0xE1D))->board_air_yaw = v1[2];
                Hero_ResetToPosition_r(v0, v1, 0x6B, 1);
                if (((Path_254DE8 *)((Hero *)(hero_globals_base + 0xE1D))->race_path) != 0) {
                    int r = select_point_index_by_distance_r(((Hero *)(hero_globals_base + 0xE1D))->motion.pos.f, (Path_254DE8 *)((Hero *)(hero_globals_base + 0xE1D))->race_path, 0.0f);

                    if (r < ((Hero *)(hero_globals_base + 0xE1D))->race_path_node) {
                        ((Hero *)(hero_globals_base + 0xE1D))->race_path_node = r;
                    }
                }
            }
        }
    } else if (((Hero *)(hero_globals_base + 0xE1D))->off_course_reset_pending == 0) {
        ((Hero *)(hero_globals_base + 0xE1D))->off_course_frames = 0;
    }
    for (j = 0; j < d->f40; j++) {
        int i;
        int n1;
        int next;

        i = ((Hero *)(hero_globals_base + 0xE1D))->racer_path_point[j];
        n1 = path->n + 1;
        next = (i + n1) % path->n;
        ang = fast_atan2_r2(path->p[next].x - path->p[i].x, path->p[next].y - path->p[i].y);
        qcopy(v0, moby_pool_4f60_r2[d->f2C[j]].pos);
        if (FastDiffRots_r(ang, fast_atan2_r2(v0[0] - path->p[i].x, v0[1] - path->p[i].y)) < 1.5707964f) {
            ((Hero *)(hero_globals_base + 0xE1D))->racer_path_point[j] = next;
            if (next == 0) {
                ((Hero *)(hero_globals_base + 0xE1D))->racer_laps[j]++;
            }
        }
    }
    ((Hero *)(hero_globals_base + 0xE1D))->race_position = 1;
    for (j = 0; j < d->f40; j++) {
        if (((Hero *)(hero_globals_base + 0xE1D))->race_lap < ((Hero *)(hero_globals_base + 0xE1D))->racer_laps[j] || (((Hero *)(hero_globals_base + 0xE1D))->racer_laps[j] == ((Hero *)(hero_globals_base + 0xE1D))->race_lap && ((Hero *)(hero_globals_base + 0xE1D))->racer_path_point[j] >= ((Hero *)(hero_globals_base + 0xE1D))->race_path_node)) {
            ((Hero *)(hero_globals_base + 0xE1D))->race_position++;
        }
    }
    for (k = 0; k < d->f40; k++) {
        ((Hero *)(hero_globals_base + 0xE1D))->race_rank[k] = 1;
        if (((Hero *)(hero_globals_base + 0xE1D))->race_lap > ((Hero *)(hero_globals_base + 0xE1D))->racer_laps[k] || (((Hero *)(hero_globals_base + 0xE1D))->racer_laps[k] == ((Hero *)(hero_globals_base + 0xE1D))->race_lap && ((Hero *)(hero_globals_base + 0xE1D))->race_path_node > ((Hero *)(hero_globals_base + 0xE1D))->racer_path_point[k])) {
            ((Hero *)(hero_globals_base + 0xE1D))->race_rank[k] = 2;
        }
        for (m = 0; m < d->f40; m++) {
            if (((Hero *)(hero_globals_base + 0xE1D))->racer_laps[k] < ((Hero *)(hero_globals_base + 0xE1D))->racer_laps[m] || (((Hero *)(hero_globals_base + 0xE1D))->racer_laps[k] == ((Hero *)(hero_globals_base + 0xE1D))->racer_laps[m] && ((Hero *)(hero_globals_base + 0xE1D))->racer_path_point[k] < ((Hero *)(hero_globals_base + 0xE1D))->racer_path_point[m])) {
                ((Hero *)(hero_globals_base + 0xE1D))->race_rank[k]++;
            }
        }
    }
    if (((Hero *)(hero_globals_base + 0xE1D))->race_lap < 3) {
        return;
    }
    if (*(special_items_base_r + 0x13B) != 0) {
        hoverboard_race_finished = 1;
    }
    if (g_current_level_index_r == 5) {
        rilgar_hoverboard_races_finished_r++;
    } else if (g_current_level_index_r == 0x10) {
        kalebo3_hoverboard_races_finished_r++;
    }
    if (g_current_level_index_r == 5) {
        int t;

        if (pal_mode_r != 0) {
            t = scale_ticks(0x1680);
        } else {
            t = scale_ticks(0x1644);
        }
        if (t >= ((Hero *)(hero_globals_base + 0xE1D))->race_time_frames) {
            if (((Flags_254DE8 *)(menu_item_enabled_115_r + 1))->f9 == 0) {
                ((Flags_254DE8 *)(menu_item_enabled_115_r + 1))->f9 = 1;
                PlayBankSound_r(1, 0, 0);
                ShowBanner_r(0x542D, -1);
            }
        }
        if (d->f28 == -1) {
            return;
        }
        if (*(special_items_base_r + 0x13B) == 0 && ((Hero *)(hero_globals_base + 0xE1D))->race_position == 1) {
            moby_pool_4f60_r2[d->f28].fBC = 1;
        } else {
            ((Hero *)(hero_globals_base + 0xE1D))->unk890 = (u8 *)(&moby_pool_4f60_r2[d->f28]);
            mode_freeze_init_r(0, 0, 0);
        }
        if (race_best_times_r[0] == 0) {
            race_best_times_r[0] = ((Hero *)(hero_globals_base + 0xE1D))->race_time_frames;
            gBolts_r += 0x3E8;
        } else if (((Hero *)(hero_globals_base + 0xE1D))->race_time_frames < race_best_times_r[0]) {
            race_best_times_r[0] = ((Hero *)(hero_globals_base + 0xE1D))->race_time_frames;
            gBolts_r += 0x3E8;
        }
        if (race_best_records_r[0] < ((Hero *)(hero_globals_base + 0xE1D))->race_score) {
            race_best_records_r[0] = ((Hero *)(hero_globals_base + 0xE1D))->race_score;
            gBolts_r += 0x3E8;
        }
        moby_pool_4f60_r2[d->f28].f30 = 0xFF;
        if (((Hero *)(hero_globals_base + 0xE1D))->trick_hud_handle != -1) {
            Hud_SetBarValueById_r(((Hero *)(hero_globals_base + 0xE1D))->trick_hud_handle, 0);
            ((Hero *)(hero_globals_base + 0xE1D))->trick_hud_handle = -1;
        }
        update_resource_counter_r();
    } else if (g_current_level_index_r == 0x10) {
        if (d->f28 == -1) {
            return;
        }
        if (gHaveHologuise_r == 0 && ((Hero *)(hero_globals_base + 0xE1D))->race_position == 1) {
            moby_pool_4f60_r2[d->f28].fBC = 1;
        } else {
            ((Hero *)(hero_globals_base + 0xE1D))->unk890 = (u8 *)(&moby_pool_4f60_r2[d->f28]);
            mode_freeze_init_r(0, 0, 0);
        }
        if (race_best_times_r[1] == 0) {
            race_best_times_r[1] = ((Hero *)(hero_globals_base + 0xE1D))->race_time_frames;
            gBolts_r += 0x3E8;
        } else if (((Hero *)(hero_globals_base + 0xE1D))->race_time_frames < race_best_times_r[1]) {
            race_best_times_r[1] = ((Hero *)(hero_globals_base + 0xE1D))->race_time_frames;
            gBolts_r += 0x3E8;
        }
        if (race_best_records_r[1] < ((Hero *)(hero_globals_base + 0xE1D))->race_score) {
            race_best_records_r[1] = ((Hero *)(hero_globals_base + 0xE1D))->race_score;
            gBolts_r += 0x3E8;
        }
        moby_pool_4f60_r2[d->f28].f30 = 0xFF;
        if (((Hero *)(hero_globals_base + 0xE1D))->trick_hud_handle != -1) {
            Hud_SetBarValueById_r(((Hero *)(hero_globals_base + 0xE1D))->trick_hud_handle, 0);
            ((Hero *)(hero_globals_base + 0xE1D))->trick_hud_handle = -1;
        }
        update_resource_counter_r();
    }
}

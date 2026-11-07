void update_hero_hoverboard_race_progress(void) {
    float v0[4] __attribute__((aligned(16)));
    float v1[4] __attribute__((aligned(16)));
    Data_254DE8 *d;
    Path *path;
    int near;
    int j;
    int k;
    int m;
    float ang;
    float dir;

    g_hero.race_time_frames++;
    if (((Moby *)g_hero.hoverboard_moby) == 0) {
        return;
    }
    d = (Data_254DE8 *)((Moby *)g_hero.hoverboard_moby)->pvars;
    if (d->f20 == -1) {
        return;
    }
    path = rilgar_paths_r2[d->f20];
    g_hero.race_path = (void *)path;
    if (g_current_level_index_r == LEVEL_KALEBO3) {
        *(u128_254DE8 *)v0 = 0;
        v0[0] = 163.0f;
        v0[1] = 119.0f;
        v0[2] = 80.0f;
        if (fast_vec_dist(g_hero.motion.pos.f, v0) < 11.0f) {
            if (g_hero.race_path_node < 3 || g_hero.race_path_node > 0x46) {
                if (g_hero.race_path_node >= 3) {
                    g_hero.race_lap++;
                }
                g_hero.race_path_node = 0xD;
            }
        }
        *(u128_254DE8 *)v1 = 0;
        v1[0] = 223.0f;
        v1[1] = 281.0f;
        v1[2] = 93.0f;
        if (fast_vec_dist(g_hero.motion.pos.f, v1) < 7.0f) {
            if (g_hero.race_path_node < 0x3B) {
                g_hero.race_path_node = 0x3B;
            }
        }
    }
    {
        int i;
        int n1;
        int next;

        i = g_hero.race_path_node;
        n1 = path->n + 1;
        next = (i + n1) % path->n;
        ang = fast_atan2(path->p[next].x - path->p[i].x, path->p[next].y - path->p[i].y);
        if (FastDiffRots(ang, fast_atan2(g_hero.motion.pos.f[0] - path->p[i].x,
                                         g_hero.motion.pos.f[1] - path->p[i].y)) < 1.5707964f) {
            g_hero.race_path_node = next;
            if (next == 0) {
                g_hero.race_lap++;
            }
        }
    }
    near = 0;
    if (g_current_level_index_r == LEVEL_KALEBO3) {
        if (g_hero.motion.pos.f[2] < path->p[g_hero.race_path_node].z - 2.0f) {
            near = 1;
            if (g_hero.off_course_frames < scale_ticks(0x46)) {
                g_hero.off_course_frames = scale_ticks(0x46);
            }
        }
    }
    {
        int i;
        int n1;
        int next;

        i = g_hero.wrong_way_path_point;
        n1 = path->n + 1;
        next = (i + n1) % path->n;
        dir = fast_atan2(path->p[next].x - path->p[i].x, path->p[next].y - path->p[i].y);
    }
    if (near != 0 || (FastDiffRots(g_hero.motion.rot.f[2], dir) > 1.5707964f && g_hero.trick_input == 0)) {
        g_hero.off_course_frames++;
        if (g_hero.off_course_frames > scale_ticks(0xB4)) {
            if (race_respawn_request_active == 0) {
                race_respawn_request_active = 1;
                race_respawn_gate_state = 0;
                rilgar_off_course_reset_rate = frame_scale_r * 0.08f;
            }
            g_hero.off_course_reset_pending = 1;
            if (race_respawn_gate_state == 1) {
                g_hero.off_course_reset_pending = 0;
                compute_hero_hoverboard_respawn_transform_r(v0, v1);
                g_hero.off_course_frames = 0;
                g_hero.board_air_yaw = v1[2];
                set_race_camera(v0, v1, 0x6B, 1);
                if (((Path *)g_hero.race_path) != 0) {
                    int r = select_point_index_by_distance(g_hero.motion.pos.f, (Path *)g_hero.race_path, 0.0f);

                    if (r < g_hero.race_path_node) {
                        g_hero.race_path_node = r;
                    }
                }
            }
        }
    } else if (g_hero.off_course_reset_pending == 0) {
        g_hero.off_course_frames = 0;
    }
    for (j = 0; j < d->f40; j++) {
        int i;
        int n1;
        int next;

        i = g_hero.racer_path_point[j];
        n1 = path->n + 1;
        next = (i + n1) % path->n;
        ang = fast_atan2(path->p[next].x - path->p[i].x, path->p[next].y - path->p[i].y);
        qcopy(v0, moby_pool_4f60_r2[d->f2C[j]].pos);
        if (FastDiffRots(ang, fast_atan2(v0[0] - path->p[i].x, v0[1] - path->p[i].y)) < 1.5707964f) {
            g_hero.racer_path_point[j] = next;
            if (next == 0) {
                g_hero.racer_laps[j]++;
            }
        }
    }
    g_hero.race_position = 1;
    for (j = 0; j < d->f40; j++) {
        if (g_hero.race_lap < g_hero.racer_laps[j] ||
            (g_hero.racer_laps[j] == g_hero.race_lap && g_hero.racer_path_point[j] >= g_hero.race_path_node)) {
            g_hero.race_position++;
        }
    }
    for (k = 0; k < d->f40; k++) {
        g_hero.race_rank[k] = 1;
        if (g_hero.race_lap > g_hero.racer_laps[k] ||
            (g_hero.racer_laps[k] == g_hero.race_lap && g_hero.race_path_node > g_hero.racer_path_point[k])) {
            g_hero.race_rank[k] = 2;
        }
        for (m = 0; m < d->f40; m++) {
            if (g_hero.racer_laps[k] < g_hero.racer_laps[m] ||
                (g_hero.racer_laps[k] == g_hero.racer_laps[m] &&
                 g_hero.racer_path_point[k] < g_hero.racer_path_point[m])) {
                g_hero.race_rank[k]++;
            }
        }
    }
    if (g_hero.race_lap < 3) {
        return;
    }
    if (*(special_items_base_r + 0x13B) != 0) {
        hoverboard_race_finished = 1;
    }
    if (g_current_level_index_r == LEVEL_RILGAR) {
        rilgar_hoverboard_races_finished_r++;
    } else if (g_current_level_index_r == LEVEL_KALEBO3) {
        kalebo3_hoverboard_races_finished_r++;
    }
    if (g_current_level_index_r == LEVEL_RILGAR) {
        int t;

        if (pal_mode_r != 0) {
            t = scale_ticks(0x1680);
        } else {
            t = scale_ticks(0x1644);
        }
        if (t >= g_hero.race_time_frames) {
            if (((Flags *)(menu_item_enabled_115_r + 1))->f9 == 0) {
                ((Flags *)(menu_item_enabled_115_r + 1))->f9 = 1;
                allocate_voice_for_bank_entry(1, 0, 0);
                queue_dialogue_message(0x53D6, -1);
            }
        }
        if (d->f28 == -1) {
            return;
        }
        if (*(special_items_base_r + 0x13B) == 0 && g_hero.race_position == 1) {
            moby_pool_4f60_r2[d->f28].fBC = 1;
        } else {
            g_hero.unk890 = (u8 *)(&moby_pool_4f60_r2[d->f28]);
            ((void (*)(int, int))mode_freeze_init)(0, 0);
        }
        if (race_best_times_r[0] == 0) {
            race_best_times_r[0] = g_hero.race_time_frames;
            gBolts_r += 0x3E8;
        } else if (g_hero.race_time_frames < race_best_times_r[0]) {
            race_best_times_r[0] = g_hero.race_time_frames;
            gBolts_r += 0x3E8;
        }
        if (race_best_records_r[0] < g_hero.race_score) {
            race_best_records_r[0] = g_hero.race_score;
            gBolts_r += 0x3E8;
        }
        moby_pool_4f60_r2[d->f28].f30 = 0xFF;
        if (g_hero.trick_hud_handle != -1) {
            set_animation_parameter(g_hero.trick_hud_handle, 0);
            g_hero.trick_hud_handle = -1;
        }
        decrement_resource_counter();
    } else if (g_current_level_index_r == LEVEL_KALEBO3) {
        if (d->f28 == -1) {
            return;
        }
        if (gHaveHologuise_r == 0 && g_hero.race_position == 1) {
            moby_pool_4f60_r2[d->f28].fBC = 1;
        } else {
            g_hero.unk890 = (u8 *)(&moby_pool_4f60_r2[d->f28]);
            ((void (*)(int, int))mode_freeze_init)(0, 0);
        }
        if (race_best_times_r[1] == 0) {
            race_best_times_r[1] = g_hero.race_time_frames;
            gBolts_r += 0x3E8;
        } else if (g_hero.race_time_frames < race_best_times_r[1]) {
            race_best_times_r[1] = g_hero.race_time_frames;
            gBolts_r += 0x3E8;
        }
        if (race_best_records_r[1] < g_hero.race_score) {
            race_best_records_r[1] = g_hero.race_score;
            gBolts_r += 0x3E8;
        }
        moby_pool_4f60_r2[d->f28].f30 = 0xFF;
        if (g_hero.trick_hud_handle != -1) {
            set_animation_parameter(g_hero.trick_hud_handle, 0);
            g_hero.trick_hud_handle = -1;
        }
        decrement_resource_counter();
    }
}

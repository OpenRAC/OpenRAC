extern short D_E1_L02_00161FAC;
void spawn_background_ship_2_level_10_joint_sparks(char *moby, void *arg) {
    float v0[4];
    float v1[4];
    float mat[4];
    float v2[4];
    float m[4];
    int i;
    FastVecSub(mat, moby + 0x10, arg);
    for (i = 0; i < 2; i++) {
        int a, b, c, d, e;
        char *p;
        Moby_GetJointWorldPos(moby, i, m);
        FastVecScale(v0, mat, *(float *)&background_ship_2_orxon_spark_start_vel_scale);
        FastVecScale(v1, mat, *(float *)&background_ship_2_orxon_spark_end_vel_scale);
        rand_vec(v2, 0.0f, *(float *)&background_ship_2_orxon_spark_jitter * g_frame_time);
        FastVecAdd(v1, v1, v2);
        v0[3] = random_float_between(*(float *)&background_ship_2_spark_start_size_min, *(float *)&background_ship_2_spark_start_size_max) / 1.5f;
        v1[3] = random_float_between(*(float *)&background_ship_2_spark_end_size_min, *(float *)&background_ship_2_spark_end_size_max) / 1.5f;
        a = FastTweenColor(*(int *)&background_ship_2_orxon_spark_start_color_a, *(int *)&background_ship_2_orxon_spark_start_color_b, random_float_between(0.0f, 1.0f));
        b = FastTweenColor(*(int *)&background_ship_2_orxon_spark_end_color_a, *(int *)&D_E1_L02_00161FAC, random_float_between(0.0f, 1.0f));
        c = truncate_float_to_s32_r(multiply_global_factor((float)*(int *)&background_ship_2_spark_lifetime * random_float_between(0.0f, 1.0f) + 1.0f));
        d = truncate_float_to_s32_r(multiply_global_factor((float)*(int *)&background_ship_2_spark_time2 * (random_float_between(-*(float *)&background_ship_2_spark_time_jitter, *(float *)&background_ship_2_spark_time_jitter) + 1.0f)));
        e = truncate_float_to_s32_r(multiply_global_factor((float)*(int *)&background_ship_2_spark_time3 * (random_float_between(-*(float *)&background_ship_2_spark_time_jitter, *(float *)&background_ship_2_spark_time_jitter) + 1.0f)));
        p = PartType02Spawn(m, v0, v1, a, b, c, d, e, -1);
        if (p != 0) {
            p[9] = truncate_float_to_s32_r(8.0f) + 0x60;
        }
    }
}

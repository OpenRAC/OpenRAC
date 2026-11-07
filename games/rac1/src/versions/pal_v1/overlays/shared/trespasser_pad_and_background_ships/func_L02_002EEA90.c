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
        moby_point_position(moby, i, m);
        FastVecScale(v0, mat, background_ship_2_level_10_first_spark_position_delta_scale_gp);
        FastVecScale(v1, mat, background_ship_2_level_10_second_spark_position_delta_scale_gp);
        random_vector(v2, 0.0f, background_ship_2_level_10_spark_random_offset_radius_scale_gp * g_frame_time);
        FastVecAdd(v1, v1, v2);
        v0[3] = random_float_between(background_ship_2_first_spark_fourth_component_range_start_gp,
                                     background_ship_2_first_spark_fourth_component_range_end_gp) /
                1.5f;
        v1[3] = random_float_between(background_ship_2_second_spark_fourth_component_range_start_gp,
                                     background_ship_2_second_spark_fourth_component_range_end_gp) /
                1.5f;
        a = FastTweenColor(background_ship_2_level_10_first_spark_color_tween_start_gp,
                           background_ship_2_level_10_first_spark_color_tween_end_gp, random_float_between(0.0f, 1.0f));
        b = FastTweenColor(background_ship_2_level_10_second_spark_color_tween_start_gp, *(int *)&D_E1_L02_00161FAC,
                           random_float_between(0.0f, 1.0f));
        c = truncate_float_to_s32_r(multiply_global_factor(
            (float)background_ship_2_spark_frame_count_random_span_gp * random_float_between(0.0f, 1.0f) + 1.0f));
        d = truncate_float_to_s32_r(
            multiply_global_factor((float)background_ship_2_spark_second_frame_count_base_gp *
                                   (random_float_between(-background_ship_2_spark_frame_count_variation_fraction_gp,
                                                         background_ship_2_spark_frame_count_variation_fraction_gp) +
                                    1.0f)));
        e = truncate_float_to_s32_r(
            multiply_global_factor((float)background_ship_2_spark_third_frame_count_base_gp *
                                   (random_float_between(-background_ship_2_spark_frame_count_variation_fraction_gp,
                                                         background_ship_2_spark_frame_count_variation_fraction_gp) +
                                    1.0f)));
        p = PartType02Spawn(m, v0, v1, a, b, c, d, e, -1);
        if (p != 0) {
            p[9] = truncate_float_to_s32_r(8.0f) + 0x60;
        }
    }
}

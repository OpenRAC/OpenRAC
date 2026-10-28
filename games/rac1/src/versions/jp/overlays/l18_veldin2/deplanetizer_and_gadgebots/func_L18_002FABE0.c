extern float D_J_L18_001625DC MACRO_ADDR;
void draw_three_scaled_camera_facing_quads(void) {
    extern ML_9780 veldin2_cutscene_state;
    extern int scale_ticks(int);
    extern unsigned long GetEffectTex(int);
    extern void VU1_addGSregister(int, long);
    extern void matrix_mul_vec3(void *, void *, void *);
    extern void FastVecAdd(void *, void *, void *);
    extern void FastVecSub(void *, void *, void *);
    extern void vec_scale_to_length(void *, void *, float);
    extern void FastVecCross(void *, void *, void *);
    extern void FastVecScale(void *, void *, float);
    extern void transform_vector(void *, void *, void *);
    extern void draw_world_quad(void *, void *, int);
    extern short deplanetizer_quads_start_tick;
    extern char veldin2_camera_matrix[];
    extern short deplanetizer_flare_outer_color;
    extern short deplanetizer_flare_inner_color;
    extern float veldin2_deplanetizer_quad_corners[4][4];
    Pk_9780 pk;
    float mat[4][4];
    float up[4];
    int i;
    int color;
    float tilt;
    char *target;

    if (veldin2_cutscene_state.n == scale_ticks((*(int *)&deplanetizer_quads_start_tick) + 1)) {
        deplanetizer_camera_quad_local_offset_x = 1.0f;
        deplanetizer_flare_cam_offset_y = 0.0f;
        if (veldin2_cutscene_state.state == 7) {
            tilt = -0.15f;
        } else {
            tilt = 0.15f;
        }
        deplanetizer_flare_cam_offset_z = tilt;
        D_J_L18_001625DC = 1.0f;
    }
    pk.b = GetEffectTex(0xB);
    pk.c = 0x0000FF9000000260ULL;
    pk.d = 0x8000000048ULL;
    pk.a = 0;
    VU1_addGSregister(0x4A, 0);
    VU1_addGSregister(0x47, 0x51001);
    matrix_mul_vec3(mat[3], &deplanetizer_camera_quad_local_offset_x, veldin2_camera_matrix);
    target = veldin2_camera_matrix;
    target -= 0x210;
    FastVecAdd(mat[3], mat[3], target);
    FastVecSub(mat[0], mat[3], target);
    vec_scale_to_length(mat[0], mat[0], 1.0f);
    *(u128_9780 *)up = 0;
    up[2] = 1.0f;
    up[3] = 1.0f;
    FastVecCross(mat[1], mat[0], up);
    vec_scale_to_length(mat[1], mat[1], 1.0f);
    FastVecCross(mat[2], mat[1], mat[0]);
    pk.uv[0] = 1.0f;
    pk.uv[1] = 1.0f;
    pk.uv[2] = 0.0f;
    pk.uv[3] = 1.0f;
    pk.uv[4] = 1.0f;
    pk.uv[5] = 0.0f;
    pk.uv[6] = 0.0f;
    pk.uv[7] = 0.0f;
    color = (*(int *)&deplanetizer_flare_outer_color);
    pk.col[3] = color;
    pk.col[2] = color;
    pk.col[1] = color;
    pk.col[0] = color;
    for (i = 0; i < 4; i++) {
        FastVecScale(pk.m[i], veldin2_deplanetizer_quad_corners[i], deplanetizer_camera_quad_base_scale);
        transform_vector(pk.m[i], pk.m[i], mat);
    }
    draw_world_quad(&pk, 0, 0);
    color = (*(int *)&deplanetizer_flare_inner_color);
    pk.col[3] = color;
    pk.col[2] = color;
    pk.col[1] = color;
    pk.col[0] = color;
    for (i = 0; i < 4; i++) {
        FastVecScale(pk.m[i], veldin2_deplanetizer_quad_corners[i], deplanetizer_camera_quad_base_scale * 0.75f);
        transform_vector(pk.m[i], pk.m[i], mat);
    }
    draw_world_quad(&pk, 0, 0);
    for (i = 0; i < 4; i++) {
        FastVecScale(pk.m[i], veldin2_deplanetizer_quad_corners[i], deplanetizer_camera_quad_base_scale * 0.5f);
        transform_vector(pk.m[i], pk.m[i], mat);
    }
    draw_world_quad(&pk, 0, 0);
}

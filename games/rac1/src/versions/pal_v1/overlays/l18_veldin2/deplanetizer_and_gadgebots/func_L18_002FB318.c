extern float D_E1_L18_001DFAD0[4][4];
void draw_moby_three_scaled_two_tint_quads(struct Moby *m) {
    Pk_9c20_FB318 pk;
    float mat[4][4];
    float up[4];
    Vec4 vertices[4];
    int i = 0;
    int color;

    pk.b = GetEffectTex(0xB);
    pk.c = 0x0000FF9000000260ULL;
    pk.d = 0x8000000048ULL;
    pk.a = 0;
    VU1_addGSregister(0x4A, 0);
    VU1_addGSregister(0x47, 0x51001);
    matrix_mul_vec3(mat[3], &deplanetizer_glow_local_offset_x, &m->rot_row0);
    FastVecAdd(mat[3], mat[3], &m->pos);
    *(u128_9c20_FB318 *)up = 0;
    up[2] = -1.0f;
    up[3] = 1.0f;
    matrix_mul_vec3(mat[0], up, &m->rot_row0);
    FastVecSub(mat[1], mat[3], veldin2_camera_pos);
    FastVecCross(mat[2], mat[1], mat[0]);
    vec_scale_to_length(mat[2], mat[2], 1.0f);
    FastVecCross(mat[1], mat[0], mat[2]);

    color = (*(int *)&deplanetizer_glow_tint_a) | 0xFF000000;
    pk.uv[0] = 1.0f;
    pk.uv[1] = 1.0f;
    pk.uv[2] = 0.0f;
    pk.uv[3] = 1.0f;
    pk.uv[4] = 1.0f;
    pk.uv[5] = 0.0f;
    pk.uv[6] = 0.0f;
    pk.uv[7] = 0.0f;

    pk.col[3] = color;
    pk.col[2] = color;
    pk.col[1] = color;
    pk.col[0] = color;
    i = 0;
    do {
        qcopy_nc(&vertices[i], D_E1_L18_001DFAD0[i]);
        {
            vertices[i].f[2] *= (*(float *)&deplanetizer_streak1_z_scale);
            vertices[i].f[0] *= deplanetizer_three_quad_local_x_scale;
        }
        i++;
    } while (i < 4);
    for (i = 0; i < 4; i++) {
        transform_vector(pk.m[i], &vertices[i], mat);
    }
    draw_world_quad(&pk, 0, 0);

    pk.col[3] = (*(int *)&deplanetizer_glow_tint_b) | 0x7F000000;
    pk.col[2] = pk.col[3];
    pk.col[1] = pk.col[3];
    pk.col[0] = pk.col[3];
    i = 0;
    do {
        qcopy_nc(&vertices[i], D_E1_L18_001DFAD0[i]);
        {
            vertices[i].f[2] *= (*(float *)&deplanetizer_streak2_z_scale);
            vertices[i].f[0] *= deplanetizer_three_quad_local_x_scale;
        }
        i++;
    } while (i < 4);
    for (i = 0; i < 4; i++) {
        transform_vector(pk.m[i], &vertices[i], mat);
    }
    draw_world_quad(&pk, 0, 0);

    pk.col[3] = (*(int *)&deplanetizer_glow_tint_b) | 0x7F000000;
    pk.col[2] = pk.col[3];
    pk.col[1] = pk.col[3];
    pk.col[0] = pk.col[3];
    i = 0;
    do {
        qcopy_nc(&vertices[i], D_E1_L18_001DFAD0[i]);
        {
            vertices[i].f[2] *= (*(float *)&deplanetizer_streak3_z_scale);
            vertices[i].f[0] *= deplanetizer_three_quad_local_x_scale;
        }
        i++;
    } while (i < 4);
    for (i = 0; i < 4; i++) {
        transform_vector(pk.m[i], &vertices[i], mat);
    }
    draw_world_quad(&pk, 0, 0);
}

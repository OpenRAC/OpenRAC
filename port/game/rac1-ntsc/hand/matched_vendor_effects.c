/* SPDX-License-Identifier: GPL-3.0-or-later */
/* FUN_L00_002a5040 (draw_vendor_effects), the Gadgetron vendor's glow and sparkles. As the port
 * runs it: the NTSC decomp's matching C (src/overlays/shared/vendor/vendor.c) with the second
 * euler_rotation_basis call given effect_camera_rotation_angles itself. The matching C hands it
 * that address through an asm statement (`addiu %0,$sp,0xD0` into the register variable d0_arg,
 * frame slot 0xD0 being the array's), a fakematch for retail's instruction order the host cannot
 * run: d0_arg stayed 0 and the sparkles' basis came from guest address 0. Otherwise the matching C, line for line; it goes at the end of
 * vendor.c, whose declarations give the name its address. Not a PS2 match; the decomp's C stays as
 * it is (hostgen.json "overrides": this C replaces it in the port only). */
void draw_vendor_effects(char *vendor_effect_moby) {
    char    *vendor_effect_vars = *(char **)(vendor_effect_moby + 0x78);
    Quad     qd;
    long     pk[4];
    float    q[4][4];
    float    effect_camera_rotation_angles[4];
    float    mtx[3][4];
    V_2A62D0 M[4];
    V_2A62D0 T[4];
    float    zero;
    char    *g, *effect_camera_state;
    float(*uv_v)[2];
    float(*uvs)[2];
    int   izero;
    float uv_v_scroll_or_camera_z_angle;
    /* Fakematch: the sparkle radius 1.1 lives in $f22, copied from the glow block's 1.1. */
    register float sparkle_radius __asm__("$f22");
    int             k, i, i0, i1, i2, i3, col;

    effect_camera_state = (char *)&l00_camera_lui;
    effect_camera_rotation_angles[2] = *(float *)(effect_camera_state + 0x158);
    effect_camera_rotation_angles[0] = 0.0f;
    effect_camera_rotation_angles[1] = 0.0f;
    euler_rotation_basis(mtx, effect_camera_rotation_angles);

    vendor_sparkle_uv_scroll = vendor_sparkle_uv_scroll + 0.01f;
    if (1.0f < vendor_sparkle_uv_scroll) {
        vendor_sparkle_uv_scroll = vendor_sparkle_uv_scroll - 1.0f;
    }

    pk[2] = 1;
    pk[3] = 0x4000000044L;
    pk[0] = 0;
    pk[1] = get_effect_texture(0x18);

    for (k = 0; k < 4; k++) {
        transform_vector_by_basis(
            q[0], l00_vendor_sparkle_local_vertices[vendor_sparkle_quad_corners[k][0]], mtx);
        transform_vector_by_basis(
            q[1], l00_vendor_sparkle_local_vertices[vendor_sparkle_quad_corners[k][1]], mtx);
        transform_vector_by_basis(
            q[2], l00_vendor_sparkle_local_vertices[vendor_sparkle_quad_corners[k][2]], mtx);
        transform_vector_by_basis(
            q[3], l00_vendor_sparkle_local_vertices[vendor_sparkle_quad_corners[k][3]], mtx);

        qd.v[0][0] =
            *(float *)(vendor_effect_moby + 0x10) + q[0][0] * *(float *)(vendor_effect_vars + 0x90);
        qd.v[0][1] =
            *(float *)(vendor_effect_moby + 0x14) + q[0][1] * *(float *)(vendor_effect_vars + 0x90);
        qd.v[0][2] = *(float *)(vendor_effect_moby + 0x18) + q[0][2];
        qd.v[1][0] =
            *(float *)(vendor_effect_moby + 0x10) + q[1][0] * *(float *)(vendor_effect_vars + 0x90);
        qd.v[1][1] =
            *(float *)(vendor_effect_moby + 0x14) + q[1][1] * *(float *)(vendor_effect_vars + 0x90);
        qd.v[1][2] = *(float *)(vendor_effect_moby + 0x18) + q[1][2];
        qd.v[2][0] =
            *(float *)(vendor_effect_moby + 0x10) + q[2][0] * *(float *)(vendor_effect_vars + 0x90);
        qd.v[2][1] =
            *(float *)(vendor_effect_moby + 0x14) + q[2][1] * *(float *)(vendor_effect_vars + 0x90);
        qd.v[2][2] = *(float *)(vendor_effect_moby + 0x18) + q[2][2];
        qd.v[3][0] =
            *(float *)(vendor_effect_moby + 0x10) + q[3][0] * *(float *)(vendor_effect_vars + 0x90);
        qd.v[3][1] =
            *(float *)(vendor_effect_moby + 0x14) + q[3][1] * *(float *)(vendor_effect_vars + 0x90);
        qd.v[3][2] = *(float *)(vendor_effect_moby + 0x18) + q[3][2];

        i0 = vendor_sparkle_quad_corners[k][0];
        i1 = vendor_sparkle_quad_corners[k][1];
        i2 = vendor_sparkle_quad_corners[k][2];
        i3 = vendor_sparkle_quad_corners[k][3];
        uv_v_scroll_or_camera_z_angle = vendor_sparkle_uv_scroll;
        qd.col[0] = vendor_sparkle_colors[i0];
        qd.col[1] = vendor_sparkle_colors[i1];
        qd.col[2] = vendor_sparkle_colors[i2];
        qd.col[3] = vendor_sparkle_colors[i3];
        uvs = l00_vendor_sparkle_base_uvs;
        uv_v = (float(*)[2]) & uvs[0][1];
        qd.uv[0][0] = uvs[i0][0];
        qd.uv[0][1] = uv_v[i0][0] + uv_v_scroll_or_camera_z_angle;
        qd.uv[1][0] = uvs[i1][0];
        qd.uv[1][1] = uv_v[i1][0] + uv_v_scroll_or_camera_z_angle;
        qd.uv[2][0] = uvs[i2][0];
        qd.uv[2][1] = uv_v[i2][0] + uv_v_scroll_or_camera_z_angle;
        qd.uv[3][0] = uvs[i3][0];
        qd.uv[3][1] = uv_v[i3][0] + uv_v_scroll_or_camera_z_angle;
        draw_geometry_quad(&qd, 0, 1);
    }

    pk[1] = get_effect_texture(0x1B);

    vendor_glow_phase = vendor_glow_phase + 0.01f;
    if (1.0f < vendor_glow_phase) {
        vendor_glow_phase = 0.0f;
    }

    {
        float g = vendor_glow_phase;
        float glow_negative_extent = -g * 1.2f;
        float pp = g * 1.2f;
        float z = g * 1.4f + 1.1f;
        float zz = z - 0.2f;
        sparkle_radius = 1.1f;

        T[0].q = 0;
        T[0].f[0] = glow_negative_extent * *(float *)(vendor_effect_vars + 0x90);
        T[0].f[1] = glow_negative_extent * *(float *)(vendor_effect_vars + 0x90);
        T[0].f[2] = z;
        T[1].q = 0;
        T[1].f[0] = pp * *(float *)(vendor_effect_vars + 0x90);
        T[1].f[1] = glow_negative_extent * *(float *)(vendor_effect_vars + 0x90);
        T[1].f[2] = zz;
        T[2].q = 0;
        T[2].f[0] = glow_negative_extent * *(float *)(vendor_effect_vars + 0x90);
        T[2].f[1] = pp * *(float *)(vendor_effect_vars + 0x90);
        T[2].f[2] = z;
        T[3].q = 0;
        T[3].f[0] = pp * *(float *)(vendor_effect_vars + 0x90);
        T[3].f[1] = pp * *(float *)(vendor_effect_vars + 0x90);
        T[3].f[2] = zz;
        M[0].q = T[0].q;
        T[0].q = M[0].q;
        M[1].q = T[1].q;
        T[1].q = M[1].q;
        M[2].q = T[2].q;
        M[3].q = T[3].q;
    }

    g = (char *)&l00_camera_lui;
    uv_v_scroll_or_camera_z_angle = *(float *)(g + 0x158);
    effect_camera_rotation_angles[0] = 0.0f;
    effect_camera_rotation_angles[2] = uv_v_scroll_or_camera_z_angle;
    effect_camera_rotation_angles[1] = 0.0f;
    euler_rotation_basis(mtx, effect_camera_rotation_angles);
    transform_vector_by_basis(q[0], M[0].f, mtx);
    transform_vector_by_basis(q[1], M[1].f, mtx);
    transform_vector_by_basis(q[2], M[2].f, mtx);
    transform_vector_by_basis(q[3], M[3].f, mtx);

    col = ((int)((1.0f - vendor_glow_phase) * 128.0f) << 24) | 0x808080;
    qd.v[0][0] = *(float *)(vendor_effect_moby + 0x10) + q[0][0];
    qd.v[0][1] = *(float *)(vendor_effect_moby + 0x14) + q[0][1];
    qd.v[0][2] = *(float *)(vendor_effect_moby + 0x18) + q[0][2];
    qd.v[1][0] = *(float *)(vendor_effect_moby + 0x10) + q[1][0];
    qd.v[1][1] = *(float *)(vendor_effect_moby + 0x14) + q[1][1];
    qd.v[1][2] = *(float *)(vendor_effect_moby + 0x18) + q[1][2];
    qd.v[2][0] = *(float *)(vendor_effect_moby + 0x10) + q[2][0];
    qd.v[2][1] = *(float *)(vendor_effect_moby + 0x14) + q[2][1];
    qd.v[2][2] = *(float *)(vendor_effect_moby + 0x18) + q[2][2];
    qd.v[3][0] = *(float *)(vendor_effect_moby + 0x10) + q[3][0];
    qd.v[3][1] = *(float *)(vendor_effect_moby + 0x14) + q[3][1];
    qd.v[3][2] = *(float *)(vendor_effect_moby + 0x18) + q[3][2];
    qd.col[0] = col;
    qd.col[1] = col;
    qd.col[2] = col;
    qd.col[3] = col;
    qd.uv[0][0] = 0.0f;
    qd.uv[0][1] = 0.0f;
    qd.uv[1][0] = 1.0f;
    qd.uv[1][1] = 0.0f;
    qd.uv[2][0] = 0.0f;
    qd.uv[2][1] = 1.0f;
    qd.uv[3][0] = 1.0f;
    qd.uv[3][1] = 1.0f;
    draw_geometry_quad(&qd, 0, 1);

    /* the sparkle vector reuses the first matrix row's slot in retail; its w is that row's 0 */
    izero = 0;
    zero = *(float *)&izero;
    for (i = 0; i < 4; i++) {
        float sparkle_angle = (float)i * 3.14159f * 0.5f - 3.14159f;
        T[0].f[0] = fast_cos(sparkle_angle) * sparkle_radius;
        T[0].f[1] = fast_sin(sparkle_angle) * sparkle_radius;
        T[0].f[2] = 0.59f;
        transform_vector_by_basis(T[0].f, T[0].f, vendor_effect_moby + 0xC0);
        fast_vec_add(T[0].f, T[0].f, vendor_effect_moby + 0x10);
        draw_offset_textured_effect_quad(
            T[0].f, *(int *)(vendor_effect_moby + 0x90) & 0xFFFF0000, 0.1333f, zero);
    }
}

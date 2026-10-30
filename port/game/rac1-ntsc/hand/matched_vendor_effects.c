/* SPDX-License-Identifier: GPL-3.0-or-later */
/* FUN_L00_002a5040 (draw_vendor_effects), the Gadgetron vendor's glow and sparkles. As the port
 * runs it: the NTSC decomp's matching C (src/overlays/shared/vendor/vendor.c) with the second
 * euler_rotation_basis call given d0 itself. The matching C hands it d0's address through an asm
 * statement (`addiu %0,$sp,0xD0` into the register variable d0_arg, frame slot 0xD0 being d0's), a
 * fakematch for retail's instruction order the host cannot run: d0_arg stayed 0 and the sparkles'
 * basis came from guest address 0. Otherwise the matching C, line for line; it goes at the end of
 * vendor.c, whose declarations give the name its address. Not a PS2 match; the decomp's C stays as
 * it is (hostgen.json "overrides": this C replaces it in the port only). */
void draw_vendor_effects(char *m) {
    char    *p = *(char **)(m + 0x78);
    Quad     qd;
    long     pk[4];
    float    q[4][4];
    float    d0[4];
    float    mtx[3][4];
    V_2A62D0 M[4];
    V_2A62D0 T[4];
    float    zero;
    char    *g, *h;
    float(*uv_v)[2];
    float(*uvs)[2];
    int   izero;
    float off;
    /* Fakematch: the sparkle radius 1.1 lives in $f22, copied from the glow block's 1.1. */
    register float sparkle_radius __asm__("$f22");
    int             k, i, i0, i1, i2, i3, col;

    h = (char *)&l00_camera_lui;
    d0[2] = *(float *)(h + 0x158);
    d0[0] = 0.0f;
    d0[1] = 0.0f;
    euler_rotation_basis(mtx, d0);

    vendor_sparkle_uv_scroll = vendor_sparkle_uv_scroll + 0.01f;
    if (1.0f < vendor_sparkle_uv_scroll) {
        vendor_sparkle_uv_scroll = vendor_sparkle_uv_scroll - 1.0f;
    }

    pk[2] = 1;
    pk[3] = 0x4000000044L;
    pk[0] = 0;
    pk[1] = get_effect_texture(0x18);

    for (k = 0; k < 4; k++) {
        transform_vector_by_basis(q[0], D_L00_001D7360[vendor_sparkle_quad_corners[k][0]], mtx);
        transform_vector_by_basis(q[1], D_L00_001D7360[vendor_sparkle_quad_corners[k][1]], mtx);
        transform_vector_by_basis(q[2], D_L00_001D7360[vendor_sparkle_quad_corners[k][2]], mtx);
        transform_vector_by_basis(q[3], D_L00_001D7360[vendor_sparkle_quad_corners[k][3]], mtx);

        qd.v[0][0] = *(float *)(m + 0x10) + q[0][0] * *(float *)(p + 0x90);
        qd.v[0][1] = *(float *)(m + 0x14) + q[0][1] * *(float *)(p + 0x90);
        qd.v[0][2] = *(float *)(m + 0x18) + q[0][2];
        qd.v[1][0] = *(float *)(m + 0x10) + q[1][0] * *(float *)(p + 0x90);
        qd.v[1][1] = *(float *)(m + 0x14) + q[1][1] * *(float *)(p + 0x90);
        qd.v[1][2] = *(float *)(m + 0x18) + q[1][2];
        qd.v[2][0] = *(float *)(m + 0x10) + q[2][0] * *(float *)(p + 0x90);
        qd.v[2][1] = *(float *)(m + 0x14) + q[2][1] * *(float *)(p + 0x90);
        qd.v[2][2] = *(float *)(m + 0x18) + q[2][2];
        qd.v[3][0] = *(float *)(m + 0x10) + q[3][0] * *(float *)(p + 0x90);
        qd.v[3][1] = *(float *)(m + 0x14) + q[3][1] * *(float *)(p + 0x90);
        qd.v[3][2] = *(float *)(m + 0x18) + q[3][2];

        i0 = vendor_sparkle_quad_corners[k][0];
        i1 = vendor_sparkle_quad_corners[k][1];
        i2 = vendor_sparkle_quad_corners[k][2];
        i3 = vendor_sparkle_quad_corners[k][3];
        off = vendor_sparkle_uv_scroll;
        qd.col[0] = vendor_sparkle_colors[i0];
        qd.col[1] = vendor_sparkle_colors[i1];
        qd.col[2] = vendor_sparkle_colors[i2];
        qd.col[3] = vendor_sparkle_colors[i3];
        uvs = D_L00_001D73F0;
        uv_v = (float(*)[2]) & uvs[0][1];
        qd.uv[0][0] = uvs[i0][0];
        qd.uv[0][1] = uv_v[i0][0] + off;
        qd.uv[1][0] = uvs[i1][0];
        qd.uv[1][1] = uv_v[i1][0] + off;
        qd.uv[2][0] = uvs[i2][0];
        qd.uv[2][1] = uv_v[i2][0] + off;
        qd.uv[3][0] = uvs[i3][0];
        qd.uv[3][1] = uv_v[i3][0] + off;
        draw_geometry_quad(&qd, 0, 1);
    }

    pk[1] = get_effect_texture(0x1B);

    vendor_glow_phase = vendor_glow_phase + 0.01f;
    if (1.0f < vendor_glow_phase) {
        vendor_glow_phase = 0.0f;
    }

    {
        float g = vendor_glow_phase;
        float n = -g * 1.2f;
        float pp = g * 1.2f;
        float z = g * 1.4f + 1.1f;
        float zz = z - 0.2f;
        sparkle_radius = 1.1f;

        T[0].q = 0;
        T[0].f[0] = n * *(float *)(p + 0x90);
        T[0].f[1] = n * *(float *)(p + 0x90);
        T[0].f[2] = z;
        T[1].q = 0;
        T[1].f[0] = pp * *(float *)(p + 0x90);
        T[1].f[1] = n * *(float *)(p + 0x90);
        T[1].f[2] = zz;
        T[2].q = 0;
        T[2].f[0] = n * *(float *)(p + 0x90);
        T[2].f[1] = pp * *(float *)(p + 0x90);
        T[2].f[2] = z;
        T[3].q = 0;
        T[3].f[0] = pp * *(float *)(p + 0x90);
        T[3].f[1] = pp * *(float *)(p + 0x90);
        T[3].f[2] = zz;
        M[0].q = T[0].q;
        T[0].q = M[0].q;
        M[1].q = T[1].q;
        T[1].q = M[1].q;
        M[2].q = T[2].q;
        M[3].q = T[3].q;
    }

    g = (char *)&l00_camera_lui;
    off = *(float *)(g + 0x158);
    d0[0] = 0.0f;
    d0[2] = off;
    d0[1] = 0.0f;
    euler_rotation_basis(mtx, d0);
    transform_vector_by_basis(q[0], M[0].f, mtx);
    transform_vector_by_basis(q[1], M[1].f, mtx);
    transform_vector_by_basis(q[2], M[2].f, mtx);
    transform_vector_by_basis(q[3], M[3].f, mtx);

    col = ((int)((1.0f - vendor_glow_phase) * 128.0f) << 24) | 0x808080;
    qd.v[0][0] = *(float *)(m + 0x10) + q[0][0];
    qd.v[0][1] = *(float *)(m + 0x14) + q[0][1];
    qd.v[0][2] = *(float *)(m + 0x18) + q[0][2];
    qd.v[1][0] = *(float *)(m + 0x10) + q[1][0];
    qd.v[1][1] = *(float *)(m + 0x14) + q[1][1];
    qd.v[1][2] = *(float *)(m + 0x18) + q[1][2];
    qd.v[2][0] = *(float *)(m + 0x10) + q[2][0];
    qd.v[2][1] = *(float *)(m + 0x14) + q[2][1];
    qd.v[2][2] = *(float *)(m + 0x18) + q[2][2];
    qd.v[3][0] = *(float *)(m + 0x10) + q[3][0];
    qd.v[3][1] = *(float *)(m + 0x14) + q[3][1];
    qd.v[3][2] = *(float *)(m + 0x18) + q[3][2];
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
        float ang = (float)i * 3.14159f * 0.5f - 3.14159f;
        T[0].f[0] = fast_cos(ang) * sparkle_radius;
        T[0].f[1] = fast_sin(ang) * sparkle_radius;
        T[0].f[2] = 0.59f;
        transform_vector_by_basis(T[0].f, T[0].f, m + 0xC0);
        fast_vec_add(T[0].f, T[0].f, m + 0x10);
        draw_offset_textured_effect_quad(T[0].f, *(int *)(m + 0x90) & 0xFFFF0000, 0.1333f, zero);
    }
}

/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Native-only PAL 001FA1C0..001FA1F4: the VU constructor writes a complete
 * uniform-scale matrix, including its translation/homogeneous row.
 * Not a matching PS2 decompilation. */
void func_001FA1C0(float* matrix, float scale) {
    int i;
    for (i = 0; i < 16; ++i) matrix[i] = 0.0f;
    /* Retail adds the input scalar to cleared lanes, including for -0. */
    matrix[0] = matrix[5] = matrix[10] = 0.0f + scale;
    matrix[15] = 1.0f;
}

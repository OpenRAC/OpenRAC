/*
 * Retained manual C reconstruction; not linked or counted.
 * Profile: -O2 -G0 -ffunction-sections -frac2-zero-ti-por
 * Retail: 292 bytes; compiled: 292 bytes.
 * Final coverage: us: 0/1; us2: 0/1; pal: 0/1.
 * Residual: 292/292 bytes with the correct five saved FPRs and full 64-byte matrix. The
 * third float controls and feeds func_002ACEC0; func_002830C0 receives the transform.
 * Zero temporary, initial matrix-copy registers, prologue scheduling and loop-branch
 * layout still differ.
 * Reopen: A real initial-axis/matrix load-lifetime witness. Initializing the axis after
 * row copies tied. Whole-aggregate assignment emitted 324 bytes through 64-bit
 * transfers; explicit aligned TI rows retain 292. No constant-folded replacement for the
 * live third float.
 */
#include "common.h"

typedef int TI128 __attribute__((mode(TI)));
typedef union { TI128 q; float f[4]; } Vector;

typedef struct { TI128 row[4]; } Matrix64 __attribute__((aligned(16)));
extern Matrix64 D_L14_001AEE10;
extern void func_002ACEC0(void *, void *, void *, float);
extern void func_002830C0(void *, void *, void *);

void func_L14_00390EC0(char *output, char *transform, float yScale, float zScale, float angle) {
    Vector axis;
    Matrix64 rows;
    int i;
    rows.row[0] = D_L14_001AEE10.row[0];
    rows.row[1] = D_L14_001AEE10.row[1];
    rows.row[2] = D_L14_001AEE10.row[2];
    rows.row[3] = D_L14_001AEE10.row[3];
    axis.q = 0;
    axis.f[0] = 1.0f;
    axis.f[3] = 1.0f;
    for (i = 0; i < 4; i++) {
        qcopy(output, &rows.row[i]);
        *(float *)(output + 4) *= yScale;
        *(float *)(output + 8) *= zScale;
        if (angle != 0.0f)
            func_002ACEC0(output, output, &axis, angle);
        *(float *)(output + 0xC) = 1.0f;
        func_002830C0(output, output, transform);
        output += 0x10;
    }
}

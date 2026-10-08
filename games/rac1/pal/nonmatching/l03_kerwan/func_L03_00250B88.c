/* NON_MATCHING func_L03_00250B88 -- src/overlays/l03_kerwan/mobyutil_00250B88.c
 * Best so far: BYTES 13/316 (95.9% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Builds a 4x4 cross-product matrix from vec a, Rodrigues-style rotation via func_L00_001FFCF8/FFDD0, result int
 *   (w05) Builds a skew-symmetric axis matrix on the stack, then a Rodrigues rotation via matrix helpers. Best thi
 *   Only difference: FP register choice (z/y/x -> f0/f5/f3 in retail) and order of the m[2]/m[4]/m[6]/m[0]/m[8] st
 */
typedef int u128 __attribute__((mode(TI)));
extern void func_001FA540(void *, void *, void *);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_L00_001FFCF8(void *, void *, float);
extern void func_L00_001FFDD0(void *, void *, void *);
extern void func_001FA190(void *);
extern void func_002153E8(void *, void *);

/* Build a rotation matrix about an axis (Rodrigues) from vector a and angle, store its result. */
void func_L03_00250B88(float *pa, float *pb, float ang) {
    float a[4] __attribute__((aligned(16)));
    float b[4] __attribute__((aligned(16)));
    float m[16] __attribute__((aligned(16)));
    float t60[16] __attribute__((aligned(16)));
    float tA0[16] __attribute__((aligned(16)));
    float tE0[16] __attribute__((aligned(16)));
    float t120[16] __attribute__((aligned(16)));
    float *bp = b; float x, y, z;
    *(u128 *)a = *(u128 *)pa;
    *(u128 *)bp = *(u128 *)pb; z = a[2]; y = a[1]; x = a[0];
    m[0] = 0.0f; m[8] = y; m[6] = x; m[4] = -z; m[9] = -x; m[2] = -y; m[1] = z;
    m[12] = 0.0f; m[5] = 0.0f; m[13] = 0.0f; m[10] = 0.0f; m[14] = 0.0f; m[3] = 0.0f; m[7] = 0.0f; m[11] = 0.0f; m[15] = 0.0f;
    func_001FA540(t60, m, m);
    func_L00_001FFCF8(tE0, t60, 1.0f - func_001F9F90(ang));
    func_L00_001FFCF8(tA0, m, func_001F9FA8(ang));
    func_L00_001FFDD0(tA0, tA0, tE0);
    func_001FA190(tE0);
    func_L00_001FFDD0(t120, tA0, tE0);
    func_002153E8(t120, bp);
}

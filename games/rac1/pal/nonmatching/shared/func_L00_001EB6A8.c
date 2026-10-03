/* NON_MATCHING func_L00_001EB6A8 -- src/overlays/shared/camera_001EB508.c
 * Best so far: SIZE ours 284 / retail 280, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Steers *p by (c*r - d*(*p)) where r = func_001FA790(b, a) (returns float, needs an asm-alias declaration becau
 *   p2.c compiles to the packet's 70 instructions identically by eye (registers, branch-likely delay slots, all). 
 */
extern float func_001FA790_f(float, float) __asm__("func_001FA790");
extern float func_001F9B88(float);
extern float func_001FA748(float, float);

/* steers *p toward a target with damping, clamps it, then calls FA748 */
void func_L00_001EB6A8(float *p, float a, float b, float c, float d, float lim) {
    float r = func_001FA790_f(b, a);
    float v;
    *p = *p + (c * r - d * *p);
    if (lim != 0.0f) {
        if (lim < *p) {
            *p = lim;
        } else if (*p < -lim) {
            *p = -lim;
        }
    }
    if (func_001F9B88(r) < *p) {
        *p = func_001F9B88(r);
    } else if (*p < -func_001F9B88(r)) {
        *p = -func_001F9B88(r);
    }
    func_001FA748(a, *p);
}

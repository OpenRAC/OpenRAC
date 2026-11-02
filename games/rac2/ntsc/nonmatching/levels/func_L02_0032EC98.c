/* from src/levels/shared/L02_002B5F50.c: not matched. Retail 320 bytes, 9 place(s), first word
 * at L02:0032EC98.
 *
 * The body builds the difference of two positions, turns it into a frame, and clamps an angle
 * to it: three vectors on the stack (at +0x00, +0x10, +0x20), the table entry's count and
 * vertices through func_00282CF0 / func_00282CC0 / func_00282F20, two angles from
 * func_00283248, and the clamp of the second one between the two halves of |f20|.
 *
 * The C below compiles to the same 320 bytes and every instruction has retail's counterpart in
 * the same order, except for the two float registers the first angle and the first float
 * argument end up in: retail keeps the angle in $f21 and the argument in $f22, this cc1 keeps
 * the argument in $f21 and the angle in $f22. The stack slots and the whole body agree, so
 * only the order in which this cc1 hands out the callee-saved float registers differs.
 *
 * What was tried and did not change it: the two float locals declared in either order; the
 * first float argument used directly or copied into a local; the second angle held in its own
 * local or inlined into the call that uses it. The allocation gives the argument the leftover
 * half of the $f20 pair and the angle a fresh $f22; retail gives the angle the leftover half
 * and the argument the fresh $f22.
 *
 * What would unblock it: a source form in which the angle's pseudo is ranked above the
 * argument's, or a compiler profile that ranks them the other way. */
extern void func_00282F20(float *out, const float *in, float s);
extern void func_00282CC0(void *a0, void *a1, void *a2);
extern void func_00282CF0(float *out, const float *a, const float *b);
extern float func_00283248(float a, float b);
extern float func_00282C48(float t);
extern float func_00283BE0(float a, float b);
extern float func_00283B98(float a, float b);

float func_L02_0032EC98(void *a0, void *a1, void *a2, float f12, float f13) {
    float v0[4];
    float v1[4];
    float v2[4];
    float f20;
    float f22;
    float f21;
    float f;

    func_00282CF0(v0, (float *)a1, (float *)a0);
    func_00282F20(v0, v0, f13);
    func_00282CC0(v1, a0, v0);
    func_00282CF0(v2, (float *)a1, v0);

    f21 = func_00283248(v1[0] - *(float *)a2, v1[1] - *(float *)((char *)a2 + 4));
    f20 = func_00283BE0(func_00283248(v2[0] - *(float *)a2,
                                      v2[1] - *(float *)((char *)a2 + 4)), f21) * 0.5f;
    f21 = func_00283B98(f20, f21);
    f22 = func_00283BE0(f12, f21);
    f = func_00282C48(f20);
    if (f < f22)
        f22 = f;
    f = -f;
    if (f22 < f)
        f22 = f;
    return func_00283B98(f22, f21);
}

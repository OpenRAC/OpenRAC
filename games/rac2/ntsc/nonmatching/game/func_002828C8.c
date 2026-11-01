/* func_002828C8 - retained C near miss, not linked. Current-master -G8 probe:
 * 44/40 bytes, 0/1 boot place. A 28 October 2026 `-ffast-math` probe exposed
 * `adda.s`/`madd.s` and the correct GP-relative load but remained 44 bytes;
 * spelling the offset as `(x * D + 0.25f) + 0.25f` grew to 48 bytes. Keep the
 * marker: tested forms still miss retail's ordering and accumulator seed.
 * The retail sequence loads D_001A7910 into $f2,
 * seeds ACC with 0.25f + 0.25f using `adda.s`, then computes ACC + (float)a0 *
 * D_001A7910 with `madd.s` before `cvt.w.s`. The corresponding C expression is
 * below. The probe emits separate `mul.s`/`add.s`, materialises 0.5f directly,
 * and emits the data load at 0($gp) instead of -0x76E0($gp). Keep the marker;
 * the accumulator schedule and small-data reference remain unresolved. This
 * is not evidence for handwritten assembly. Public instruction descriptions:
 * https://ps2tek.github.io/.
 */
typedef float f32;
typedef int s32;

extern f32 D_001A7910 __attribute__((sda));

int func_002828C8(s32 a0) {
    return (s32)((f32)a0 * D_001A7910 + 0.5f);
}

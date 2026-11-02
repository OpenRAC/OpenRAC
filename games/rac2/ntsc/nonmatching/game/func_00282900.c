/* func_00282900 - retained C near miss, not linked. Current-master -G8 probe:
 * 44/40 bytes, 0/1 boot place. A 28 October 2026 `-ffast-math` probe exposes
 * `adda.s`/`madd.s` but remains 44/40; spelling the offset as
 * `(x * D + 0.25f) + 0.25f` grows to 48 bytes. The retail sequence loads
 * D_001A7914 into $f2,
 * seeds ACC with 0.25f + 0.25f using `adda.s`, then computes ACC + (float)a0 *
 * D_001A7914 with `madd.s` before `cvt.w.s`. The corresponding C expression is
 * below. The probe emits separate `mul.s`/`add.s`, materialises 0.5f directly,
 * and emits the data load at 0($gp) instead of -0x76DC($gp). Keep the marker;
 * the accumulator schedule and small-data reference remain unresolved. This
 * is not evidence for handwritten assembly. Public instruction descriptions:
 * https://ps2tek.github.io/.
 */
typedef float f32;
typedef int s32;

extern f32 D_001A7914 __attribute__((sda));

int func_00282900(s32 a0) {
    return (s32)((f32)a0 * D_001A7914 + 0.5f);
}

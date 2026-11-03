/* func_00282890 - retained C near miss, not linked. Current-master -G8 probe:
 * 44/40 bytes, 0/1 boot place. A 28 October 2026 `-ffast-math` probe exposed
 * `adda.s`/`madd.s` but remained 44 bytes and seeded ACC with 0.5 + 0.5; spelling
 * the source as `(x * D + 0.25f) + 0.25f` grew to 48 bytes. Adding
 * `-fno-schedule-insns` to the 0.5f form left the 44-byte output unchanged.
 * No tested form matches retail's order. The retail sequence loads D_001A790C into $f2,
 * seeds ACC with 0.25f + 0.25f using `adda.s`, then computes ACC + (float)a0 *
 * D_001A790C with `madd.s` before `cvt.w.s`. The corresponding C expression is
 * below. The probe emits separate `mul.s`/`add.s`, materialises 0.5f directly,
 * and emits the data load at 0($gp) instead of -0x76E4($gp). `__builtin_fmaf`
 * was also tried, but this compiler emitted an 80-byte helper-call sequence.
 * Keep the marker until the EE accumulator schedule and correct small-data
 * reference can be expressed in accepted C; no handwritten classification is
 * proposed. Public instruction descriptions: https://ps2tek.github.io/.
 */
typedef float f32;
typedef int s32;

extern f32 D_001A790C __attribute__((sda));

int func_00282890(s32 a0) {
    return (s32)((f32)a0 * D_001A790C + 0.5f);
}

/* func_L00_003F5870 (384 bytes, 27 places) - NEAR MISS, worker 6, 28 October 2026.
 * Profile: src/levels/shared/L00_003F5870-g8.c, "-O2 -G8 -ffunction-sections -frac2-zero-ti-por".
 * 384/384 bytes, 2 masked diff lines (was about 17).
 * What fixed it: the sum in the `||` condition is its own pointer local, `cur`, assigned inside the
 * condition as `cur = (char *)(index * 0x30 + (int)base)` (product first, as retail's addu), and the
 * else arm uses `cur` for +0x10/+0x20/+0x24; the tail keeps its own `rec`. That keeps the product in
 * $s0 and the sum in $s2 as retail does.
 * Left: the third argument of func_00282CF0. Retail: `addu $a2,$s0,$s4; ...; addiu $a2,$a2,-0x20`
 * (an un-merged base+product, then -0x20). Ours: `addiu $a2,$s0,-0x20; addu $a2,$a2,$s4`. Every
 * association of index*0x30, (int)base and -0x20 was tried: either this (2 lines) or cse merges it
 * into cur-0x20 (380 B).
 */
ore it, scaled by the joint's length. */
void func_L00_003F5870(char *base, int index)
{
    float up[4] __attribute__((aligned(16)));
    float dir[4] __attribute__((aligned(16)));
    float diff[4] __attribute__((aligned(16)));
    float normal[4] __attribute__((aligned(16)));
    char *rec;
    char *cur;

    *(TI128 *)up = 0;
    up[2] = 1.0f;
    if (ref_L00_003F5870_00 != 0)
        func_002AFFD8(up, base + index * 0x30 + 0x10, 1);
    if ((float)index == 0.0f
        || func_00282C48(*(float *)((cur = (char *)(index * 0x30 + (int)base)) + 0x20)) < 0.00001f) {
        func_00282D30(dir, up, *(float *)(base + index * 0x30 + 0x24));
    } else {
        func_00282CF0(diff, cur + 0x10, (char *)(index * 0x30 + ((int)base - 0x20)));
        func_00282F20(normal, diff, 1.0f);
        func_002AB668(normal, normal, *(float *)(cur + 0x20));
        func_002ACDE8(dir, up, normal);
        func_00282D30(dir, dir, *(float *)(cur + 0x24));
    }
    rec = base + index * 0x30;
    func_00282CC0(rec, rec + 0x10, dir);
    *(float *)(rec + 0xC) = ref_L00_003F5870_01;
}

/* Retained near miss, 24 October 2026.
 * Game `-O2 -G0 -ffunction-sections` (the default unit of 002DDEC8.c).
 * USA v1.01, PAL and USA v2.00: 192/196 bytes, 0/28 places each.
 * Seven checker invocations, no scratch compiles.
 *
 * The chunk-table walk, the ternary mode/flag test (retail's `beqz $a0` plus
 * `xori`/`andi`), the two func_002DE6F0 calls, the func_00282A60 fill and the
 * key return are represented. The 4-byte residual is one entry copy: retail
 * keeps `mode` in $a0 across the loop's continue paths, because only the
 * returning path clobbers $a0; this source keeps it in $t1. A struct-pointer
 * source grew the frame to 0x20 and saved $s1; the pointer-array form is 184
 * bytes; the index form is 192. Reading the key into a local again is 160
 * bytes. All tested forms keep the extra entry copy.
 *
 * Reopen with evidence for the original parameter lifetime or a source form
 * that lets the allocator keep mode in $a0, not with another spelling of the
 * same table walk.
 */
extern int D_001B1F50[] __attribute__((sda));
extern int D_001B1F54[] __attribute__((sda));
extern int func_002DE6F0(int key);
extern void func_00282A60(void *data, unsigned int tag, int size);

int func_002DE550(int mode)
{
    int i;
    int size;

    for (i = 0; i < 5; i++) {
        if ((mode ? (D_001B1F54[i * 2] ^ 1) : (D_001B1F54[i * 2] & 1)) != 0)
            continue;
        if (D_001B1F50[i * 2] == 0)
            continue;
        if (D_001B1F54[i * 2] & 2)
            continue;
        D_001B1F54[i * 2] |= 2;
        func_002DE6F0(D_001B1F50[i * 2]);
        size = func_002DE6F0(D_001B1F50[i * 2]);
        func_00282A60((void *)D_001B1F50[i * 2], 0xDEADBEEF, size);
        return D_001B1F50[i * 2];
    }
    return 0;
}

/* NON_MATCHING func_L04_002D27F8 -- src/overlays/l04_eudora/vendor_002CB800.c
 * Best so far: BYTES 42/496 (91.5% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * No longer builds in its file (COMPILE failed, 2026-10-09): match its declarations to the file's first.
 * What the last attempts found:
 *   Copies 13 words from the parent's table into the moby data block, then sets byte flags and float defaults (x D
 *   Best p3.c (BYTES 42/496): the 13-word copy matches; the float block differs only in FPU register numbering (re
 *   Unblock: some form that makes the allocator number the float temporaries ascending; locals order, store order 
 */
extern void func_L04_00293530(void *unused, char *arg);
extern void func_L04_00293578(void *, void *);
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;

// Fills a moby's data block from its parent's table and sets particle/spin defaults.
void func_L04_002D27F8(char *m) {
    unsigned char *d = *(unsigned char **)(m + 0x78);
    float e, c, b, a, s;
    char *t = (char *)d + 0x160;
    *(int *)((char *)d + 0x1D0) = *(int *)(*(char **)(*(char **)(m + 0x24) + 0x70) + 0x14);
    *(int *)((char *)d + 0x1D4) = *(int *)(*(char **)(*(char **)(m + 0x24) + 0x70) + 0x14);
    *(int *)((char *)d + 0x1D8) = *(int *)(*(char **)(*(char **)(m + 0x24) + 0x70) + 0x14);
    *(int *)((char *)d + 0x1DC) = *(int *)(*(char **)(*(char **)(m + 0x24) + 0x70) + 0x14);
    *(int *)((char *)d + 0x1E0) = *(int *)(*(char **)(*(char **)(m + 0x24) + 0x70) + 0x14);
    *(int *)((char *)d + 0x1E4) = *(int *)(*(char **)(*(char **)(m + 0x24) + 0x70) + 0x14);
    *(int *)((char *)d + 0x1E8) = *(int *)(*(char **)(*(char **)(m + 0x24) + 0x70) + 0x14);
    *(int *)((char *)d + 0x1EC) = *(int *)(*(char **)(*(char **)(m + 0x24) + 0x54) + 0x14);
    *(int *)((char *)d + 0x1F0) = *(int *)(*(char **)(*(char **)(m + 0x24) + 0x70) + 0x14);
    *(int *)((char *)d + 0x1F4) = *(int *)(*(char **)(*(char **)(m + 0x24) + 0x70) + 0x14);
    *(int *)((char *)d + 0x1F8) = *(int *)(*(char **)(*(char **)(m + 0x24) + 0x70) + 0x14);
    *(int *)((char *)d + 0x1FC) = *(int *)(*(char **)(*(char **)(m + 0x24) + 0x70) + 0x14);
    *(int *)((char *)d + 0x200) = *(int *)(*(char **)(*(char **)(m + 0x24) + 0x70) + 0x14);
    func_L04_00293530(m, t);
    s = D_0015EE70;
    a = s * 9.8f;
    b = s * 3.1415927f;
    c = s * 10.0f;
    e = D_0015EE6C * 6.2831855f;
    s = s * 6.2831855f;
    d[0x210] = 0;
    d[0x211] = 0;
    *(float *)(d + 0x23C) = a;
    *(float *)(d + 0x22C) = b;
    *(float *)(d + 0x230) = s;
    *(float *)(d + 0x234) = e;
    *(float *)(d + 0x224) = c;
    *(float *)(d + 0x240) = 0.5f;
    d[0x212] = 0xFF;
    d[0x213] = 0x10;
    d[0x215] = 7;
    d[0x250] = 10;
    d[0x251] = 8;
    d[0x253] = 0xC;
    d[0x252] = 0xE;
    d[0x300] = 0xB;
    d[0x301] = 9;
    d[0x303] = 0xD;
    d[0x302] = 0xF;
    d[0x214] = 7;
    func_L04_00293578(m, t);
}

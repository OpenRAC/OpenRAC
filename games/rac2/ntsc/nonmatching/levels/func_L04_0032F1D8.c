/* 0 of 5 places match - 280 bytes against retail's 360; last line of tools/check.py:
   04_barlow: 280 bytes, expected 360

   The body's +0xC word is 0x48A00000, a VU0 word (qmtc2.ni $zero, $vf0): a quadword
   move to the vector unit's register 0. This cc1 has no COP2 pattern at all, and the
   only word it emits for a quadword clear is the `por` (0x70xxxxxx) the game uses
   elsewhere, always followed by the `sq` that stores it. Plain C cannot emit the
   retail word, so this function is not a C candidate as it stands.

   What is left of the body is a plain copy: sixteen groups of two sixteen-byte blocks
   (the destination stride is 0x60, the source stride 0x40), then four groups of three
   sixteen-byte blocks, then sixteen groups of three words, and six words at the end.
   The draft below spells that out; it still needs the loop bodies and the register
   roles of retail (the inner loop copies 32 bytes per pass and counts down from two,
   the outer counts down from sixteen), and the VU0 word above.
*/
typedef unsigned int u32;
typedef unsigned char u8;
typedef int L04Quad __attribute__((mode(TI)));

void func_L04_0032F1D8(void *a0, void *a1) {
    u8 *dst = (u8 *)a0;
    u8 *src = (u8 *)a1;
    u8 *d = dst + 0x70;
    u8 *s = src;
    s32 i, j;

    for (i = 0; i < 16; i++) {
        for (j = 0; j < 2; j++) {
            L04Quad *q = (L04Quad *)d;
            L04Quad *r = (L04Quad *)s;

            q[0] = r[0];
            q[1] = r[1];
            d += 0x20;
            s += 0x20;
        }
        d += 0x20;
    }
    for (i = 0; i < 4; i++) {
        *(L04Quad *)(dst + 0x10 + i * 0x10) = *(L04Quad *)(src + 0x400 + i * 0x10);
        *(L04Quad *)(dst + 0x670 + i * 0x10) = *(L04Quad *)(src + 0x480 + i * 0x10);
        *(L04Quad *)(dst + 0x6D0 + i * 0x10) = *(L04Quad *)(src + 0x500 + i * 0x10);
    }
    for (i = 0; i < 16; i++) {
        *(u32 *)(dst + 0x6C + i * 0x60) = *(u32 *)(src + 0x440 + i * 4);
        *(u32 *)(dst + 0xB0 + i * 0x60) = *(u32 *)(src + 0x4C0 + i * 4);
        *(u32 *)(dst + 0xB4 + i * 0x10) = *(u32 *)(src + 0x540 + i * 4);
    }
    *(u32 *)(dst + 0xC) = *(u32 *)(src + 0x580);
    *(u32 *)(dst + 0x50) = *(u32 *)(src + 0x584);
    *(u32 *)(dst + 0x66C) = *(u32 *)(src + 0x588);
    *(u32 *)(dst + 0x6B0) = *(u32 *)(src + 0x58C);
    *(u32 *)(dst + 0x6B4) = *(u32 *)(src + 0x590);
    *(u32 *)(dst + 0x710) = *(u32 *)(src + 0x594);
}

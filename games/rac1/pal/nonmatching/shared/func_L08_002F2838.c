/* NON_MATCHING func_L08_002F2838 -- src/overlays/shared/vendor_002D3DF8.c
 * Best so far: BYTES 42/228 (81.6% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern float D_0015EE7C MACRO_ADDR;
extern short D_L08_00161E70_h __asm__("D_L08_00161E70");
extern char D_L08_00161E74_a[] __asm__("D_L08_00161E74");
extern char D_L08_00161EA0_a[] __asm__("D_L08_00161EA0");
extern char D_L08_00161EA4_a[] __asm__("D_L08_00161EA4");

/* Scrolls the two texture offsets of scroller idx (8-byte records at D_L08_00161EA0) by their
 * speeds (records at D_L08_00161E70/E74) times the frame time, wrapping each into [-1, 1]. */
void func_L08_002F2838(int idx) {
    int off = idx << 3;
    float *p;
    p = (float *)(off + (int)D_L08_00161EA0_a);
    *p = *p + *(float *)(off + (int)&D_L08_00161E70_h) * D_0015EE7C;
    if (1.0f < *p) {
        *p = *p - 1.0f;
    }
    if (*p < -1.0f) {
        *p = *p + 1.0f;
    }
    p = (float *)(off + (int)D_L08_00161EA4_a);
    *p = *p + *(float *)(off + (int)D_L08_00161E74_a) * D_0015EE7C;
    if (1.0f < *p) {
        *p = *p - 1.0f;
    }
    if (*p < -1.0f) {
        *p = *p + 1.0f;
    }
}

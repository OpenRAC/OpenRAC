/* NON_MATCHING func_L13_002B60D8 -- src/overlays/l13_gemlik/vendor_002B2020.c
 * Best so far: BYTES 8/140 (94.3% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern void *func_L13_002811B0(void *, int, int, int, float, float, float, int);
extern void func_L00_002688A8(void *);

void func_L13_002B60D8(unsigned char *moby, char *data) {
    float opacity = 0.2f;
    void **handle = (void **)(data + 0x228);
    if (moby[0x31]) {
        if (!*handle) {
            *handle = func_L13_002811B0(moby, 0, 0x80808080, 0x10808080, opacity, opacity, 0.8f, 0x19);
        }
    } else if (*handle) {
        func_L00_002688A8(*handle);
        *handle = 0;
    }
}

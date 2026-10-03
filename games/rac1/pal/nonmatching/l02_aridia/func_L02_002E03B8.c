/* NON_MATCHING func_L02_002E03B8 -- src/overlays/l02_aridia/vendor_002A59D8.c
 * Best so far: BYTES 24/304 (92.1% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L02_002E03B8: per-frame update of two linked mobys (moby+0x84/0x88): calls func_00214D88 for each, then e
 *   Only difference (24 of 304 bytes): after the first func_001FA748 store, retail loads D_L02_00161CCC (lwc1 -0x5
 *   Would need a source shape that changes the scheduling priority of the second statement's gp load; not found.
 */
extern float func_00214D88(float *, float *, float, float, float, float);
extern float func_001FA748(float, float);
extern void func_L00_00251E30(void *);
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern short D_L02_00161CC0;
extern short D_L02_00161CC4;
extern short D_L02_00161CC8;
extern short D_L02_00161CCC;

// Updates two linked mobys' heights and angles toward this moby.
void func_L02_002E03B8(char *moby) {
    char *d = *(char **)(moby + 0x78);
    func_00214D88((float *)(*(char **)(d + 0x84) + 0x18), (float *)(d + 0x8C),
                  *(float *)(moby + 0x18) + *(float *)(d + 0x94),
                  *(float *)&D_L02_00161CC4 * D_0015EE70,
                  *(float *)&D_L02_00161CC8 * D_0015EE70,
                  *(float *)&D_L02_00161CC0 * D_0015EE6C);
    func_00214D88((float *)(*(char **)(d + 0x88) + 0x18), (float *)(d + 0x90),
                  *(float *)(moby + 0x18) + *(float *)(d + 0x98),
                  *(float *)&D_L02_00161CC4 * D_0015EE70,
                  *(float *)&D_L02_00161CC8 * D_0015EE70,
                  *(float *)&D_L02_00161CC0 * D_0015EE6C);
    *(float *)(*(char **)(d + 0x84) + 0x48) =
        func_001FA748(*(float *)(moby + 0x48),
                      (*(float *)(*(char **)(d + 0x84) + 0x18) - *(float *)(moby + 0x18)) * *(float *)&D_L02_00161CCC * 0.017453292f);
    *(float *)(*(char **)(d + 0x88) + 0x48) =
        func_001FA748(*(float *)(moby + 0x48),
                      (*(float *)(*(char **)(d + 0x88) + 0x18) - *(float *)(moby + 0x18)) * *(float *)&D_L02_00161CCC * 0.017453292f);
    func_L00_00251E30(*(void **)(d + 0x84));
    func_L00_00251E30(*(void **)(d + 0x88));
}

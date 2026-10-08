/* NON_MATCHING func_L07_00314590 -- src/overlays/l07_umbris/vendor_00313D28.c
 * Best so far: BYTES 12/416 (97.1% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Emits layered effects and resolves movement along scaled displacement; p1/p2/p3 compile identically at BYTES 1
 *   Stopped at three distinct equivalent wordings: only first effect-call f12/f13/zero argument/color-low setup or
 *   Needs natural call argument scheduling change; the rest matches.
 */
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_L07_00314250(void *, int, int, float, float);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_0025A8C0(void *, void *, int, float, void *);
extern int func_L00_001EFFF0(void *, void *, int, int, int);
extern int func_L00_001F2BE8(float, void *, int, void *, void *);
/* Emits layered effects and resolves movement along a scaled displacement. */
void func_L07_00314590(char *moby) {
    float displacement[4];
    char hit[48];
    float direction[4];
    char *data = *(char **)(moby + 0x78);
    void *origin;
    void *position;
    qzero(displacement);
    origin = data + 0x110;
    position = data + 0x120;
    func_L00_001FF4B0(displacement, data + 0x130, *(float *)(data + 0x16C) * 0.8f);
    func_001F9BD8(data + 0x140, displacement, origin);
    func_L00_001FF4B0(displacement, displacement, *(float *)(data + 0x16C));
    func_001F9BD8(position, displacement, origin);
    func_L07_00314250(moby, 0, 0x20000080, 1.0f, 1.0f);
    func_L07_00314250(moby, 10, 0x28008080, 0.7f, 2.0f);
    func_L07_00314250(moby, 20, 0x30B0FFFF, 0.4f, 3.0f);
    func_001F9BF0(direction, position, origin);
    func_L00_001FF4B0(direction, direction, 1.0f);
    func_L00_0025A8C0(hit, moby, 0x30000, 1.000123f, direction);
    if (!func_L00_001EFFF0(origin, position, 0, (int)moby, (int)hit)) {
        func_L00_001F2BE8(0.5f, position, 0, moby, hit);
    }
}

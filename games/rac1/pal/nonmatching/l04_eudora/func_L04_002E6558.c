/* NON_MATCHING func_L04_002E6558 -- src/overlays/l04_eudora/vendor_002CB800.c
 * Best so far: BYTES 26/548 (95.3% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Bouncing moby update: spins rotation toward targets, in state 0 reflects velocity off a plane (func_001F9C78 d
 *   Best p4.c (548 bytes, 26 bytes differ): everything right except the scheduling of the second func_L00_00258C80
 *   qcopy() was needed for the two lq/sq copies, short + *(float*)& for the $gp floats.
 */
typedef int u128 __attribute__((mode(TI)));
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern char D_L04_00174080[];
extern short D_L04_00161E08;
extern short D_L04_00161E0C;
extern void func_001F9BD8(void *, void *, void *);
extern float func_001FA748(float, float);
extern int func_001F9938(void *);
extern int func_L00_001F10E0(float, void *, int, void *);
extern float func_001F9C78(void *a, void *b);
extern void func_L00_001FF610(void *, void *, void *);
extern void func_001F9C30(void *, void *, float);
extern float func_L00_00258C80(float lo, float hi);
extern void func_0020D678(void *);

// Updates a bouncing moby: spins it, then in state 0 reflects its direction off a plane and in state 1 fades it out.
void func_L04_002E6558(char *moby) {
    float old[4];
    char *pos = moby + 0x10;
    char *data = *(char **)(moby + 0x78);
    char *vel;
    qcopy(old, pos);
    vel = data + 0x10;
    *(float *)(data + 0x18) = *(float *)(data + 0x18) - D_0015EE70 * 20.0f;
    func_001F9BD8(pos, pos, vel);
    *(float *)(moby + 0x40) = func_001FA748(*(float *)(moby + 0x40), *(float *)(data + 0x20));
    *(float *)(moby + 0x44) = func_001FA748(*(float *)(moby + 0x44), *(float *)(data + 0x24));
    *(float *)(moby + 0x48) = func_001FA748(*(float *)(moby + 0x48), *(float *)(data + 0x28));
    switch ((unsigned char)moby[0x20]) {
    case 0:
        if (*(float *)(moby + 0x18) < *(float *)(data + 4) - 3.0f) {
            moby[0x20] = 1;
        } else if (func_001F9938(data + 0xE) != 0 && *(short *)(data + 0xC) != 0) {
            if (func_L00_001F10E0(*(float *)(data + 8), pos, 2, 0) != 0) {
                char *n = D_L04_00174080;
                float zero;
                if (func_001F9C78(vel, n) < (zero = 0.0f)) {
                    qcopy(pos, n - 0x10);
                    *(unsigned short *)(data + 0xC) = *(unsigned short *)(data + 0xC) - 1;
                    func_L00_001FF610(vel, vel, n);
                    func_001F9C30(vel, vel, 0.75f);
                    *(float *)(data + 0x20) = zero;
                    *(float *)(data + 0x24) = func_L00_00258C80(*(float *)&D_L04_00161E08, *(float *)&D_L04_00161E0C) * 0.017453292f * D_0015EE6C;
                    *(float *)(data + 0x28) = func_L00_00258C80(*(float *)&D_L04_00161E08, *(float *)&D_L04_00161E0C) * 0.017453292f * D_0015EE6C;
                }
            }
        }
        break;
    case 1: {
        unsigned t;
        *(float *)(moby + 0x2C) = *(float *)(moby + 0x2C) * 0.9f;
        t = (unsigned char)moby[0x23];
        if (t < 4) {
            func_0020D678(moby);
        } else {
            moby[0x23] = t - 4;
        }
        break;
    }
    }
}

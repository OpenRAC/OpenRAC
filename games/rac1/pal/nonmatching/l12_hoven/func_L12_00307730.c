/* NON_MATCHING func_L12_00307730 -- src/overlays/l12_hoven/vendor_002EDAA0.c
 * Best so far: SIZE ours 400 / retail 404, checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Randomizes member speed and rotation; p2/p3/p4 compile identically at SIZE 400/404.
 *   Stopped at three distinct equivalent wordings: member/data saved-register assignment and global float load/pro
 *   Needs a natural source shape changing allocation and delaying stores; best current is p2.
 */
extern float func_002140F8(float, float);
extern float func_L00_00258C80(float, float);
extern int D_L12_001FB948[];
extern char *D_L12_00160058 MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
/* Randomizes per-member speed and rotation parameters. */
void func_L12_00307730(char *moby, int index) {
    char *data = *(char **)(moby + 0x78);
    char *member;
    int offset;
    float *speed;
    char *rotation;
    if (index == 17) { member = moby; offset = 0x44; }
    else { offset = index * 4; member = D_L12_00160058 + D_L12_001FB948[index] * 256; }
    { float value = func_002140F8(D_0015EE70 * 3.7f, D_0015EE70 * 5.0f);
    speed = (float *)(data + 0x570) + offset / 4;
    *speed = value; }
    { float low = 0.052359879f * D_0015EE6C;
    float high = 0.13962634f * D_0015EE6C;
    float value = func_L00_00258C80(low, high);
    rotation = data + index * 16;
    *(float *)(rotation + 0x450) = value; }
    *(float *)(rotation + 0x454) = func_L00_00258C80(0.052359879f * D_0015EE6C, D_0015EE6C * 0.17453292f);
    if (*(short *)(member + 0xA6) == 0x4FD) {
        *(float *)(rotation + 0x45C) = func_002140F8(0.001f, 0.0045f);
        *speed = func_002140F8(D_0015EE70 * 5.5f, D_0015EE70 * 9.0f);
    } else if (*(short *)(member + 0xA6) == 0x500) *speed = D_0015EE70 * 13.0f;
}

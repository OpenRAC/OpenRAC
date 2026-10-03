/* NON_MATCHING func_L14_002F25C8 -- src/overlays/l14_oltanis/vendor_002E0538.c
 * Best so far: SIZE ours 288 / retail 292, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Phase switch (jump table 0..4) picks target from D_L14_001B0F30[data->0x94/98/9C/A0], then sets data->0xC0/C4 
 *   Best: p10.c/p3.c: only register allocation differs (x=D6C*5 lands in $f1, retail $f2; y in $f2 vs $f1) so load
 *   Unblock: some pseudo numbering / live-range change that gives the D_0015EE6C product the higher FP register.
 */
extern float func_001F9D10(void *, void *);
extern float D_0015EE6C MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;

// Picks a target by phase, then sets velocity scale from its distance.
void func_L14_002F25C8(char *moby)
{
    char *data = *(char **)(moby + 0x78);
    int *t;
    float d;
    float k;
    float x;
    float y;

    switch (*(short *)(data + 0xB4)) {
    case 0:
        *(int **)(data + 0xAC) = D_L14_001B0F30[*(int *)(data + 0x94)];
        *(short *)(data + 0xB6) = 1;
        break;
    case 1:
        *(int **)(data + 0xAC) = D_L14_001B0F30[*(int *)(data + 0x98)];
        *(short *)(data + 0xB6) = 0;
        break;
    case 2:
        *(int **)(data + 0xAC) = D_L14_001B0F30[*(int *)(data + 0x9C)];
        *(short *)(data + 0xB6) = 3;
        break;
    case 3:
        *(int **)(data + 0xAC) = D_L14_001B0F30[*(int *)(data + 0xA0)];
        *(short *)(data + 0xB6) = 0;
        break;
    case 4:
        break;
    }
    t = *(int **)(data + 0xAC);
    d = func_001F9D10((char *)t + 0x10, (char *)t + 0x20);
    d = d * (float)*(int *)*(int **)(data + 0xAC);
    y = D_0015EE70 * 2.5f;
    x = D_0015EE6C * 5.0f;
    k = 1.0f / d;
    *(int *)(data + 0xC4) = 0;
    *(int *)(data + 0xC0) = 0;
    *(float *)(data + 0xD8) = y * k;
    *(float *)(data + 0xD4) = x * k;
}

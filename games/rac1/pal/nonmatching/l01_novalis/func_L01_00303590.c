/* NON_MATCHING func_L01_00303590 -- src/overlays/l01_novalis/vendor_002FABE8.c
 * Best so far: BYTES 8/368 (97.8% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   EnemySpawnerUpdate: state 0 -> 1; state 1 spawns a child (func_L01_00303700) toward the player while count < D
 *   Best is p4.c (8/368 bytes differ): two instructions before the final call, `daddu $a1,$s1` and `mov.s $f12,$f0
 *   Needs a different shape for how the angle reaches the last call (a scheduler tie on argument moves).
 */
extern char D_0013E633[];
extern short D_L01_00161F20;
extern int func_L01_0026EFB8(int, int);
extern char *func_L01_00303700(char *);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern float func_L00_001FF860(float, float);
extern float func_001FA748(float, float);
extern void func_L00_0025D5B0(float ang, char *o, float *s, int a, int b, int c);

/* Spawner update: waits, then spawns one child toward the player while under the limit. */
void func_L01_00303590(char *moby) {
    char *data = *(char **)(moby + 0x78);
    switch (*(unsigned char *)(moby + 0x20)) {
    case 0:
        moby[0x20] = 1;
        break;
    case 1:
        if (*(int *)data >= 0 && func_L01_0026EFB8(*(int *)data, 12) == 0) {
            int n = *(int *)(data + 8);
            if (n < *(int *)&D_L01_00161F20) {
                char *child;
                *(int *)(data + 8) = n + 1;
                child = func_L01_00303700(moby);
                if (child != 0) {
                    char *cd = *(char **)(child + 0x78);
                    float *s = (float *)(cd + 0x60);
                    float k = 0.8f;
                    char *g;
                    int z = 0;
                    *(int *)(cd + 0x84) = 8;
                    *(int *)(cd + 0x80) = func_001FA898_r(*(float *)(cd + 0x250) * k * 1024.0f);
                    *(float *)(cd + 0x88) = *(float *)(cd + 0x250) * k;
                    *(float *)(cd + 0xB0) = 7.0f;
                    *(float *)(cd + 0xB4) = 13.0f;
                    *(int *)(cd + 0x7C) = 0;
                    *(int *)(cd + 0x78) = 0;
                    g = D_0013E633 + 0xE1D;
                    func_L00_0025D5B0(func_001FA748(func_L00_001FF860(*(float *)(g + 0x80) - *(float *)(moby + 0x10), *(float *)(g + 0x84) - *(float *)(moby + 0x14)), 3.14159265f), child, s, 5, 8, z);
                    child[0x20] = 9;
                }
            }
        }
    }
}

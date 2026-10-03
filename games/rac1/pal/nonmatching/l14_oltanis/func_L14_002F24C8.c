/* NON_MATCHING func_L14_002F24C8 -- src/overlays/l14_oltanis/vendor_002E0538.c
 * Best so far: SIZE ours 248 / retail 252, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Finds the first of 5 indices in data+0x80 whose point in table D_L14_001601AC (128-byte entries, +0x30) is wit
 *   p5 (loop via data+0x80+i*4, MACRO_ADDR table, float lim local, tbl local after loop) reproduces the loop shape
 *   Needs a source form that makes `data+0x80` a separate pseudo computed at the loop test; not found in 9 runs.
 */
extern float func_001F9D10(void *, void *);
extern char *D_L14_001601AC MACRO_ADDR;

/* Picks the first of five candidate points within range and copies its vectors. */
void func_L14_002F24C8(char *moby) {
    char *data = *(char **)(moby + 0x78);
    int *ids = (int *)(data + 0x80);
    float lim = 1.0f;
    int i;
    *(short *)(data + 0xB4) = -1;
    for (i = 0; i < 5; i++) {
        if (func_001F9D10(moby + 0x10, D_L14_001601AC + (*(int *)(data + 0x80 + i * 4) << 7) + 0x30) < lim) {
            *(short *)(data + 0xB4) = i;
            break;
        }
    }
    moby[0x20] = 1;
    {
        char *tbl = D_L14_001601AC;
        int idx = ids[*(short *)(data + 0xB4)];
        qcopy(data + 0x60, tbl + (idx << 7) + 0x30);
        qcopy(data + 0x70, tbl + (idx << 7) + 0x70);
    }
}

/* NON_MATCHING func_L14_002F03E8 -- src/overlays/l14_oltanis/vendor_002E0538.c
 * Best so far: SIZE ours 500 / retail 496, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Path moby init: deletes itself if its path table (data+0xA0 / +0x114) is invalid, else sums the segment length
 *   Best p3.c/p4.c: instructions identical to retail, only register numbering differs (retail: path s5, copy/walki
 *   Would unblock: a wording that keeps a separate copy pointer for block 1 (and separate locals for block 2); dec
 */
extern void func_0020D678(void *);
extern float func_001F9D10(void *, void *);
extern char *D_L14_001B0F30[];

/* Measures the segment lengths of two path tables for a path moby, accumulating each total in its data. */
void func_L14_002F03E8(char *moby) {
    char *data = *(char **)(moby + 0x78);
    float *path;
    float *p;
    float *first;
    float *cur;
    float *nxt;
    float *out;
    float d;
    int i;
    if (*(int *)(data + 0xA0) == -1) {
        func_0020D678(moby);
        return;
    }
    path = (float *)D_L14_001B0F30[*(int *)(data + 0xA0)];
    if (*(int *)path == 0 || *(int *)(data + 0x114) == -1 ||
        *(int *)D_L14_001B0F30[*(int *)(data + 0x114)] != 11) {
        func_0020D678(moby);
    } else {
        *(int *)(data + 0xB4) = 0;
        p = path;
        first = p + 4;
        i = 0;
        if (*(int *)p - 1 > 0) {
            nxt = p + 8;
            out = p + 7;
            cur = first;
            do {
                d = func_001F9D10(cur, nxt);
                i++;
                *out = d;
                nxt += 4;
                cur += 4;
                *(float *)(data + 0xB4) += d;
                out += 4;
            } while (i < *(int *)path - 1);
        }
        path[i * 4 + 7] = func_001F9D10(&path[i * 4 + 4], first);
        path = (float *)D_L14_001B0F30[*(int *)(data + 0xB8)];
        *(int *)(data + 0xBC) = 0;
        i = 0;
        if (*(int *)path - 1 > 0) {
            out = path + 7;
            nxt = path + 8;
            cur = path + 4;
            do {
                d = func_001F9D10(cur, nxt);
                i++;
                *out = d;
                nxt += 4;
                cur += 4;
                *(float *)(data + 0xBC) += d;
                out += 4;
            } while (i < *(int *)path - 1);
        }
        path[i * 4 + 7] = func_001F9D10(&path[i * 4 + 4], &path[4]);
        *(float *)(data + 0xAC) = -1.0f;
        *(int *)(data + 0xA8) = 0;
        *(int *)(data + 0xA4) = 0;
    }
}

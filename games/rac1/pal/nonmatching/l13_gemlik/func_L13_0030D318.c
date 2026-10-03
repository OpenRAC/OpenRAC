/* NON_MATCHING func_L13_0030D318 -- src/overlays/l13_gemlik/vendor_0030CAE0.c
 * Best so far: SIZE ours 328 / retail 332, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   UpdateMoby_1577: state 0 copies the class ids of 8 linked mobys (-1 when missing/dead 0xFE/0xFD); state 1 re-c
 *   Best: p8.c (size 324 vs 332). Bodies match (state 1 loop exact with `for (i=0, in=..., out=...; i<8; i++, in++
 *   Left: state 0 loop. Retail eliminates the counter (signed slt against end pointer d+0x50, no dbra); ours turns
 */
extern void func_0020D678(void *);
extern char *D_L13_00160058;

/* Moby update: state 0 records linked mobys' classes, state 1 deletes itself when too few still match. */
void func_L13_0030D318(char *m) {
    char *d = *(char **)(m + 0x78);
    char *in = d + 0x10;
    char *out = d + 0x30;
    int i;
    unsigned char st = m[0x20];
    if (st == 0) {
        char *base = D_L13_00160058;
        m[0x20] = 1;
        for (i = 0; i < 0x20; i += 4) {
            char *e = base + (*(int *)(in + i) << 8);
            if (e == 0) *(int *)(out + i) = -1;
            else if ((unsigned char)e[0x20] == 0xFE) *(int *)(out + i) = -1;
            else if ((unsigned char)e[0x20] == 0xFD) *(int *)(out + i) = -1;
            else *(int *)(out + i) = *(short *)(e + 0xA6);
        }
        if (*(int *)d == 0) m[0x20] = 2;
    } else if (st == 1) {
        int cnt = 0;
        char *base = D_L13_00160058;
        for (i = 0; i < 0x20; i += 4) {
            if (*(int *)(in + i) == -1) {
                *(int *)(out + i) = -1;
            } else if (*(int *)(out + i) == -1) {
                *(int *)(out + i) = -1;
            } else {
                char *e = base + (*(int *)(in + i) << 8);
                if (e == 0) *(int *)(out + i) = -1;
                else if (*(short *)(e + 0xA6) != *(int *)(out + i)) *(int *)(out + i) = -1;
                else if ((unsigned char)e[0x20] == 0xFE) *(int *)(out + i) = -1;
                else if ((unsigned char)e[0x20] == 0xFD) *(int *)(out + i) = -1;
                else cnt++;
            }
        }
        if (cnt < *(int *)d) func_0020D678(m);
    }
}

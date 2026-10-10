/* Walks the 30-entry table at D_0013A5E0+0x2460+0x1E0, starting at the head index (s16 at +0x18E), for the first entry with bits of mask set; t counts the steps (1 up to min(count, cap)). Returns the masked entry or 0; *out gets t-1 when given. */
extern char D_0013A5E0[];
int func_L00_00267BA8(int mask, int cap, int *out) {
    char *r = D_0013A5E0 + 0x2460;
    int v, n, t, h, idx;
    unsigned int e;
    v = *(int *)(r + 0x190);
    n = (v < cap ? v : cap) + 1;
    t = 1;
    if (!(1 < n)) {
        return 0;
    }
    do {
        h = *(short *)(r + 0x18E);
        idx = (h - t + 30) % 30;
        e = *(unsigned int *)(r + 0x1E0 + idx * 4);
        if ((e & mask) != 0) {
            if (out != 0) {
                *out = t - 1;
            }
            return (int)(e & mask);
        }
        t++;
    } while (t < n);
    return 0;
}

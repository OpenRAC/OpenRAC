/* NON_MATCHING func_L18_002D76D8 -- src/overlays/l18_veldin2/vendor_002A8400.c
 * Best so far: BYTES 91/788 (88.5% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   ## Round 1 (16 runs)
 *   Level 18 UpdateMoby_582. State 0: pairs up four nearby entries of the D_L18_0015F7EC table (32-byte entries, w
 *   Best: p10.c, SIZE 780 vs 788. Needed: switch on state, MACRO_ADDR on both table globals, 32-byte entry struct,
 *   Remaining: (1) state 0 inner loop: ours has an extra `move $17,$18` copy of i*8 (retail uses $18 directly for 
 */
extern char D_0013E633[];
extern float func_001F9D10(void *, void *);
extern float func_001F9D48(void *, void *);
extern char *func_L18_002E36B0(void *vector);
extern char *D_L18_0016016C MACRO_ADDR;
typedef struct { char pad[0xC]; float w; int *geom; int pad2[3]; } L18Ent;
extern L18Ent *D_L18_0015F7EC MACRO_ADDR;
extern int D_L18_0015F7F0 MACRO_ADDR;
typedef struct {
    struct { L18Ent *a, *b; } pr[4];
    struct { float a, b; } fl[4];
    int idx[2];
} L18Sel;

void func_L18_002D76D8(unsigned char *moby) {
    L18Sel *s = *(L18Sel **)(moby + 0x78);
    switch (moby[0x20]) {
    case 0: {
        int i;
        int *q;
        int n;
        moby[0x20] = 1;
        moby[0x30] = 0xFF;
        i = 0;
        do {
            int j;
            for (j = 0; j < D_L18_0015F7F0; j++) {
                L18Ent *a = &D_L18_0015F7EC[j];
                int k;
                if (a->w == 0.0f) continue;
                s->pr[i].a = a;
                s->fl[i].a = a->w;
                for (k = 0; k < D_L18_0015F7F0; k++) {
                    L18Ent *b = &D_L18_0015F7EC[k];
                    if (b->w == 0.0f) continue;
                    if (func_001F9D10((char *)D_L18_0015F7EC[j].geom + 0x10, (char *)b->geom + b->geom[0] * 16) < 0.5f) {
                        s->pr[i].b = &D_L18_0015F7EC[k];
                        s->fl[i].b = D_L18_0015F7EC[k].w;
                        i++;
                        D_L18_0015F7EC[k].w = 0.0f;
                        D_L18_0015F7EC[j].w = 0.0f;
                        break;
                    }
                }
            }
        } while (i < 4);
        q = s->idx;
        n = 1;
        do {
            func_L18_002E36B0(D_L18_0016016C + *q++ * 128 + 0x30);
        } while (--n >= 0);
        break;
    }
    case 1: {
        char *base = D_0013E633 + 0xE1D;
        int i;
        *(short *)(base + 0x1CA) = 5;
        for (i = 0; i < 4; i++) {
            if (*(int *)(base + 0x208C) == 15) {
                int v = *(int *)(base + 0x560);
                if (v == s->pr[i].a->geom[4] || v == s->pr[i].b->geom[4]) continue;
            }
            if (func_001F9D48(base + 0x80, (char *)s->pr[i].a->geom + 0x10) < func_001F9D48(base + 0x80, (char *)s->pr[i].b->geom + 0x10)) {
                s->pr[i].a->w = s->fl[i].a;
                s->pr[i].b->w = 0.0f;
            } else {
                s->pr[i].a->w = 0.0f;
                s->pr[i].b->w = s->fl[i].b;
            }
        }
        break;
    }
    }
}

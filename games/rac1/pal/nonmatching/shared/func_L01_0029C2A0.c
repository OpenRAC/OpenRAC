/* NON_MATCHING func_L01_0029C2A0 -- src/overlays/shared/pause_0029C2A0.c
 * Best so far: BYTES 17/288 (94.1% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L01_0029C2A0: find-or-add in a table (int count at +0, 16-byte entries from +4: key, ushort a, short flag
 *   Budget spent at 17/288 bytes (p10.c): only the scheduling of `addiu $a0,$t0,4` (e = t->e) differs; retail has 
 *   Would try: `e = t->e` set in the for-init (`for (i = 0, e = t->e; ...)`) or after the `n` copy. Loop bound mus
 */
extern void func_L00_001FF040(int, void *, int);

typedef struct {
    int key;
    int pad4;
    unsigned short a;
    short flag;
    short c;
    short d;
} Entry;

typedef struct {
    int n;
    Entry e[1];
} Table;

/* Find or add a table entry keyed on the moby offset and parameters; register the new one. */
void func_L01_0029C2A0(char *a0, int a1, char *p, int mode, Table *t) {
    short v20 = *(short *)(p + 0xB2);
    short v19 = *(short *)(p + 0xA6);
    short key;
    int i;
    Entry *e;
    Entry *ne;
    if (mode == 1) {
        key = a0 - p;
    } else {
        key = (int)a0 - *(unsigned short *)(p + 0x78);
    }
    e = t->e;
    for (i = 0; i < t->n; i++, e++) {
        if (e->key == key && e->a == a1 && v20 == e->c && v19 == e->d) {
            mode = e->flag;
            if (mode) {
                func_L00_001FF040((int)&e->pad4, a0, a1);
                return;
            }
        }
    }
    ne = &t->e[t->n];
    t->n = t->n + 1;
    ne->key = key;
    func_L00_001FF040((int)&ne->pad4, a0, a1);
    ne->a = a1;
    ne->flag = mode;
    ne->c = v20;
    ne->d = v19;
}

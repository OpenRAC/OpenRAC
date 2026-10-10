/* NON_MATCHING func_L13_002EE590 -- src/overlays/l13_gemlik/vendor_002EBD00.c
 * Best so far: SIZE ours 824 / retail 848, checked 2026-10-08.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * No longer builds in its file (COMPILE failed, 2026-10-09): match its declarations to the file's first.
 * What the last attempts found:
 *   Level 13 function (848 bytes): a moby walk with a variable-length array (VLA: the frame is extended with subu 
 *   Unblock: the source's local declaration order that gives retail's frame (vectors at 0x00/0x30/0x40/0x50/0x60/0
 */
extern void func_001FA1F8(void *, void *);
extern void func_001F9EC0(void *, void *, void *);
extern void func_001F9BD8(void *, void *, void *);
extern int func_L00_001F2BE8_2FB898(float, void *, int, void *, void *) __asm__("func_L00_001F2BE8");
extern void func_001F9BF0(void *, void *, void *);
extern void func_001FA4A0(void *, void *);
extern void func_001F9EE8(void *, void *, void *);
extern void *func_00115248(void *, void *, int);
extern void func_L00_0025AD38(unsigned char *m, int x, int y, void *p, float f);
extern int D_L13_00178280[];

// Matches the moby list against a table of ids and runs the per-moby update on the hits.
void func_L13_002EE590(char *m, short *p) {
    float s0[12];
    float vb[4];
    float va[4];
    float vbb[4];
    float vc[4];
    float vd[4];
    char *mq;
    float fn;
    float hh;
    float tt;
    int cnt;
    int nn;
    int i;
    int j;
    int next;
    char *obj;
    int *q;

    func_001FA1F8(s0, m + 0x40);
    nn = (p[5] < p[4]) ? p[4] : p[5];
    fn = (float)nn * 0.5f;
    qzero(vb);
    vb[2] = fn;
    mq = m + 0x10;
    func_001F9EC0(vb, vb, s0);
    func_001F9BD8(vb, vb, mq);
    cnt = func_L00_001F2BE8_2FB898(fn, vb, 16, m, 0);
    if (cnt != 0) {
        char *tbl[cnt];
        func_00115248(tbl, D_L13_00178280, cnt * 4);
        for (i = 0; i < cnt; i = next) {
            next = i + 1;
            obj = tbl[i];
            if (obj == 0) {
                continue;
            }
            if (obj[0x20] == 0xFE || obj[0x20] == 0xFD) {
                continue;
            }
            func_001F9BF0(va, obj + 0x10, mq);
            func_001FA4A0(vc, s0);
            func_001F9EE8(vbb, va, vc);
            qcopy(vd, vbb);
            hh = (float)p[4] * 0.5f;
            tt = hh + 0.7f;
            if (vd[0] < -tt) {
                vd[0] = (float)(-p[4]) * 0.5f;
            } else if (tt < vd[0]) {
                vd[0] = hh;
            }
            hh = (float)p[5] * 0.5f;
            tt = hh + 1.5f;
            if (vd[2] < -tt) {
                vd[2] = (float)(-p[5]) * 0.5f;
            } else if (tt < vd[2]) {
                vd[2] = hh;
            }
            vd[1] = 0.0f;
            func_001F9EC0(vd, vd, s0);
            func_001F9BD8(vd, vd, mq);
            j = func_L00_001F2BE8_2FB898(0.7f, vd, 16, m, 0);
            if (j > 0) {
                q = D_L13_00178280;
                do {
                    if (*q == (int)obj) {
                        func_L00_0025AD38((unsigned char *)obj, (int)m, 0x10001, vd, 1.0f);
                    }
                    j--;
                    q++;
                } while (j != 0);
            }
        }
    }
}

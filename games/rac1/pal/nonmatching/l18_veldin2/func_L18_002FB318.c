/* NON_MATCHING func_L18_002FB318 -- src/overlays/l18_veldin2/vendor_002F9D48.c
 * Best so far: SIZE ours 904 / retail 920, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   What mattered:
 *   - qcopy() in the scale loop keeps the forward (slti/bnez) counter; plain struct copy gets the loop reversed.
 *   - k = D_L18_001625BC read into a local before the loop (qcopy has a memory clobber, else reloaded per iteratio
 *   - pkt[2] assigned before pkt[3]; QVec e declared at function scope (stack slot 0xD0), assigned late.
 *   - `pa = &a;` right before the first func_001F9EC0 and used everywhere (loops too) gives retail's early &a comp
 *   Remaining diff: retail keeps q's address (sp+0xE0) in $18 and a's address in $21 as two long-lived pseudos
 *   (ours uses $s3/$s5 and merges); retail passes 2/3 also reload D_L18_001625BC after the color stores and recomp
 *   &q for the 9EE8 loop arg. A pq pointer (p13) compiled to identical bytes as p12.
 */
#include "common.h"
typedef struct {
    float v[4];
} __attribute__((aligned(16))) QVb318;
extern void func_00234C98(int, long);
extern void func_001F9EC0(void *, void *, void *);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9CA0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9EE8(void *, void *, void *);
extern void func_L00_001FD1D8(void *, void *, int);
extern QVb318 D_L18_001DFE90[];
extern float D_L18_001625BC;
extern short D_L18_00162594;
extern short D_L18_001625A0;
extern short D_L18_001625A4;
extern short D_L18_001625A8;
extern short D_L18_001625AC;

void func_L18_002FB318(char *moby) {
    QVb318 m[4];
    int colors[4];
    float uv[4][2];
    unsigned long pkt[4];
    QVb318 a, b, c, d, e;
    QVb318 q[4];
    int i, j;
    QVb318 *pa;
    QVb318 *pq;

    pkt[1] = func_001F4868(11);
    pkt[2] = 0xFF9000000260;
    pkt[3] = 0x8000000048;
    pkt[0] = 0;
    func_00234C98(0x4A, 0);
    func_00234C98(0x47, 0x51001);
    pa = &a;
    func_001F9EC0(&d, &D_L18_001625F0, moby + 0xC0);
    func_001F9BD8(&d, &d, moby + 0x10);
    e = (QVb318){{0.0f}};
    e.v[2] = -1.0f;
    e.v[3] = 1.0f;
    func_001F9EC0(pa, &e, moby + 0xC0);
    func_001F9BF0(&b, &d, D_L18_00167840);
    func_001F9CA0(&c, &b, pa);
    func_L00_001FF4B0(&c, &c, 1.0f);
    func_001F9CA0(&b, pa, &c);
    uv[0][0] = 1.0f; uv[0][1] = 1.0f;
    uv[1][0] = 0.0f; uv[1][1] = 1.0f;
    uv[2][0] = 1.0f; uv[2][1] = 0.0f;
    uv[3][0] = 0.0f; uv[3][1] = 0.0f;
    pq = q;
    colors[0] = colors[1] = colors[2] = colors[3] = *(int *)&D_L18_00162594 | 0xFF000000;
    {
        float z = *(float *)&D_L18_001625A4;
        float k = D_L18_001625BC;
        for (i = 0; i < 4; i++) {
            qcopy(&q[i], &D_L18_001DFE90[i]);
            pq[i].v[2] *= z;
            pq[i].v[0] *= k;
        }
    }
    for (j = 0; j < 4; j++) func_001F9EE8(&m[j], &pq[j], pa);
    func_L00_001FD1D8(m, 0, 0);
    colors[0] = colors[1] = colors[2] = colors[3] = *(int *)&D_L18_001625A0 | 0x7F000000;
    {
        float z = *(float *)&D_L18_001625A8;
        float k = D_L18_001625BC;
        for (i = 0; i < 4; i++) {
            qcopy(&q[i], &D_L18_001DFE90[i]);
            pq[i].v[2] *= z;
            pq[i].v[0] *= k;
        }
    }
    for (j = 0; j < 4; j++) func_001F9EE8(&m[j], &pq[j], pa);
    func_L00_001FD1D8(m, 0, 0);
    colors[0] = colors[1] = colors[2] = colors[3] = *(int *)&D_L18_001625A0 | 0x7F000000;
    {
        float z = *(float *)&D_L18_001625AC;
        float k = D_L18_001625BC;
        for (i = 0; i < 4; i++) {
            qcopy(&q[i], &D_L18_001DFE90[i]);
            pq[i].v[2] *= z;
            pq[i].v[0] *= k;
        }
    }
    for (j = 0; j < 4; j++) func_001F9EE8(&m[j], &pq[j], pa);
    func_L00_001FD1D8(m, 0, 0);
}

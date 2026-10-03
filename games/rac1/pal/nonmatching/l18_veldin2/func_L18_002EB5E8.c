/* NON_MATCHING func_L18_002EB5E8 -- src/overlays/l18_veldin2/vendor_002A8400.c
 * Best so far: BYTES 172/924 (81.4% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Started from p4 (199). p9 (color before uv in j loop): 281. p10 (all fields via p->): SIZE 812. p11 (p->b = i+
 *   Remaining diff in p4: spill-slot numbering of hoisted addresses (0x35C..0x374), a2/a3 and s0/s2/s3 permutation
 *   vtx/color/uv induction pointers, mult register choice. Allocator/scheduler tie; no source rewording moved it. 
 *   ## Round 4 (11 runs of 16)
 *   p16 = p4 with `qcopy(p->vtx[j], ...)` (vtx via p): 172, best. Others: p->uv 844 size; p->color 916 size; i!=0 
 *   b before a order 199/172 (no change); n after call 209; i+1 no n 207; qcopy first in j loop 228.
 *   Remaining diff in p16: retail keeps p (long-store ptr) and pkt[i] vtx ptr as two distinct mults (a3 vs t0), ou
 *   spill slot numbering 0x35C..0x374, and retail's `addiu i+1` scheduled after the first arg load. Allocator tie;
 */
extern void func_001F9BC0(void *);
extern float func_L00_001FF860(float, float);
extern float func_001F9D48(void *, void *);
extern void func_001FA218(float *, float *);
extern void func_001F9C30(void *, void *, float);
extern int func_001F4868(int);
extern int func_001FA8A8(int, int, float);
extern void func_L00_001FD1D8(void *, void *, int);
extern float D_L18_00167700[];
extern float D_L18_001D9E20[][2];
extern float D_L18_001D9E40[][4][4];
extern short D_L18_00161F74;
extern short D_L18_00161F78;
extern short D_L18_00161F7C;
extern short D_L18_00161F80;
extern short D_L18_00161F88;
extern short D_L18_00161F98;

typedef struct {
    float vtx[4][4];
    int color[4];
    float uv[4][2];
    long a;
    long b;
    long c;
    long d;
} Pkt;

void func_L18_002EB5E8(char *moby) {
    Pkt pkt[4];
    float rot[4];
    float m[4][4][4];
    char *data = *(char **)(moby + 0x78);
    int i;
    int j;
    int n;
    Pkt *p;

    func_001F9BC0(rot);
    rot[2] = func_L00_001FF860(D_L18_00167700[0x50] - *(float *)(moby + 0x10), D_L18_00167700[0x51] - *(float *)(moby + 0x14));
    rot[1] = -func_L00_001FF860(func_001F9D48(moby + 0x10, &D_L18_00167700[0x50]), D_L18_00167700[0x52] - *(float *)(moby + 0x18));
    func_001FA218(m[0][0], rot);
    func_001FA218(m[1][0], rot);
    func_001F9C30(m[1][1], m[1][1], *(float *)(data + 0x24));
    func_001F9C30(m[1][2], m[1][2], *(float *)(data + 0x24));
    qcopy(m[0][3], moby + 0x10);
    qcopy(m[1][3], moby + 0x10);
    rot[0] = *(float *)(data + 0x2C);
    func_001FA218(m[2][0], rot);
    func_001F9C30(m[2][1], m[2][1], *(float *)(data + 0x24));
    func_001F9C30(m[2][2], m[2][2], *(float *)(data + 0x24));
    qcopy(m[2][3], moby + 0x10);
    rot[0] = *(float *)(data + 0x30);
    func_001FA218(m[3][0], rot);
    func_001F9C30(m[3][1], m[3][1], *(float *)(data + 0x24));
    func_001F9C30(m[3][2], m[3][2], *(float *)(data + 0x24));
    qcopy(m[3][3], moby + 0x10);

    for (i = 0; i < 4; i++) {
        n = i + 1;
        func_001F4868(((int *)&D_L18_00161F98)[i]);
        p = &pkt[i];
        p->b = n;
        p->c = 0xFF9000000260;
        p->d = (*(int *)&D_L18_00161F74) | (long)(*(int *)&D_L18_00161F78) << 2 | (long)(*(int *)&D_L18_00161F7C) << 4 |
                   (long)(*(int *)&D_L18_00161F80) << 6 | 0x8000L << 24;
        p->a = 0;
        for (j = 0; j < 4; j++) {
            pkt[i].uv[j][0] = D_L18_001D9E20[j][0];
            pkt[i].uv[j][1] = D_L18_001D9E20[j][1];
            if (i == 0) {
                pkt[0].color[j] = func_001FA8A8(*(int *)&D_L18_00161F88 & 0xFFFFFF, *(int *)&D_L18_00161F88, *(float *)(data + 0x24));
            } else {
                pkt[i].color[j] = ((int *)&D_L18_00161F88)[i];
            }
            qcopy(p->vtx[j], D_L18_001D9E40[i][j]);
        }
    }
    func_L00_001FD1D8(&pkt[0], m[0], 0);
    func_L00_001FD1D8(&pkt[1], m[1], 0);
    func_L00_001FD1D8(&pkt[2], m[2], 0);
    func_L00_001FD1D8(&pkt[3], m[3], 0);
}

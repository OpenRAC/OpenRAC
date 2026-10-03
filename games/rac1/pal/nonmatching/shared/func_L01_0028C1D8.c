/* NON_MATCHING func_L01_0028C1D8 -- src/overlays/shared/partupd_00280428.c
 * Best so far: SIZE ours 252 / retail 248, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L01_0028C1D8: resets a path object (clears byte 5 and word 8, loads its point list from table D_L01_001B0
 *   Best candidate p7.c (54/248 bytes, loop shape right: do-while, q carried in a register through `q = p->0x10` i
 *   Remaining difference: retail reloads q (`lw $v1, 0x10($s2)`) before the loop instead of forwarding the stored 
 */
struct Pt { float v[2]; float a; float b; };
struct Path { int n; int pad[3]; struct Pt pt[1]; };
struct Obj { char pad0[5]; char c; char pad1[2]; int z; int pad2; struct Path *path; int idx; };
extern float func_001F9D10(void *, void *);
extern struct Path *D_L01_001B0C30[];

// Resets a path object and computes per-segment lengths of its points.
void func_L01_0028C1D8(struct Obj *p)
{
    int i;
    struct Path *q;
    p->c = 0;
    p->z = 0;
    p->path = D_L01_001B0C30[p->idx];
    for (i = 0; q = p->path, i < q->n; i++) {
        p->path->pt[i].b = func_001F9D10(&q->pt[i], &q->pt[(i + 1) % q->n]);
        p->path->pt[i].a += 0.5f;
        p->path->pt[i].a -= 0.5f;
    }
}

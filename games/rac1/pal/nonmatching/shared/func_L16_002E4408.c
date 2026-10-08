/* NON_MATCHING func_L16_002E4408 -- src/overlays/shared/vendor_002A1B58.c
 * Best so far: SIZE ours 176 / retail 172, checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   - Start staged near; delay point assignment into guarded loop, giving counter its earlier lifetime and target 
 *   1. BYTES27/172: delayed point pointer fixes the guard/initialization stream; counter/point/base S2/S0/S1 still
 *   2. BYTES27 identical: counter advance placement does not change saved register allocation. Use original index-
 *   3. BYTES27: explicit point step, pre-call increment and index-derived point all tie on counter/point/base save
 *   Revisit: full asm read. Earlier pointer/index spellings all used byte views. Describe the path header and 16-b
 *   4. SIZE176/172: typed heading indexing caches base+1C and adds a pointer setup. Preserve typed loop/header but
 *   5. BYTES27/172: byte-stride tail views restore exact size and all tail scheduling; retain p4 as equal typed be
 *   6. BYTES27/172 unchanged: unsigned induction with signed comparison retains the same saved counter/point/base 
 */
#include "common.h"
extern float func_L00_001FF860(float,float);
typedef struct {float x,y,z,heading;} L16HeadingPoint;
typedef struct {int count;char pad04[12];L16HeadingPoint point[1];} L16HeadingPath;
/* Set each path point's heading toward its successor, repeating the last heading. */
void func_L16_002E4408(L16HeadingPath *base) {
    int i=0;
    L16HeadingPoint *point;
    if(base->count-1>0) {
        point=base->point;
        do {
            point->heading=func_L00_001FF860(point[1].x-point->x,point[1].y-point->y);
            i++;point++;
        } while(i<base->count-1);
    }
    if(base->count>=2) {
        int n=base->count;
        base->point[n-1].heading=base->point[n-2].heading;
    }
}

/* NON_MATCHING func_L16_002E9960 -- src/overlays/l16_kalebo3/vendor_002E7C70.c
 * Best so far: BYTES 12/1000 (98.8% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   13. BYTES12: guarded outer do loop fixes all branch initialization scheduling. Divisor120 initializes after360
 *   14. BYTES12 unchanged: divisor assignment hoists identically. Use the period directly in the remainder rather 
 *   15. BYTES12 unchanged: direct modulo120 has the same divisor hoist. Explicitly cache the angular span beside t
 *   16. BYTES16: explicit span changes the initialization order, confirming a loop-invariant ordering issue. Initi
 *   17. BYTES22: explicit angular span still precedes induction zero, and early index reset swaps winding instruct
 *   18. BYTES30: reusing vertex index perturbs otherwise exact integer allocation; keep separate j. Try the diviso
 *   19. BYTES18: period-before-sign rotates their saved registers; preserve p12 BYTES12. Remove unused trial local
 *   20. BYTES12: clean p19 preserves best. Budget spent. Only +218..+220: divisor120 LI comes after the two angula
 */
#include "common.h"
extern float func_001F9B88(float);
extern int func_001FA8A8(int,int,float);
extern void func_001FA460(void *,void *);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern float func_001FA7D8(float);
extern void func_L00_001FD1D8(void *,void *,int);
extern float D_0015EE6C MACRO_ADDR;
extern char *D_L16_001601BC MACRO_ADDR;
extern float D_L16_001D9B50[][2];
extern short D_L16_00161F78,D_L16_00161F9C,D_L16_00161FA0;
extern short D_L16_00161F90,D_L16_00161F8C,D_L16_00161F94,D_L16_00161F98;
extern short D_L16_00161F88,D_L16_00161F84,D_L16_00161FA8;
typedef float L16RingVector[4] __attribute__((aligned(16)));
typedef struct {
    L16RingVector position[4];
    int color[4];
    struct { float u,v; } uv[4];
    long zero,texture,flags,mode;
} L16RingPacket;
typedef struct { L16RingVector basis[3]; L16RingVector position; } L16RingMatrix;
/* Draw stacked alternating rings with a rotating transformation and interpolated tint. */
void func_L16_002E9960(char *m) {
    L16RingPacket packet;
    L16RingMatrix matrix;
    char *d=*(char **)(m+0x78);
    int bound_index=(*(int *)((char *)d+0xA0))<<7;
    char *entry=(char *)(bound_index+(int)D_L16_001601BC);
    float translation=(*(float *)((char *)d+0xB8));
    float lower=(*(float *)((char *)entry+0x38));
    float half=(*(float *)((char *)entry+0x28));
    float upper=lower+half+translation;
    int color;
    float (*uv)[2];
    L16RingVector *point;
    float *v,*u;
    L16RingMatrix *transform;
    float radius,index,phase;
    int i,j,sign,period;
    lower=lower-half;
    lower+=translation;
    color=func_001FA8A8((*(int *)&D_L16_00161F9C),(*(int *)&D_L16_00161FA0),func_001F9B88((*(float *)((char *)d+0xB0)))/((*(float *)&D_L16_00161F78)*D_0015EE6C));
    func_001FA460(&matrix,m+0xC0);
    qcopy(matrix.position,m+16);
    matrix.position[2]=lower; matrix.position[3]=1.0f;
    packet.texture=func_001F4868(14);
    packet.flags=0xFF9000000260L;
    packet.mode=(long)(*(int *)&D_L16_00161F8C)|((long)(*(int *)&D_L16_00161F90)<<2)|((long)(*(int *)&D_L16_00161F94)<<4)|((long)(*(int *)&D_L16_00161F98)<<6)|0x8000000000L;
    packet.zero=0;
    uv=D_L16_001D9B50;point=packet.position;v=&packet.uv[0].v;u=&packet.uv[0].u;
    for(i=0;i<4;i++) {
        *u=(*uv)[0]; *v=(*uv)[1];
        if(i&1) { (*point)[2]=(*(float *)&D_L16_00161F88); radius=(*(float *)&D_L16_00161F84); }
        else { (*point)[2]=-(*(float *)&D_L16_00161F88); radius=(*(float *)&D_L16_00161F84)-0.05f; }
        index=(float)(i>>1);
        (*point)[0]=func_001F9F90(index*((*(float *)&D_L16_00161FA8)*0.017453292f))*radius;
        (*point)[1]=func_001F9FA8(index*((*(float *)&D_L16_00161FA8)*0.017453292f))*radius;
        (*point)[3]=1.0f;
        v+=2;u+=2;uv++;point++;
    }
    transform=&matrix;
    if(matrix.position[2]<upper) {
        sign=1;period=120;
        do {
        phase=(float)(D_L16_0015F6B0%period)*0.02617991715669632f;
        sign=-sign;
        for(j=0;(float)j<360.0f/(*(float *)&D_L16_00161FA8);j++) {
            matrix.basis[0][0]=func_001F9F90(func_001FA7D8(((*(float *)&D_L16_00161FA8)*0.017453292f)*(float)j+phase*(float)sign));
            matrix.basis[0][1]=func_001F9FA8(func_001FA7D8(((*(float *)&D_L16_00161FA8)*0.017453292f)*(float)j+phase*(float)sign));
            matrix.basis[0][2]=0.0f;
            matrix.basis[1][0]=func_001F9F90(func_001FA7D8(((*(float *)&D_L16_00161FA8)*0.017453292f)*(float)j+1.5707964f+phase*(float)sign));
            matrix.basis[1][1]=func_001F9FA8(func_001FA7D8(((*(float *)&D_L16_00161FA8)*0.017453292f)*(float)j+1.5707964f+phase*(float)sign));
            matrix.basis[1][2]=0.0f; matrix.basis[2][2]=1.0f;
            packet.color[3]=color;packet.color[2]=color;packet.color[1]=color;packet.color[0]=color;
            func_L00_001FD1D8(&packet,transform,0);
        }
        matrix.position[2]+=(*(float *)((char *)d+0xBC));
        } while(matrix.position[2]<upper);
    }
}

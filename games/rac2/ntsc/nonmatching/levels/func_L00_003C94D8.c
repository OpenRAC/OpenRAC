/* Tier: GPT-6.1 Sol, high effort (v1019), 29 October 2026.
 * Pinned game -O2 -G8 -ffunction-sections: 1180/1180 bytes, 0/27 places.
 * First difference +0x114: lifetime constant precedes the stack-address spill.
 * Explicit first_life = 2.0f before the first loop recovers all three retail
 * FP constant registers (f23/f21/f20); no volatile casts are used.
 * Particle/tail registers remain s0/s1 rather than s1/s0, with different store
 * and conditional-rotation scheduling. Second-loop resource address is not
 * hoisted, and outgoing emitter argument moves differ.
 * Separate counters plus an explicit resource pointer give 1188 bytes; that
 * variant and truthful FP-first rotation/scale interfaces produce identical
 * bytes. Stop on that tie. A new pointer-lifetime hypothesis is needed.
 * qzero_nc recovers the independent second-loop zero register setup.
 * Required declarations follow; no linked build was run for this near miss.
 */
#include "common.h"
typedef int BurstQuad __attribute__((mode(TI)));
typedef struct { float x,y,z,w; } BurstVec __attribute__((aligned(16)));
extern char D_0018A550[] NOT_SDA;
typedef struct { char pad[0x110]; BurstVec velocity; char gap[0x220]; int active; } BurstResident;
extern BurstResident D_00189E20 NOT_SDA;
extern char D_L00_001FE940[] NOT_SDA;
extern float D_L00_001B00FC __attribute__((sda));
extern float D_L00_001B0100 __attribute__((sda));
extern int D_L00_001B0104 __attribute__((sda));
extern int D_L00_001B010C __attribute__((sda));
extern float D_L00_001B0110 __attribute__((sda));
extern float D_L00_001B0114 __attribute__((sda));
extern float D_L00_001B0118 __attribute__((sda));
extern float D_L00_001B011C __attribute__((sda));
extern float D_L00_001B0120 __attribute__((sda));
extern float D_L00_001B0124 __attribute__((sda));
extern void func_00283098(void *,void *,void *);
extern void func_00282CC0(void *,void *,void *);
extern void func_002ACEC0(void *,void *,void *,float);
extern float func_002A7878(float,float);
extern float func_00283B98(float,float);
extern void func_00282D30(void *,void *,float);
typedef struct { int kind, color, pad08, color2; BurstVec position, previous, velocity; } BurstParticle;
extern BurstParticle *func_L00_00333F90(void *,void *);
extern float func_00282C48(float);
extern void func_002AB668(void *,void *,float);
extern void func_002ACDE8(void *,void *,void *);
extern float func_002A78D8(float,float);
extern int func_00283CF0(float);
extern void func_00282CF0(void *,void *,void *);
extern char *func_002C16C0(void *,void *,int,int,int,int,float,float,float,float,float);
void func_L00_003C94D8(char *object)
{
    BurstVec matrix, start, saved, position, velocity, local, temp, offset;
    float angle, speed, life;
    float first_life;
    int i, kind, mode;

    *(BurstQuad *)&local = 0;
    *(BurstQuad *)&saved = 0;
    saved.x=0.3f; saved.y=-1.0f; saved.w=1.0f;
    local.x=0.513f; local.y=-1.0f; local.z=0.183f; local.w=1.0f;
    func_00283098(&local,&local,object+0xC0);
    func_00282CC0(&position,object+0x10,&local);
    angle=0.0f;
    object[0xBC]=6;
    func_002ACEC0(&saved,D_0018A550,D_0018A550+0x10,0.3490659f);
    qcopy(&start,&saved);
    first_life = 2.0f;
    for(i=9;i>=0;i--) {
        BurstParticle *p;
        angle=func_00283B98(angle,func_002A7878(0.0f,0.6283186f));
        speed=func_002A7878(0.10000001f,0.3f);
        func_00282D30(&velocity,&start,speed);
        p=func_L00_00333F90(&position,&velocity);
        if(p) {
            BurstVec *tail;
            func_00282D30(&velocity,&start,0.75f);
            tail = &p->velocity;
            func_00282CC0(&p->position,&position,&velocity);
            qcopy_nc(&p->previous,&position);
            tail->w=first_life;
            p->color2=0x307F7F7F;
            p->position.w=0.05f;
            p->previous.w=0.5f;
            p->color=0x307F7F7F;
        }
        if(func_00282C48(angle)>0.00001f) {
            func_002AB668(&matrix,D_0018A550,angle);
            func_002ACDE8(&start,&saved,&matrix);
        }
    }
    func_002ACEC0(&saved,D_0018A550,D_0018A550+0x10,D_L00_001B00FC*0.017453294f);
    angle=0.0f;
    qcopy(&start,&saved);
    qzero_nc(&offset);
    if(D_00189E20.active) func_00282D30(&offset,&D_00189E20.velocity,1.7f);
    for(i=9;i>=0;i--) {
        char *p;
        angle=func_00283B98(angle,func_002A7878(0.0f,0.6283186f));
        speed=func_002A7878(D_L00_001B0114*0.016666668f,D_L00_001B0118*0.016666668f);
        func_00282D30(&velocity,&start,speed);
        func_00282CC0(&velocity,&velocity,&offset);
        kind=D_L00_001B010C;
        mode=func_00283CF0(func_002A78D8(1.0f,2.0f));
        temp.x=position.x+func_002A78D8(0.0f,D_L00_001B0100);
        temp.y=position.y+func_002A78D8(0.0f,D_L00_001B0100);
        temp.z=position.z+func_002A78D8(0.0f,D_L00_001B0100);
        life=func_002A7878(D_L00_001B011C,D_L00_001B0120)*210000.0f;
        func_00282CF0(&temp,&temp,&velocity);
        p=func_002C16C0(&temp,&velocity,kind,D_L00_001B0104,0x505050,mode,life,D_L00_001B0124,D_L00_001B0110,0.0f,0.0f);
        if(p) p[2]=(*(char **)(D_L00_001FE940+0x16C))[1];
        if(func_00282C48(angle)>0.00001f) {
            func_002AB668(&matrix,D_0018A550,angle);
            func_002ACDE8(&start,&saved,&matrix);
        }
    }
}

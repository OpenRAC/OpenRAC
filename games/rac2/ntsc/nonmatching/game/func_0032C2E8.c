/* GPT-6.1 Sol high tier, v0807: 920/928 bytes, 0/28 USA placements.
 * Profile: -O2 -G8 -Wa,-G8 -ffunction-sections -g.
 * Separate corner-indexed projection outputs and walked UV pointers reproduce
 * retail's four UV pointers and spill the state pointer. The 0xD0 packet and
 * 0x170 frame remain intact. The camera vectors use their shared context.
 * Remaining: tag-expression scheduling, camera-base reuse, counter lifetimes,
 * UV pointer allocation and loop scheduling. Sequential tag accumulation
 * restores the OR chain but does not match its load/register order. Counting
 * downward gave 904 bytes; -fno-gcse gave 900 and a wrong 0x160 frame;
 * -fno-rerun-loop-opt and a do/while did not fix the loop. No linked credit.
 * Next useful evidence: original packet/context types and loop lifetimes.
 */
#include "include_asm.h"
typedef struct { float x,y,z,w; } RenderVec __attribute__((aligned(16)));
typedef struct { RenderVec vertex[4]; unsigned int color[4]; float uv[8]; long long tag[4]; RenderVec matrix[4]; } RenderPacket;
extern void func_00282CF0(void *,const void *,const void *);
extern void func_00282F20(void *,const void *,float);
extern void func_00282DD8(void *,const void *,const void *);
extern void func_00282D30(void *,const void *,float);
extern void func_002830C0(void *,const void *,const void *);
extern void func_0027C768(void *,float *,float *);
extern float func_00283180(float),func_00283B98(float,float);
extern void func_00280B90(void *,void *,int);
extern void func_002FE8D8(void *,void *);
extern RenderVec D_001B5340;
extern char D_00189E20[];
extern long long D_001B2810;
extern int D_001B059C __attribute__((sda));
extern int D_001B05A0 __attribute__((sda));
extern int D_001B05A4 __attribute__((sda));
extern int D_001B05A8 __attribute__((sda));
extern int D_001B05AC __attribute__((sda));
extern unsigned int D_001B05B0 __attribute__((sda));
extern short D_0026B210[][2];
extern RenderVec D_0026AA00[];
extern float D_00235D00[][2] __attribute__((sda));
void func_0032C2E8(char *object)
{
    RenderPacket packet;
    RenderVec up,difference,*vertex;
    char *state;
    int face,corner,index;
    short (*indices)[2];
    float *u,*v,*phase;
    packet.tag[3]=(long long)D_001B059C;
    packet.tag[3]|=(long long)D_001B05A0<<2;
    packet.tag[3]|=(long long)D_001B05A4<<4;
    packet.tag[3]|=(long long)D_001B05A8<<6;
    packet.tag[3] |= (long long)D_001B05AC<<32;
    state=*(char **)(object+0x68);
    packet.tag[1]=D_001B2810;
    packet.tag[2]=0xFF9000000260LL;
    packet.tag[0]=0;
    func_002FE8D8((RenderVec *)(D_00189E20+0xE0),&up);
    func_00282CF0(&difference,&D_001B5340,(RenderVec *)(D_00189E20+0xE0));
    func_00282F20(&packet.matrix[0],&difference,1.0f);
    func_00282DD8(&packet.matrix[1],&packet.matrix[0],&up);
    func_00282F20(&packet.matrix[1],&packet.matrix[1],1.0f);
    func_00282DD8(&packet.matrix[2],&packet.matrix[1],&packet.matrix[0]);
    func_00282F20(&difference,&difference,2.0f);
    packet.matrix[3].x=(*(RenderVec *)(D_00189E20+0xE0)).x+difference.x;
    packet.matrix[3].y=(*(RenderVec *)(D_00189E20+0xE0)).y+difference.y;
    packet.matrix[3].z=(*(RenderVec *)(D_00189E20+0xE0)).z+difference.z;
    packet.matrix[3].w=1.0f;
    packet.color[0]=packet.color[1]=packet.color[2]=packet.color[3]=D_001B05B0;
    for(face=0;face<127;face++) {
        indices=D_0026B210+face*4;
        u=packet.uv;v=packet.uv+1;vertex=packet.vertex;
        for(corner=0;corner<4;corner++) {
            index=indices[0][1];
            func_00282D30(vertex,&D_0026AA00[indices[0][0]],*(float *)(state+0x14)*4.0f);
            func_002830C0(vertex,vertex,packet.matrix);
            func_0027C768(vertex,&packet.uv[corner*2],&packet.uv[corner*2+1]);
            *u+=func_00283180(D_00235D00[index][0])*0.01f;
            *v+=func_00283180(D_00235D00[index][1])*0.02f;
            if (*u>1.0f) *u=1.0f; else if (*u<0.0f) *u=0.0f;
            if (*v>1.0f) *v=1.0f; else if (*v<0.0f) *v=0.0f;
            u+=2;v+=2;vertex++;indices++;
        }
        func_00280B90(&packet,0,0);
    }
    phase=D_00235D00[0];
    for(face=136;face>=0;face--) {
        phase[0]=func_00283B98(phase[0],0.6283186078071594f);
        phase[1]=func_00283B98(phase[1],0.3141593039035797f);
        phase+=2;
    }
}

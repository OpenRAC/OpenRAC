/* sol-high retained attempt, USA v1.01, pinned game compiler, default -G8.
 * check.py: 1352 bytes versus 1352; 0 of 5 places match.
 * Remaining: 0x380 frame versus 0x360, two extra UV-pointer sq/lq spills,
 * fixed-row induction, allocation and scheduling. Equal size is not a match.
 * Recovered: signed packet-pointer loop, sequential GS ALPHA packing, shared
 * shrink-radius local, ordinary TI rotation clear, and explicit SDATA array
 * address views. Distinct vertex-call/radius pointers produce retail's two
 * multiplies and its caller-saved inner-loop counter spill.
 * Flattening fixed vertices reduces size to 1296 but loses the switch shape;
 * flattening the vertex member grows to 1436; named-row union gives 1360.
 * Next: recover the fixed-row IV sharing without the extra live pointers.
 * All declarations are below. No linked-build claim.
 * Related RAC1 source: GPL-3.0 gold_bolts_and_teleporters.c,
 * draw_moby_radius_ring; RAC2 constructs four quads for a batched draw.
 */
#include "common.h"
extern char D_00189EA0[];
extern void func_00282CF0(void *, void *, void *);
#define I(p,o) (*(int *)((char *)(p)+(o)))
#define F(p,o) (*(float *)((char *)(p)+(o)))
#define B(p,o) (*(unsigned char *)((char *)(p)+(o)))
typedef int TI __attribute__((mode(TI)));
typedef struct {float v[4][4]; unsigned c[4]; float uv[4][2]; long g[4];} RingQuad;
extern int D_L01_001AF5D8 __attribute__((sda));
extern int D_L01_001AF5DC __attribute__((sda));
extern int D_L01_001AF5E0 __attribute__((sda));
extern int D_L01_001AF5E4 __attribute__((sda));
extern int D_L01_001AF5E8 __attribute__((sda));
extern int D_L01_001AF5F0[] SDATA(D_L01_001AF5F0);
extern float D_L01_001AF5F8[] SDATA(D_L01_001AF5F8);
extern float D_L01_001AF618 __attribute__((sda));
extern float D_L01_001AF620 __attribute__((sda));
extern float D_L01_001AF5FC[] NOT_SDA;
extern char D_00189F20[];
extern char D_00189E20[];
extern float D_L01_001B1FA4;
extern void func_00283658(void *,void *);
extern long func_0027C878(int);
extern void func_002AC9F8(void *,float);
extern unsigned func_00283D38(unsigned,unsigned,float);
extern void func_00282C90(void *);
extern void func_002833F0(void *,void *);
extern void func_L01_002E1510(void *,int,void *,int,void *);
void func_L01_00398B50(char *m)
{
    RingQuad q[4];
    RingQuad *end=q+4;
    float mat[12];
    float pos[4];
    float transform[12];
    float rot[4];
    char *d=*(char **)(m+0x68);
    float alpha,h;
    int j,k;
    RingQuad *quad;
    func_00283658(mat,m+0xc0);
    qcopy(pos,m+0x10);
    quad=q;
    do {
        quad->g[1]=func_0027C878(D_L01_001AF5E8);
        quad->g[3]=(long)D_L01_001AF5D8;
        quad->g[3]|=(long)D_L01_001AF5DC<<2;
        quad->g[3]|=(long)D_L01_001AF5E0<<4;
        quad->g[3]|=(long)D_L01_001AF5E4<<6;
        quad->g[3]|=0x8000L<<24;
        quad->g[2]=0xFF9000000260L;quad->g[0]=0;
        quad++;
    } while ((int)quad < (int)end);
    h=0.1f;
    alpha=(1.95f-F(d,0x34))*2.0f;
    if(alpha>1) alpha=1; else if(alpha<0) alpha=0;
    if(F(d,0x34)<1.15f) {
        float t, shrink;
        char *g=D_00189E20;
        func_00282CF0(D_00189F20,m+0x10,D_00189F20-0x80);
        I(g,0x108)=0;
        func_002AC9F8(D_00189F20,0.01666666753590107f);
        shrink=F(d,0x34);
        t=1.15f-shrink; if(t<0)t=0;
        {float fade=(0.2f-shrink)*5; if(fade<0)fade=0; D_L01_001B1FA4=fade;}
        h=t+0.1f;
        if(B(m,0x20)!=7 && B(m,0x20)!=8) {
            if(shrink<0.65f) {
                float v=shrink*1.5384615659713745f;
                *(short *)(g+0x2500)=1;
                F(g,0x2024)=v;F(g,0x2020)=v;F(g,0x2028)=2-v;
            } else *(short *)(g+0x2500)=0;
        }
    }
    for(j=0;j<4;) {
        int next=j+1;
        float *p=q[j].v[0];
        unsigned *color=q[j].c;
        float *uDest=&q[j].uv[0][0];
        float *vDest=&q[j].uv[0][1];
        float *u=D_L01_001AF5F8;
        float *v=D_L01_001AF5FC;
        for(k=0;k<4;k++) {
            unsigned c=D_L01_001AF5F0[j!=0];
            *uDest=*u;
            *vDest=*v;
            *color=c;
            *color=func_00283D38(c&0xffffff,c,alpha);
            func_00282C90(q[j].v[k]);
            {float radius=F(d,0x34); if(j)radius+=0.01f; p[0]=radius;}
            if(k&1) {
                switch(j) {
                case 0:q[0].v[k][2]+=D_L01_001AF618+h;break;
                case 1:q[1].v[k][2]+=D_L01_001AF618+D_L01_001AF620;break;
                case 2:q[2].v[k][2]+=(D_L01_001AF618-h*0.8f)+D_L01_001AF620;break;
                case 3:q[3].v[k][2]+=(D_L01_001AF618+h*0.8f)+D_L01_001AF620;break;
                }
            } else {
                switch(j) {
                case 0:q[0].v[k][2]+=D_L01_001AF618-h;break;
                case 1:q[1].v[k][2]+=D_L01_001AF618-D_L01_001AF620;break;
                case 2:q[2].v[k][2]+=(D_L01_001AF618-h*0.8f)-D_L01_001AF620;break;
                case 3:q[3].v[k][2]+=(D_L01_001AF618+h*0.8f)-D_L01_001AF620;break;
                }
            }
            p+=4; color++; uDest+=2; vDest+=2; u+=2; v+=2;
        }
        j=next;
    }
    *(TI *)rot=0;rot[2]=0.19634954631328583f;
    func_002833F0(transform,rot);
    func_L01_002E1510(q,4,transform,32,mat);
}

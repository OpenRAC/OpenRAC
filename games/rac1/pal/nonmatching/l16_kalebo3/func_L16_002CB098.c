/* NON_MATCHING func_L16_002CB098 -- src/overlays/l16_kalebo3/vendor_002A50F0.c
 * Best so far: SIZE ours 992 / retail 984, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * Cannot land as written (#define in a candidate): rewrite that in plain C first.
 * What the last attempts found:
 *   1. COMPILE: pose/global declarations exist later in source, so candidate needs declarations before use. Add ex
 *   2. SIZE992: packet constants initialized at function entry, increasing lifetimes/register costs; retail builds
 *   3. SIZE996: named packet pair adds FP save because shared i is hoisted into early float call and remains live 
 *   4. SIZE996: separate counters fix all index/count lifetimes and data S3; sole extra saved register FP is cache
 *   5. BYTES114/984: derived secondary point address removes FP cache and matches whole frame, all phase/header/dr
 *   6. SIZE996: secondary walker derived once is folded to same cached FP packet address. Test typed 128-bit secon
 *   7. SIZE996: typed TI secondary copy still caches packet2 in FP and also schedules its store past the index bra
 *   8. SIZE996: separate packet objects produce identical extra FP cache/save and setup differences as p5. STOP af
 */
#include "common.h"
extern char *D_L16_001601AC_m __asm__("D_L16_001601AC") MACRO_ADDR;
extern int D_L16_0015F6B0 MACRO_ADDR;
extern float func_001F9CB8(void *);
extern float func_001FA888(int);
extern float func_001F9FA8(float);
extern int func_001F4868(int);
extern void func_L00_001FD1D8(void *,void *,int);
extern float D_L16_001D32C0[][2],D_L16_001D32E0[][2],D_L16_001D3300[][4];
extern short D_L16_00161A68,D_L16_00161A6C,D_L16_00161A50,D_L16_00161A70;
extern short D_L16_00161A58,D_L16_00161A5C,D_L16_00161A60,D_L16_00161A64,D_L16_00161A54;
#define GP(a) (*(int *)&(a))
#define F(p,o) (*(float *)((char *)(p)+(o)))
#define W(p,o) (*(int *)((char *)(p)+(o)))
typedef struct {
    float point[4][4]; int color[4]; struct {float u,v;} uv[4];
    long zero,texture,flags,mode;
} L16RibbonPacket;
/* Draw paired textured strips along an indexed pose, fading the end caps. */
void func_L16_002CB098(char *m) {
    L16RibbonPacket packets[2];
    char *d=*(char **)(m+0x78);
    float scale=func_001F9CB8(D_L16_001601AC_m+(W(d,0x60)<<7))*2.001f;
    int count=func_001FA898_caa18(scale);
    float step=2.0f/scale;
    int period,tint,color,limit,i,j;
    float phase;
    long modebits=0x8000000000L,flags=0xFF9000000260L;
    F(d,0x70)=F(d,0x70)+F(d,0x6C);
    if(F(d,0x70)>1.0f) F(d,0x70)-=1.0f;
    else if(F(d,0x70)<0.0f) F(d,0x70)+=1.0f;
    period=func_001F9850(120);
    limit=count+2;
    phase=func_001FA888(D_L16_0015F6B0%period);
    phase=phase/func_001FA888(period);
    phase=func_001F9FA8(phase*6.28318f-3.14159f)*0.5f+0.5f;
    tint=func_001FA8A8_caa18(GP(D_L16_00161A68),GP(D_L16_00161A6C),phase);
    color=GP(D_L16_00161A70);
    packets[0].texture=func_001F4868(GP(D_L16_00161A50));
    packets[0].mode=(long)GP(D_L16_00161A58)|((long)GP(D_L16_00161A5C)<<2)|((long)GP(D_L16_00161A60)<<4)|((long)GP(D_L16_00161A64)<<6)|modebits;
    packets[0].flags=flags; packets[0].zero=0;
    packets[1].texture=func_001F4868(GP(D_L16_00161A54));
    packets[1].mode=(long)GP(D_L16_00161A58)|((long)GP(D_L16_00161A5C)<<2)|((long)GP(D_L16_00161A60)<<4)|((long)GP(D_L16_00161A64)<<6)|modebits;
    packets[1].flags=flags; packets[1].zero=0;
    { float *u0=&packets[0].uv[0].u,*v0=&packets[0].uv[0].v;
      float *u1=&packets[1].uv[0].u,*v1=&packets[1].uv[0].v;
      int *c0=packets[0].color,*c1=packets[1].color;
      float *p0=packets[0].point[0],*p1=packets[1].point[0];
      float (*uv0)[2]=D_L16_001D32C0,(*uv1)[2]=D_L16_001D32E0,(*source)[4]=D_L16_001D3300;
      for(i=0;i<4;i++) {
          float a=(*uv0)[0],b=(*uv0)[1];
          *u0=a; *v0=b;
          a=(*uv1)[0]-F(d,0x70); b=(*uv1)[1];
          *u1=a; *v1=b; *c0=tint; *c1=color;
          qcopy(p0,source);qcopy(p1,source);
          if(i<2) { *p0-=step; F(p0,0x90)-=step; }
          p1+=4;source++;p0+=4;c1++;c0++;v1+=2;u1+=2;uv1++;v0+=2;u0+=2;uv0++;
      }
    }
    for(i=0;i<limit;i++) {
        if(i==0) {packets[0].color[1]=0;packets[0].color[0]=0;packets[1].color[1]=0;packets[1].color[0]=0;}
        else if(i==count+1) {packets[0].color[3]=0;packets[0].color[2]=0;packets[1].color[3]=0;packets[1].color[2]=0;}
        else if(i==1) {packets[0].color[1]=tint;packets[0].color[0]=tint;packets[1].color[1]=color;packets[1].color[0]=color;}
        func_L00_001FD1D8(&packets[0],D_L16_001601AC_m+(W(d,0x60)<<7),0);
        func_L00_001FD1D8(&packets[1],D_L16_001601AC_m+(W(d,0x60)<<7),0);
        {float *point=packets[0].point[0];
        for(j=3;j>=0;j--) {float a=*point+step,b=F(point,0x90)+step;*point=a;F(point,0x90)=b;point+=4;}
        }
    }
}

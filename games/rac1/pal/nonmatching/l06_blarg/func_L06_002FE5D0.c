/* NON_MATCHING func_L06_002FE5D0 -- src/overlays/l06_blarg/vendor_002FE5D0.c
 * Best so far: SIZE ours 692 / retail 700, checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   mini12/a03: three-form stop p3 captured texture/position indices, p5 float-first scale alias, p6 scalar color 
 *   Draws fifty transformed textured quad pairs. Remaining differences are header constant/texture result setup, p
 *   Use unsigned int mode(DI) for 64-bit packet fields: SN longlong causes unsupported wide integer operation. Def
 */
#include "common.h"
typedef int BlargQuad __attribute__((mode(TI)));
typedef unsigned int Blarg64 __attribute__((mode(DI)));
typedef struct { BlargQuad pos[4]; int color[4]; float uv[8]; Blarg64 header[4]; } BlargBatch;
extern int func_001F4868(int);
extern void func_001FA190(void *);
extern void func_001F9C30_f(float,void *,void *) __asm__("func_001F9C30");
extern void func_001F9EE8(void *,void *,void *);
extern void func_001F9BF0(void *,void *,void *);
extern void func_L00_001FF4B0(void *,void *,float);
extern void func_001F9EC0(void *,void *,void *);
extern float func_001F9C78(void *,void *);
extern void func_L00_001FD1D8(void *,void *,int);
extern char D_L06_00167500[];
extern char D_L06_00167640[];
extern BlargQuad D_L06_001F0C50[];
extern float D_L06_001F1890[][2];
extern float D_L06_001F2960[][4];
extern float D_L06_001F1D20[][4];
extern short D_L06_001F1570[][8];
extern short D_L06_00162030,D_L06_00162034,D_L06_00162020,D_L06_00162010,D_L06_0016200C,D_L06_00162014,D_L06_00162018,D_L06_0016201C,D_L06_00162024,D_L06_00162028,D_L06_0016202C;
/* Draws fifty transformed pairs of textured quads with camera-scaled UV offsets. */
void func_L06_002FE5D0(void) {
 struct { BlargBatch front,back; float matrix[16],dir[4],normal[4]; } w;
 float x=*(float *)&D_L06_00162030**(float *)(D_L06_00167500+0x158);
 float y=*(float *)&D_L06_00162034**(float *)(D_L06_00167500+0x154);
 Blarg64 bits;
 int i=0;
 w.front.header[1]=func_001F4868(*(int *)&D_L06_00162020);
 w.back.header[1]=func_001F4868(42);
 bits=(Blarg64)*(int *)&D_L06_0016200C | ((Blarg64)*(int *)&D_L06_00162010<<2) | ((Blarg64)*(int *)&D_L06_00162014<<4) | ((Blarg64)*(int *)&D_L06_00162018<<6) | ((Blarg64)*(int *)&D_L06_0016201C<<32);
 w.front.header[3]=bits;w.back.header[3]=bits;
 w.front.header[2]=0xFF9000000260ULL;w.back.header[2]=0xFF9000000260ULL;
 w.back.header[0]=0;w.front.header[0]=0;
 func_001FA190(w.matrix);
 { int c=*(int *)&D_L06_00162024,d=*(int *)&D_L06_00162028;
 w.front.color[0]=c;w.front.color[3]=c;w.front.color[2]=c;w.front.color[1]=c;
 w.back.color[0]=d;w.back.color[3]=d;w.back.color[2]=d;w.back.color[1]=d; }
 do {
  func_001F9C30_f(*(float *)&D_L06_0016202C,w.normal,D_L06_001F2960[i]);
  func_001F9EE8(w.normal,w.normal,w.matrix);
  func_001F9BF0(w.dir,w.normal,D_L06_00167640);
  func_L00_001FF4B0(w.dir,w.dir,1.0f);
  func_001F9EC0(w.normal,D_L06_001F1D20[i],w.matrix);
  func_001F9C78(w.dir,w.normal);
  {
  short *indices=D_L06_001F1570[i];
  BlargQuad *pf=w.front.pos,*pb=w.back.pos;
  float *ux=w.front.uv,*uy=w.front.uv+1,*vx=w.back.uv,*vy=w.back.uv+1;
  int j;
  for(j=3;j>=0;j--) {
   int position=indices[0],texture=indices[1];
   float u,v;
   qcopy(pf,&D_L06_001F0C50[position]);qcopy(pb,pf);
   u=D_L06_001F1890[texture][0];v=D_L06_001F1890[texture][1];
   *ux=u+x;*uy=v+y;*vx=u;*vy=v;
   pf++;pb++;indices+=2;ux+=2;uy+=2;vx+=2;vy+=2;
  }
  }
  func_L00_001FD1D8(&w.front,w.matrix,0);
  func_L00_001FD1D8(&w.back,w.matrix,0);
 } while(++i<50);
}

/* NON_MATCHING func_L01_0028C958 -- src/overlays/shared/partupd_00280428.c
 * Best so far: BYTES 45/672 (93.3% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   mini12/a03 budget8 spent. Computes sampled curve segment lengths, accumulates total, optionally draws debug sa
 *   Only prologue offsets24-64 differ: draw argument spill scheduled before saves/initial reads, then scalar sched
 *   ABI-equivalent C690 float-before-index alias fixes setup; workspace struct fixes vector slots, loading table a
 */
#include "common.h"
typedef int Curve958Quad __attribute__((mode(TI)));
extern void func_L01_00286530(void *,int,int,unsigned int,int,int,int,float);
extern void func_L01_0028C690_f(void *,int *,float,float,int) __asm__("func_L01_0028C690");
extern int func_L01_0028C5F0(char *,int);
extern void func_L01_0028C848(float,void *,void *,void *,void *,void *);
extern float func_001F9D10(void *,void *);
/* Samples curve segments and records their lengths, optionally emitting debug particles. */
float func_L01_0028C958(char *effect,int draw,int style,float a,float b) {
 struct { Curve958Quad p0,next,p1,start,out,prev; } w;
 int count;
 int saved=*(int *)effect;
 char *entries=*(char **)(effect+0x10);
 int i;
 float total=0.0f;
 count=*(int *)entries;
 *(int *)effect=0;
 w.start=*(Curve958Quad *)(entries+0x10);
 if(draw) func_L01_00286530(&w.start,style,0,0x80101080,127,-1,255,0.666f);
 func_L01_0028C690_f(&w.next,(int *)effect,a,b,0);
 for(i=0;i<count;i++) {
  float t,dist;
  w.p1=w.start;w.p0=w.next;
  if(draw) func_L01_00286530(&w.p1,style,i%15+2,0x80108010,127,-1,255,1.0f);
  { int idx=func_L01_0028C5F0(effect,i+1);
    w.start=*(Curve958Quad *)((idx<<4)+(int)*(char **)(effect+0x10)+0x10); }
  *(int *)effect=i;
  func_L01_0028C690_f(&w.next,(int *)effect,a,b,1);
  w.prev=w.p1;
  dist=0.0f;
  for(t=0.05f;t<=1.0f;t+=0.05f) {
   if(draw) func_L01_00286530(&w.prev,style,0,0x80404080,127,-1,255,0.2f);
   func_L01_0028C848(t,&w.out,&w.p1,&w.start,&w.p0,&w.next);
   dist+=func_001F9D10(&w.prev,&w.out);
   w.prev=w.out;
  }
  total+=dist;
  *(float *)(*(char **)(effect+0x10)+(i<<4)+0x1C)=dist;
 }
 *(int *)effect=saved;
 return total;
}

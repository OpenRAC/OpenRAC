/* NON_MATCHING func_L12_00309590 -- src/overlays/l12_hoven/vendor_002EDAA0.c
 * Best so far: SIZE ours 412 / retail 416, checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Updates linked-object completion, effect handle and draw registration.
 *   Stopped at budget 8: p5 and p7 differ in 44 of 416 bytes; saved moby/state registers swap, table setup schedul
 *   Uses actual D_0014C150 and D_0013E650 objects from p3 onward; independently derived allocator/scheduler idiom 
 */
#include "common.h"
extern void func_0020D678(void *);
extern int func_L01_00276680(char *,float);
extern void func_001F49B0(void *,void *);
extern int func_0022ED80_r(int,int,int) __asm__("func_0022ED80");
extern void func_L00_0028EBF0(int);
extern void func_L00_002512D8(int);
extern void func_L12_00309730(void);
extern char *D_L12_00160058 MACRO_ADDR;
extern int D_0015EE84 MACRO_ADDR;
extern char D_0014171B[], D_0013E633[];
/* Updates a linked object, its effect handle and completion state. */
void func_L12_00309590(unsigned char *m) {
 int state=m[0x20];
 int *d=*(int **)(m+0x78);
 switch(state) {
 case 0: {
  int index=m[0xB0];
  unsigned char *table=(unsigned char *)D_0014171B+0xAA35;
  if(index!=255 && table[index+(D_0015EE84<<4)]==255) {
   func_0020D678(m); return;
  }
  m[0x20]=1;
  break;
 }
 case 1: {
  int draw=0;
  if(~func_L01_00276680((char *)m,48.0f)) draw=state;
  if(draw) func_001F49B0((void *)func_L12_00309730,m);
  if(d[1]==-1) d[1]=func_0022ED80_r(0,4,(int)m);
  if(d[0]>=0) {
   int handle=d[1];
   if(handle!=-1) {
    char *effect=D_0013E633+0x1D+handle*0x70;
    if(*(void **)(effect+0x88)==m && (unsigned char)effect[0x74]) func_L00_0028EBF0(handle);
   }
   { char *other=D_L12_00160058+d[0]*256;
   d[1]=-1;
   if((unsigned char)other[0x20]==7) {
    if(m[0xB0]!=255) func_L00_002512D8(m[0xB0]);
    func_0022ED80_r(1,0,(int)m);
    *(int *)(m+0x94)=0;
    m[0x20]=2;
   }
   }
  }
  break;
 }
 }
}

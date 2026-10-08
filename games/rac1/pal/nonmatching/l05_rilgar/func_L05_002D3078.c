/* NON_MATCHING func_L05_002D3078 -- src/overlays/l05_rilgar/vendor_002D28D0.c
 * Best so far: BYTES 17/420 (96.0% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Emits two randomized particles when a countdown expires.
 *   Stopped: p2 for loop, p3 do loop, and p4 while loop produce identical bytes, 17 differing bytes of 420.
 *   Only setup order at +0x40 through +0x5C remains; compiler chooses global lui in branch delay slot instead of i
 */
#include "common.h"
extern int func_001F9908(int *);
extern float func_001FA748(float,float);
extern float func_002140F8(float,float);
extern void func_001F9C30(void *,void *,float);
extern void func_001F9BD8(void *,void *,void *);
extern unsigned char *func_L00_00272770(void *,void *,void *,float,float);
extern int func_001F9850(int);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern char D_L05_001613A8[];
extern float D_L05_001613A8_m __asm__("D_L05_001613A8") MACRO_ADDR;
extern char D_L05_0015F660[] MACRO_ADDR;
/* Emits two randomized particles when the countdown expires. */
void func_L05_002D3078(char *m) {
 float pos[4];
 char *d=*(char **)(m+0x78);
 int i;
 if(!func_001F9908((int *)(d+0x84))) return;
 {
  char *axis=m+0xD0;
  char *origin=m+0x10;
  char *color=D_L05_001613A8;
  i=0;
  while(i<2) {
   unsigned char *p;
   float angle=func_001FA748(*(float *)(m+0x48),3.1415927f);
   angle=func_001FA748(angle,func_002140F8(-0.785398f,0.785398f));
   func_001F9C30(pos,axis,func_002140F8(-0.5f,0.5f));
   func_001F9BD8(pos,pos,origin);
   pos[2]=D_L05_001613A8_m+0.05f;
   p=func_L00_00272770(pos,D_L05_0015F660,color,func_002140F8(0.7f,1.0f),i==0?2.0f:-2.0f);
   if(p) *(short *)(p+0xA)=func_001F9850(15);
   ++i;
   *(int *)(d+0x84)=func_001FA898_r(func_002140F8(3.0f,5.0f));
  }
 }
}

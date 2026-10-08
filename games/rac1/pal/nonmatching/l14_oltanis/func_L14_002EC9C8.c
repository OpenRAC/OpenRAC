/* NON_MATCHING func_L14_002EC9C8 -- src/overlays/l14_oltanis/vendor_002E0538.c
 * Best so far: SIZE ours 676 / retail 680, checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Updates48 particles: inactive slots spawn, active slots fade/move and may be shortened near camera; registers 
 *   mini12 a01 budget8 spent; p2 size676 versus680 restores zero-based interpolation, p6/p7 size672 defer initiali
 *   Unblock requires natural timer/scale addressing retaining retail shared offset and counter rather than multipl
 */
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern int func_00215570(void *,int);
extern int func_001F9938(void *);
extern void func_L14_002EC7E8(void *,int,void *);
extern float func_001FA888(int);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_001F9C30(void *,void *,float);
extern void func_001F9BD8(void *,void *,void *);
extern float func_001F9B88(float);
extern float func_001FA850(float,float);
extern int func_001F9850(int);
extern void func_001F49B0(void *,void *);
extern void func_L14_002ECC70(void);
extern char camera_c9[] __asm__("D_L14_00167480");
extern short D_L14_00161CFC;
extern short D_L14_00161CE0;
extern char D_0013F4D0[];
// Updates forty-eight particles, their fade timers, camera avoidance and draw registration.
void func_L14_002EC9C8(char *m) {
 float direction[4], center[4], scaled[4];
 int active;
 char *d=*(char **)(m+0x78);
 int i=0, off=0;
 short *age=(short *)(d+0x320),*alpha=(short *)(d+0x3e0);
 char *pos=d+0x20;
 direction[0]=func_001F9F90(*(float *)(m+0x48));
 direction[1]=func_001F9FA8(*(float *)(m+0x48));
 direction[2]=0;
 qcopy(center,D_0013F4D0);center[2]+=1;
 active=func_00215570(center,*(int *)(d+0x504));
 do {
  if(func_001F9938(age)) {
   if(!active || i<24)func_L14_002EC7E8(m,i,direction);
  } else {
   short *limit=(short *)(d+0x380+off);
   short *start=(short *)(d+0x320+off);
   float f;
   int max=*(int *)&D_L14_00161CE0;
   if(*limit-*(int *)&D_L14_00161CFC<*start)
    f=func_001FA888(*limit-*start);
   else f=func_001FA888(*start);
   f/=func_001FA888(*limit-*(int *)&D_L14_00161CFC);
   *alpha=func_001FA898_r(0.0f+((float)max-0.0f)*f);
   func_001F9C30(scaled,direction,*(float *)(d+0x440+i*4));
   func_001F9BD8(pos,pos,scaled);
   if(active && func_001F9B88(*(float *)pos-*(float *)(camera_c9+0x140))<8 &&
      func_001F9B88(*(float *)(pos+4)-*(float *)(camera_c9+0x144))<8 &&
      func_001FA850(*(float *)(m+0x48),*(float *)(camera_c9+0x158))>2.3561945f) {
    if(func_001F9850(20)<*start)*start=func_001F9850(20);
   }
  }
  i++;off+=2;pos+=16;age++;alpha++;
 } while(i<48);
 func_001F49B0(func_L14_002ECC70,m);
}

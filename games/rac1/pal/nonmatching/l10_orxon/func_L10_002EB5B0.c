/* NON_MATCHING func_L10_002EB5B0 -- src/overlays/l10_orxon/vendor_002E30F8.c
 * Best so far: BYTES 45/756 (94.0% of the bytes match), checked 2026-10-07.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   mini16 a01: Point-emitter state dispatch over bounded path points. Best p3.c BYTES45/756; scoped loop pointers
 *   Remaining pure saved-register cycles in terrain block +d4..100, first effect loop +18c..1c0, second effect loo
 */
extern float func_00214358(void *,int,float);
extern void func_L10_002EB8A8(void *,short *,short *);
extern void func_L10_002EBDC8_a01(float,void *,short *,short *) __asm__("func_L10_002EBDC8");
extern void func_L10_002EBB70(void *,short *);
extern char *D_L10_001B0C30[];
// Updates a point emitter, optionally applying effects over up to 32 path points.
void func_L10_002EB5B0(char *m) {
 char *d=*(char **)(m+0x78);
 switch((unsigned char)m[0x20]) {
 case 0:
  switch(*(int *)d) {
  case 0:m[0x20]=1;break;
  case 1:
   if(*(int *)(d+4)!=-1) {
    char *tab=D_L10_001B0C30[*(int *)(d+4)];
    int n=*(int *)tab;
    if(n>32)n=32;
    if(n>0) {float *height=(float *)(tab+0x1C);int count=n;char *pos=tab+0x10;
    do {*height=func_00214358(pos,0,0.5f);pos+=0x10;height+=4;}while(--count);}
   }else *(float *)(d+0x88)=func_00214358(m+0x10,0,0.5f);
   m[0x20]=2;break;
  case 2:m[0x20]=3;break;
  }
  break;
 case 1:
  if(*(int *)(d+4)==-1)func_L10_002EB8A8(m+0x10,(short *)(d+8),(short *)(d+0x48));
  else {
   char *tab=D_L10_001B0C30[*(int *)(d+4)];int n=*(int *)tab;
   if(n>32)n=32;
   if(n>0) {short *timer=(short *)(d+8);char *pos=tab+0x10;int count=n;short *burst=(short *)(d+0x48);
   do {func_L10_002EB8A8(pos,timer++,burst);++burst;--count;pos+=0x10;}while(count);}
  }
  break;
 case 2:
  if(*(int *)(d+4)==-1)func_L10_002EBDC8_a01(*(float *)(d+0x88),m+0x10,(short *)(d+8),(short *)(d+0x48));
  else {
   char *tab=D_L10_001B0C30[*(int *)(d+4)];int n=*(int *)tab;
   if(n>32)n=32;
   if(n>0) {short *timer=(short *)(d+8);float *height=(float *)(tab+0x1C);int count=n;char *pos=tab+0x10;short *burst=(short *)(d+0x48);
   do {func_L10_002EBDC8_a01(*height,pos,timer,burst);height+=4;++burst;++timer;--count;pos+=0x10;}while(count);}
  }
  break;
 case 3:
  if(*(int *)(d+4)==-1)func_L10_002EBB70(m+0x10,(short *)(d+8));
  else {
   char *tab=D_L10_001B0C30[*(int *)(d+4)];int n=*(int *)tab;
   if(n>32)n=32;
   if(n>0) {short *timer=(short *)(d+8);char *pos=tab+0x10;int count=n;
   do {func_L10_002EBB70(pos,timer);++timer;--count;pos+=0x10;}while(count);}
  }
  break;
 }
}

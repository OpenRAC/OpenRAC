/* NON_MATCHING func_L00_00267BA8 -- src/overlays/shared/stream_002670E0.c
 * Best so far: SIZE ours 156 / retail 160, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Fragment: the loop's continue path (beql ... func_L00_00267C34) branches outside the function into the next sy
 *   mini3/a02: joined target is 160 bytes; exact actual symbol D_0013CA40 replaces unrelated base+offset. Best p6 
 *   Stopped after struct indexing, explicit ring local, and explicit bits local (p4/p7/p8) produce identical 152-b
 */
extern char D_0013CA40_h[] __asm__("D_0013CA40");
/* Returns buffered pressed mask bits and their age. Adapted from Lombyte (MIT) for PAL: src/overlays/shared/ui_menus_00266d60.c, FUN_L00_00266d60. */
int func_L00_00267BA8(int mask,int n,int *out) {
 char *b=D_0013CA40_h;
 int lim=*(int *)(b+0x190),i;
 if(lim>=n) lim=n;
 for(i=1;i<lim+1;++i) {
 int k=(30+(*(short *)(b+0x18e)-i))%30;
 int *p=(int *)(b+0x1e0+k*4);
 if(*p&mask) { if(out) *out=i-1; return *p&mask; }
 }
 return 0;
}

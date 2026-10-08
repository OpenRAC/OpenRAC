/* NON_MATCHING func_L00_00207CC0 -- src/overlays/shared/help_00203E98.c
 * Best so far: SIZE ours 516 / retail 512, checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Fades player packed color, flashes temporary effects, and updates an attachment.
 *   p7 is 492 bytes versus retail 512: channel register/order differences near 0x50–0x88 and missing block-local b
 *   Stopped at budget 8; packed-color local allocation and hero-pointer lifetime/scope need another source formula
 */
extern void func_L00_00251328(void *, int, int, int);
extern int func_001F9850(int);
extern void func_L00_00251358(void *, void *, void *, void *);
extern void func_L00_002078E0(void);
extern void func_L00_00207948(void *, char *);
extern int D_0015EE84_m __asm__("D_0015EE84") MACRO_ADDR;
extern unsigned char D_0013F450[] NOT_SDA;
extern unsigned char D_0013E620[];
// Fades player color, flashes temporary effects, and updates active attachments.
void func_L00_00207CC0(void) {
 char *h=D_0013F450; int flash=0;
 if (*(int *)(h+0x22f4)) {
  int b,g,r; int color=*(int *)(h+0x22f8);
  b=color>>16; g=color>>8; r=color;
  b=(b&255)-7; g=(g&255)-7; r=(r&255)-7;
  if(b<0)b=0; if(r<0)r=0; if(g<0)g=0;
  func_L00_00251328(*(void **)(h+0x2080),r,g,b);
  *(unsigned int *)(h+0x22f8)=(((unsigned int)color>>24)<<24)|(b<<16)|(g<<8)|r;
 }
 if (*(int *)(h+0x2084)==0x80 || *(int *)(h+0x2084)==0x82) flash=*(int *)(h+0x198)<func_001F9850(30);
 { char *h=(char *)D_0013F450;
 if ((D_0015EE84_m==15 || D_0015EE84_m==17) && *(int *)(h+0x2084)==0x76) {
  if (*(int *)(h+0x198)<func_001F9850(20)) flash=1;
 }
 }
 if(flash) {
  char *h=(char *)D_0013F450;
  int r,g,b;
  func_L00_00251358(*(void **)(h+0x2080),&r,&g,&b);
  if(*(int *)(h+0x198)%4<3) {r=0;g=0;b=0;} else {g=144;b=240;r=144;}
  func_L00_00251328(*(void **)(h+0x2080),r,g,b); func_L00_002078E0();
 }
 { char *h=(char *)D_0013F450;
 if(*(int *)(h+0x10b8)>=0 && D_0013E620[*(int *)(h+0x10b8)]!=0 && *(void **)(h+0x1090)!=0) func_L00_00207948(*(void **)(h+0x1090),*(char **)(h+0x2080));
 }
}

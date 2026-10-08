/* NON_MATCHING func_L10_002E6D28 -- src/overlays/l10_orxon/vendor_002E30F8.c
 * Best so far: SIZE ours 624 / retail 628, checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Updates collision reactions and chooses an enemy movement target.
 *   p7 is 620 versus retail 628 bytes: only missing hero high-base preservation move at 0x178 and final low-base r
 *   Stopped at budget8; need natural high-address lifetime splitting while retaining actual D_0013F450 symbol. p7 
 */
extern char *func_L00_0025B478(void *,int,int);
extern int func_L00_0025B4D0(void *,void *,void *,int,int *,float *,int,int);
extern void func_L00_0025E4B0(void *,short *);
extern int func_001F9850(int);
extern void func_00213DE0(void *,int,int,int);
extern void func_L00_0025E590(void *,void *);
extern void func_L06_002F4908(float,void *,void *);
extern int func_L00_0025A778(void *,void *,int);
extern int func_L00_00260FB0(float,char *,void *,int,int,void *,int);
extern char *D_L10_001B0C30[];
extern unsigned char D_0013F450[];
// Updates hit reactions and selects an enemy's movement target.
void func_L10_002E6D28(char *m) {
 char *d=*(char **)(m+0x78),*hit=func_L00_0025B478(m,0x370000,0); int flag; float amount=0;
 int result=func_L00_0025B4D0(m,hit,d+0x20,5,&flag,&amount,0,4);
 if(hit && flag!=1 && (unsigned char)m[0x20]!=8) {
  *(float *)(d+0x20)-=amount; if(*(float *)(d+0x20)<=0) result=1;
  if(*(int *)(hit+0x24)&0x40000) result=1;
  switch(result) {
  case 1: goto death;
  case 2: goto react;
  case 3: goto react;
  case 4: goto react;
  case 5: goto react;
  case 6: goto react;
  case 7: goto react;
  case 8: goto react;
  case 9: goto stun;
  case 10: goto stun;
  case 11: goto react;
  case 12: goto stun;
  case 13: goto finish;
  default: goto react;
  }
 stun:
  ((unsigned char *)d)[0xc7]=250;func_L00_0025E4B0(m,(short *)(d+0xc0));goto finish;
 death:
  ((unsigned char *)d)[0xc7]=250;func_L00_0025E4B0(m,(short *)(d+0xc0));m[0x20]=8;goto finish;
 react:
  ((unsigned char *)d)[0xc7]=250;*(short *)(d+0x26)=func_001F9850(60);
  func_L00_0025E4B0(m,(short *)(d+0xc0));func_00213DE0(m,4,0,func_001F9850(6));
  m[0xbc]=m[0x20];m[0x20]=7;
 }
 finish:
 ((unsigned char *)m)[0xa4]=255;func_L00_0025E590(m,d+0xc0);
 { unsigned char *hero=D_0013F450;
 if(hero[0x20a4]==1) {
  char *v;func_L06_002F4908(101.0f,m,d+0x110);
  if(*(char **)(d+0x110)==0) *(int *)(d+0x114)=2;
  else {v=D_L10_001B0C30[*(int *)(d+0x190)];
   if(func_L00_0025A778(*(char **)(d+0x110)+0x10,v+0x10,*(int *)v)) {*(int *)(d+0x114)=0;goto finaltarget;}
   else {*(int *)(d+0x110)=0;*(int *)(d+0x114)=2;}
  }
 } else {
  char *v=D_L10_001B0C30[*(int *)(d+0x190)];
  if(func_L00_00260FB0(101.0f,m,d+0xd0,0,0,v+0x10,*(int *)v)!=2 && (unsigned char)m[0x31]!=0) *(int *)(d+0x114)=2;
 }
finaltarget:
 if(*(int *)(d+0x110)==0) *(char **)(d+0x110)=*(char **)(hero+0x2080);
 }
}

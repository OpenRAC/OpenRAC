/* NON_MATCHING func_L11_002D3248 -- src/overlays/l11_pokitaru/vendor_002CC828.c
 * Best so far: SIZE ours 696 / retail 692, checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Changes animation and initializes state-specific vectors/timers. Budget8 exhausted; p7 SIZE684/692 with indexe
 *   Remaining differences are the prologue byte-store/switch masking schedule and missing argument moves in state7
 *   A parameter/store form matching entry scheduling and call setup would unblock it; small-data vectors are decla
 */
extern int func_001F9850(int);
extern void func_00213DE0(void *, int, int, int);
extern void func_001F9BC0(void *);
extern void func_001F9EC0(void *, void *, void *);
extern void func_001F9BD8(void *, void *, void *);
extern float D_0015EE6C MACRO_ADDR;
extern unsigned char *D_L11_00160058_t __asm__("D_L11_00160058") MACRO_ADDR;
extern short D_L11_00161460,D_L11_00161464;
extern short D_L11_00161470;
extern short D_L11_00161490;
/* Changes a moby's animation and initializes state-specific vectors and timers. */
void func_L11_002D3248(unsigned char *m,int state) {
 float tmp[4];
 char *d=*(char **)(m+0x78);
 float speed;
 m[0x20]=state;
 switch((unsigned char)state) {
 case 1:
idle:
 if(m[0x53])func_00213DE0(m,0,0,func_001F9850(20));
 if(m[0x20]==14)*(float *)(d+0x2C)=10.0f;
 func_001F9BC0(d+0x130);
 if(m[0x20]==14)*(short *)(d+0x56)=2;
 return;

 case 2: break;
 case 3: goto idle;
 case 4:
  if(m[0x53]!=2)func_00213DE0(m,2,0,func_001F9850(20));
  speed=*(float *)&D_L11_00161460;goto setspeed;
 case 5:
  if(m[0x53]!=3)func_00213DE0(m,3,0,func_001F9850(20));
  speed=*(float *)&D_L11_00161464;goto setspeed;
 case 6:
  if(m[0x53]!=3)func_00213DE0(m,3,0,func_001F9850(20));
  speed=*(float *)&D_L11_00161464;
setspeed:
 *(float *)(d+0x154)=speed*D_0015EE6C;return;

 case 7:
  if(m[0x53]!=4)func_00213DE0(m,4,0,func_001F9850(20));
 goto clear;
 case 8:
  if(m[0x53]!=3)func_00213DE0(m,3,0,func_001F9850(20));
  {
  char *other=(char *)D_L11_00160058_t+(((int *)(d+0xF0))[*(int *)(d+0x158)!=0]<<8);
  qcopy(d+0x120,other+0x10);
  func_001F9EC0(tmp,&D_L11_00161470,other+0xC0);
  func_001F9BD8(d+0x120,d+0x120,tmp);
  }
  break;
 case 9:
reset:
 if(m[0x53])func_00213DE0(m,0,0,func_001F9850(20));
clear:
 func_001F9BC0(d+0x130);
 return;


 case 10:
  if(m[0x53]!=3)func_00213DE0(m,3,0,func_001F9850(20));
  {
  char *other=(char *)D_L11_00160058_t+(((int *)(d+0xF0))[*(int *)(d+0x158)!=0]<<8);
  qcopy(d+0x120,other+0x10);
  func_001F9EC0(tmp,&D_L11_00161490,other+0xC0);
  func_001F9BD8(d+0x120,d+0x120,tmp);
  }
  break;
 case 11: goto idle;
 case 12: goto idle;
 case 13: break;
 case 14: goto idle;
 case 15: break;
 case 16: goto reset;
 }
 return;
}

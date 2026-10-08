/* NON_MATCHING func_L00_0024EEF0 -- src/overlays/shared/missionfunc_0024EEF0.c
 * Best so far: SIZE ours 296 / retail 300, checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Initializes or resets function parameters, clearing two values at provided pointers when a second parameter is
 *   ## Where the difference is
 *   Generated code is 52 bytes vs retail 56 bytes. The instruction sequence differs in how branches are structured
 *   ## What would unblock it
 *   The 4-byte difference may be due to different branch prediction patterns or the bnel likely instruction requir
 *   Joined retry budget spent. Collects mission IDs or alternate IDs, optional state mask and indices, and complet
 *   p9 is 296 versus 300 bytes: only missing daddu v1,a0,zero at +0x88; ternary gets beql right. Moving output inc
 *   Unblock requires a plain C pointer-lifetime idiom retaining that move with the pointer increment at +0xD8.
 */
extern short *D_L00_001870A0;
/* collects active mission entries and returns their count */
int func_L00_0024EEF0(int *out,int *flags,int *indices,int alternate) {
 char *p=(char *)D_L00_001870A0; int count=0; int complete=1;
 *out=0; if(flags)*flags=0; if(indices)*indices=-1; if(!p)return 0;
 while(*(short *)p) {
 int state=*(short *)(p+0x24); unsigned int bits=*(unsigned short *)(p+0x10);
 if(state!=2 && !(bits&2))complete=0;
 if(!(bits&2) && state && (!(bits&1)||state!=2)) {
 int *dest=out; *out=*(short *)p;
 if(alternate) { int value=(*(short *)(p+0x24)==2)?0x5243:*(short *)(p+0x14+*(short *)(p+0x26)*2); *dest=value; }
 if(flags && *(short *)(p+0x24)==2)*flags|=1<<count;
 ++out; if(indices) { *indices=*(short *)(p+0x12)+*(short *)(p+0x26); ++indices; }
 ++count;
 }
 p+=0x28;
 }
 if(flags && complete)*flags|=0x80000000; return count;
}

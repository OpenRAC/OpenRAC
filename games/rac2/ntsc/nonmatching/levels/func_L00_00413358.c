/* GPT-6.1 Sol high, task v1058, USA v1.01: 1304/1304 bytes, 0/27 places.
 * Profile: -O2 -G8 -ffunction-sections (no -g).
 * The frame, initial FP saves, spacing calculation's FP registers, and first
 * point-loop registers now agree. Integer offset before table-base addition
 * recovers the hit-record lookup's register assignment. Remaining differences:
 * packet byte constant registers, two absent nops before the spacing div.s,
 * phase-store scheduling, second-loop FP allocation/expression scheduling, and
 * burst constant lifetime / final particle-size and argument setup.
 * Separate first/second loop counters and conversion temporaries improve the
 * draft. Splitting the second-loop accumulator shifts allocation again; explicit
 * frameTime and final-size locals under -g reached 1308 bytes but did not match.
 * Reusing the spacing/step/ratio locals for later phases changes earlier FP
 * allocation adversely. Next useful work is the original second-loop expression
 * and burst temporary lifetimes, not manual padding or volatile casts.
 * Declarations needed by this draft follow. Not a linked match.
 */
#include "common.h"
typedef unsigned int Q __attribute__((mode(TI)));
typedef union { Q q; float f[4]; } V;
extern float D_L00_001B13A8 __attribute__((sda));
extern unsigned char D_00139595[];
typedef struct {char pad[112]; float scale; char rest[108];} HitRecord;
extern HitRecord D_L00_00262570[];
extern char D_L00_001A8F20[], ref_L00_00413358_00[];
extern float D_L00_001B1390 __attribute__((sda));
extern float D_L00_001B1398 __attribute__((sda));
extern float D_L00_001B13A4 __attribute__((sda));
extern float D_L00_001B1398 __attribute__((sda));
extern float D_L00_001B1390 __attribute__((sda));
extern float D_L00_001B1394 __attribute__((sda));
extern float D_L00_001B139C __attribute__((sda));
extern float D_L00_001B13A0 __attribute__((sda));
extern float D_L00_001B13AC __attribute__((sda));
extern float D_L00_001B13B0 __attribute__((sda));
extern float D_L00_001B13B4 __attribute__((sda));
extern float D_L00_001B13B8 __attribute__((sda));
extern float D_L00_001B13BC __attribute__((sda));
extern float D_L00_001B13C0 __attribute__((sda));
extern float func_00282E48(void *,void *),func_00283CE0(int),func_00283B98(float,float),func_00283BE0(float,float),func_002ABB18(float),func_00283198(float),func_00283180(float),func_002A78D8(float,float),func_002A7878(float,float),func_002A7940(void);
extern int func_00282978(void *),func_002A7820(int,int),func_002A77E0(int);
extern void func_00282CF0(void *,void *,void *),func_00282F20(void *,void *,float),func_002AF000(void *,float,float,float),func_L00_002E6EF8(void *,void *),func_002A8E20(void *,void *);
extern char *func_002C2210(void *,int,float,float,float,int,int,int,void *,int);
void func_L00_00413358(char *object)
{
    union { V vector; struct { char pad[16];char *owner; int flags; unsigned char kind,mode;unsigned short id;float scale;int enabled; } packet; } scratch;
    char *state=*(char **)(object+0x68);
    float distance=func_00282E48(object+0x10,state+0x30);
    float ratio,spacing,step,angle,value,t,a,b,size;
    float point10, amp10, point19, phase19, sine19;
    int i,sign,i10,i19;
    if(*(short *)(state+0x1c)!=0) {
        if(func_00282978(state+0x1c) && *(int *)(state+0x20)!=0) {
            scratch.packet.flags=0x810000;
            scratch.packet.enabled=1;
            scratch.packet.owner=object;
            scratch.packet.scale=((HitRecord *)(D_00139595[0]*sizeof(HitRecord)+(int)D_L00_00262570))->scale;
            func_00282CF0(&scratch,state+0x30,object+0x10);
            func_00282F20(&scratch,&scratch,1);
            scratch.packet.kind=3;
            scratch.vector.f[3]=5627.925f;
            scratch.packet.mode=2;scratch.packet.id=*(unsigned short *)(object+0xaa);
            func_002A8E20(*(void **)(state+0x20),&scratch);
        }
    }
    i=0;
    step=.017453294f;
    point10=func_00283CE0(*(short *)(state+0x1c));
    point10=point10/func_00283CE0(6);
    ratio=distance+(.01f-distance)*point10;
    spacing=ratio/func_00283CE0(10);
    *(float *)(state+8)=func_00283B98(*(float *)(state+8),D_L00_001B13A0*step);
    *(float *)(state+12)=func_00283BE0(*(float *)(state+12),D_L00_001B13A8);
    *(float *)(state+16)=func_00283B98(*(float *)(state+16),D_L00_001B13B4);
    step=(360/func_00283CE0(10))*step;
    for(i10=0;i10<10;i10++) {
        point10=func_00283CE0(i10);
        func_00283CE0(10);
        amp10=D_L00_001B1398;
        if(i10==0||i10==9) amp10=0;
        else {amp10*=func_00283198(func_002ABB18(*(float *)(state+8)+point10*step));amp10+=func_002A78D8(0,D_L00_001B1394);}
        *(float *)(*(char **)(state)+i10*16+8)=amp10;
        *(int *)(*(char **)(state)+i10*16+4)=0;
        *(float *)(*(char **)(state)+i10*16)=spacing*point10;
        *(float *)(*(char **)(state)+i10*16+12)=1;
    }
    for(i19=0;i19<19;i19++) {
        point19=func_00283CE0(i19);
        *(int *)(*(char **)(state+4)+i19*16+8)=0;
        phase19=func_002ABB18(*(float *)(state+12)+point19*D_L00_001B13AC);
        angle=func_002ABB18(*(float *)(state+16)+point19*D_L00_001B13B8);
        amp10=D_L00_001B13BC*func_00283198(angle);
        sine19=func_00283198(phase19);
        angle=func_002A78D8(D_L00_001B1390,D_L00_001B1394);
        amp10=amp10*sine19+angle;
        *(float *)(*(char **)(state+4)+i19*16+8)+=amp10;
        *(float *)(*(char **)(state+4)+i19*16+4)=D_L00_001B13C0*func_00283180(phase19);
        *(float *)(*(char **)(state+4)+i19*16)=spacing*point19;
        *(float *)(*(char **)(state+4)+i19*16+12)=1;
    }
    func_L00_002E6EF8(ref_L00_00413358_00,object);
    if(*(short *)(state+0x1c)<=0) {
        for(i=4;i>=0;i--) {
            t=func_002A7878(1,6)*.016666668f;
            a=func_002A7940();b=func_002A7940();
            func_002AF000(&scratch,t,a,b);
            sign=func_002A7820(1,4);if(func_002A77E0(2))sign=-sign;
            size=func_002A7878(.4f,.8f);
            func_002C2210(state+0x30,15,size*.3f,size,0,*(int *)(state+0x24)|0x7f000000,0,sign,&scratch,0);
            func_002C2210(state+0x30,15,size*.15f,size*.5f,0,0x307f7f7f,0,-sign,&scratch,0);
        }
        sign=func_002A7820(1,8);if(func_002A77E0(2))sign=-sign;
        size=func_002A7878(.15f,3);
        func_002C2210(state+0x30,12,size*.1f,size,0,*(int *)(state+0x24)|0x7f000000,0,sign,D_L00_001A8F20,0);
    }
}

/*
 * Tier: opus-1 (Claude Opus 5.5, xhigh), task v1908. USA v1.01, levels 04 and 11, entry 003FA7E8.
 * Best check.py result: 4540 bytes / 4516 expected, 0 of 2 places match; frame 0x370.
 * Unit to reproduce it: this file as src/levels/shared/L04_003FA7E0.c, with the marker in
 * L04_003C8380.c replaced by a comment, and this line in config/cflags.txt:
 *   src/levels/shared/L04_003FA7E0.c  -O2 -G0 -ffunction-sections -g -frac2-gcse-unsplit-edges
 * -g gives the pre-div.s nop pair at 003FAAA8; unsplit edges removes the state+0x24 PRE load.
 * All declarations it needs are below. Not a behavioural claim: it has never been linked.
 *
 * What still differs, in the whole 1129-instruction listing:
 *  1. The template copy (+24 bytes, the entire size difference). `settings = D_L04_002CA100;`
 *     gets retail's loop shape, pointers and scratch registers (src v0, dst v1, end a0, data
 *     a1/a2, final data a0, and the reload `por $t0` displaced by the block move's four
 *     scratches), but as four ld/sd per 32-byte step and two for the last 16, where retail
 *     has two lq/sq and one. This cc1 cannot emit that: mips.c expand_block_move clamps
 *     alignment to UNITS_PER_WORD and output_block_move has no 128-bit unit, and no patch or
 *     transformer in tools/compiler touches them. __builtin_memcpy gives the same ld/sd.
 *     An explicit TImode loop is not a way round it: its data pseudos are block-local, so
 *     local-alloc gives them v0/v1 before global-alloc places the pointers (measured: dst a0,
 *     src a1, end a2, data v0/v1, `por $v0`), and it shifts the float registers after it.
 *  2. One scheduling row at retail 003FB824: `sw $v0,0x168($s1)` comes after the argument
 *     moves in retail and before them here. sched1 already orders it that way; why was not
 *     established. Same bytes from: the two stores swapped, a typed camera struct versus
 *     byte offsets, a camera pointer versus the global written directly.
 * All other rows are identical or differ only by relocation (the object is unlinked).
 * This wording is the one to keep if the compiler gains quadword block moves: only item 2
 * would then remain.
 *
 * Wordings that reproduce retail here (each measured; the causes are inferred from the output
 * and the dumps):
 *  - both turn calls are written out in each arm, and jump2 cross-jumps their tails; that is
 *    what puts yaw in the delay slot and lets the third turn parameter share its register;
 *  - the three loops are counted for-loops that loop reversal rewrites; the first has its own
 *    counter and the other two share one, which sets the s6/s7/fp and s0-s3 order;
 *  - the steering distance reuses `jitter`; the arrival threshold and camera distance are separate;
 *  - the result matrices are declared result-first (0x1B0 result, 0x230 first factor);
 *  - the cross matrix is written column by column, as retail's store order shows;
 *  - the argument banks are interleaved (func_002BBCB0, func_00308408, func_00277F48);
 *  - each particle call has its own lifetime locals and colour pointers;
 *  - the exit test is timer || !owner || distance > 260, then destroy.
 * The colour/vector union preserves retail's late scratch overlap at 0x280.
 * The dead store to direction.v[0] is in retail (003FB7B4).
 */
#include "common.h"
/* Reconstructed projectile steering and impact update. */
typedef int HomingUpdateQuad __attribute__((mode(TI), aligned(16)));
typedef union { float v[4]; HomingUpdateQuad q; } HomingUpdateVector;
typedef struct { HomingUpdateVector v[5]; } __attribute__((aligned(16))) HomingUpdateTemplate;
typedef struct { unsigned int v[6]; } HomingUpdateColors;
typedef union { HomingUpdateColors colors; HomingUpdateVector vectors[2]; } HomingUpdateColorScratch;
extern HomingUpdateTemplate D_L04_002CA100;
extern unsigned char *D_L04_001FF25C;
extern float D_L04_001B200C MACRO_ADDR;
extern unsigned char D_L04_001B9540[];
extern unsigned char D_L04_001C3B40[];
extern char D_L04_001B9680[];
extern HomingUpdateColors D_L04_001B0458, D_L04_001B0470;
extern void f_30EE38(void *, void *, float) __asm__("func_00282D30");
extern void f_30ED98(void *, void *, void *) __asm__("func_00282CC0");
extern void f_30EDB0(void *, void *, void *) __asm__("func_00282CD8");
extern void f_30EDC8(void *, void *, void *) __asm__("func_00282CF0");
extern float f_30F468(float, float) __asm__("func_00283248");
extern float f_30EFA8(void *, void *) __asm__("func_00282E80");
extern float f_30EF28(void *) __asm__("func_00282E20");
extern float f_30EEB8(void *, void *) __asm__("func_00282DB0");
extern float f_30EEF8(void *) __asm__("func_00282DF0");
extern float f_30FF88(int) __asm__("func_00283CE0");
extern int f_30FF98(float) __asm__("func_00283CF0");
extern int f_30E9D8(void *) __asm__("func_00282948");
extern void f_30EA08(void *) __asm__("func_00282978");
extern float f_33BB18(float, float) __asm__("func_002A7878");
extern float f_33BBE0(void) __asm__("func_002A7940");
extern int f_33BA80(int) __asm__("func_002A77E0");
extern int f_33BAC0(int, int) __asm__("func_002A7820");
extern void f_33FA90(void *, float, void *, float, float, float, int) __asm__("func_002AA898");
extern void f_346220(void *, float, float, float) __asm__("func_002AF000");
extern void f_30F090(void *, void *, float) __asm__("func_00282F20");
extern void f_30F128(void *, void *, float) __asm__("func_00282FB8");
extern void f_30F270(void *, void *, void *) __asm__("func_00283098");
extern void f_3425D0(void *, void *, void *, float) __asm__("func_002ACEC0");
extern void f_357930(void *, void *, int, int, int, int, float) __asm__("func_002BD8B0");
extern unsigned char *f_35BC18(void *, void *, int, int, int, int, float, float, float, float, float) __asm__("func_002C16C0");
extern unsigned char *f_40EC80(void *, void *, void *, void *, void *, void *, float, float) __asm__("func_L04_0040EC80");
extern void f_3FA350(void *, void *, void *) __asm__("func_L04_003FA350");
extern void f_40ACC8(void) __asm__("func_L04_0040ACC8");
extern int f_3F9ED0(void *, void *, void *, void *, void *, void *, void *) __asm__("func_L04_003F9ED0");
extern void f_382EA0(int, int, void *, int) __asm__("func_002E5DA0");
extern int f_301948(void *, int, void *, float, int) __asm__("func_00277F48");
extern void f_33E8C0(void *, void *, void *, int, int, int, int, int, float, float, float) __asm__("func_002A9850");
extern void f_30F9E8(void *, void *, void *) __asm__("func_00283788");
extern float f_30F3A0(float) __asm__("func_00283180");
extern float f_30F3B8(float) __asm__("func_00283198");
extern void f_30F958(void *, void *, float) __asm__("func_L03_002EFB60");
extern void f_30FA30(void *, void *, void *) __asm__("func_L04_0030FA30");
extern void f_30F5A8(void *) __asm__("func_00283388");
extern void f_30F298(void *, void *, void *) __asm__("func_002830C0");
extern void f_356A70(void *, void *, int, int, int, int, int, int, int, float) __asm__("func_002BCB00");
extern void f_3B4EA0(void *, void *, int, int, int) __asm__("func_00308600");
extern void f_355C20(void *, void *, int, int, int, int, float, float, int, int) __asm__("func_002BBCB0");
extern void f_3557A8(void *, void *, int, int, int, float) __asm__("func_002BB838");
extern void f_3B2BC0(void *, void *, void *, float, int, unsigned char, unsigned char, unsigned char, unsigned char) __asm__("func_00308408");
extern void f_3C2B38(void *, void *, int, int) __asm__("func_00312B70");
extern void f_4356D0(void *, void *, void *, float) __asm__("func_0032DA58");
extern void f_3317E8(void *) __asm__("func_0029F338");
typedef struct {
    unsigned char pad0[0x140];
    float focus[4];
    unsigned char pad150[0x10];
    float shakeA;
    int pad164;
    int shakeTimeA;
    int shakeClockA;
    unsigned char pad170[0x10];
    float shakeB;
    int pad184;
    int shakeTimeB;
} HomingCamera;
#define BF(p,o) (*(float *)((unsigned char *)(p)+(o)))
#define BI(p,o) (*(int *)((unsigned char *)(p)+(o)))
#define BP(p,o) (*(unsigned char **)((unsigned char *)(p)+(o)))
void func_L04_003FA7E0(unsigned char *m) {
    HomingUpdateTemplate settings;
    HomingUpdateVector cameraMotion, oldPos, velocity, normal, trailMotion, direction, hitPos, acceleration, motion, steeringTemp, targetPos, temp, random;
    float cross[16], crossSquared[16], matC[16], matB[16], matA[16];
    HomingUpdateColorScratch colorsA;
    HomingUpdateColors colorsB, colorsC;
    float outX, outY, outZ;
    int collision;
    unsigned char *state, *target, *owner, *ownerState, *particle;
    HomingCamera *camera;
    float yaw, pitch, jitter, threshold, camDist, angle, speed, fade;
    int i, j, count, lifeA, lifeB;
    settings=D_L04_002CA100;
    collision=0;
    velocity.q = 0; velocity.v[2] = 0.13333334f;
    normal.q = 0; normal.v[2] = 1.0f;
    acceleration.q = 0;
    state = BP(m,0x68);
    BF(state,0x30) += (BF(state,0x44) - BF(state,0x30)) * 0.1f;
    target = BP(state,0x28);
    if (target && *(short *)(target+0xAA) == 0xBBC) {
        if (target[0xBC] == 6) goto destroy;
        ownerState = BP(target,0x68);
        qcopy(&targetPos,target+0x10);
        f_30EE38(&steeringTemp,target+0xC0,1.75f);
        f_30ED98(&targetPos,&targetPos,&steeringTemp);
        if (BI(ownerState,0x744)) {
            jitter = f_33BB18(-0.25f,0.25f);
            if (!(BI(ownerState,0x744)&1)) jitter += 1.5f;
            else jitter -= 1.5f;
            f_30EE38(&steeringTemp,target+0xD0,jitter);
            f_30ED98(&targetPos,&targetPos,&steeringTemp);
            f_30EE38(&steeringTemp,target+0xC0,1.0f);
            f_30ED98(&targetPos,&targetPos,&steeringTemp);
        }
        yaw = f_30F468(targetPos.v[0]-BF(m,0x10),targetPos.v[1]-BF(m,0x14));
        pitch = -f_30F468(f_30EFA8(m+0x10,&targetPos),targetPos.v[2]-BF(m,0x18));
    } else {
        if (!target) {
            *(HomingUpdateQuad *)&temp = *(HomingUpdateQuad *)(m+0x10);
            *(HomingUpdateQuad *)&random = *(HomingUpdateQuad *)(m+0xC0);
            target = f_40EC80(m,&temp,&random,&outX,&outY,&outZ,0.3490659f,30.0f);
            if (target) { BP(state,0x28)=target; BF(state,0x2C)=outZ; }
        }
        owner = BP(state,0x20);
        if (owner && *(short *)(owner+0xAA)==0xBBC) {
            unsigned char *ownerData = BP(owner,0x68);
            f_30EA08(state+0x4C);
            jitter = f_30FF88(*(short *)(state+0x4C));
            jitter /= f_30FF88(*(short *)(state+0x4E));
            f_30EE38(&acceleration,ownerData+0x290,jitter);
        }
        if (target) {
            f_30EDC8(&motion,target+0x10,m+0x10);
            jitter = f_30EEB8(&motion,&motion);
        } else jitter=200.0f;
        if (jitter>32.0f && (target || BI(state,0x24)>60)) {
            f_3FA350(m,state+0x3C,state+0x40);
            yaw=BF(state,0x3C);
            pitch=BF(state,0x40);
        } else if (target && BI(state,0x24)<300 && (signed char)target[0x20]>=0) {
            qcopy(&targetPos,target+0x10);
            targetPos.v[2]+=BF(state,0x2C);
            f_30EDC8(&motion,&targetPos,m+0x10);
            yaw=f_30F468(motion.v[0],motion.v[1]);
            pitch=-f_30F468(f_30EF28(&motion),motion.v[2]);
        } else if (BI(state,0x24)<300) {
            yaw=BF(state,0x3C); pitch=BF(state,0x40);
        } else { yaw=BF(m,0xF8); pitch=BF(m,0xF4); }
    }
    if (target) {
        if (*(short *)(target+0xAA)==0xBBC) {
            f_33FA90(m+0xF8,yaw,state+0x34,0.1f,0.28f,0.20943955f,0);
            f_33FA90(m+0xF4,pitch,state+0x38,0.1f,0.28f,0.20943955f,0);
        } else {
            f_33FA90(m+0xF8,yaw,state+0x34,2.4f,0.18f,0.17453294992446899414f,0);
            f_33FA90(m+0xF4,pitch,state+0x38,2.4f,0.18f,0.17453294992446899414f,0);
        }
    } else {
        f_33FA90(m+0xF8,yaw,state+0x34,4000.0f,0.18f,0.20943955f,0);
        f_33FA90(m+0xF4,pitch,state+0x38,10000.0f,0.18f,0.3141593f,0);
    }
    f_346220(&direction,BF(state,0x30),BF(m,0xF8),-BF(m,0xF4));
    random.q = 0;
    random.v[0]=f_33BB18(-1.0f,1.0f);
    random.v[1]=f_33BB18(-1.0f,1.0f);
    random.v[2]=f_33BB18(-1.0f,1.0f);
    *(HomingUpdateQuad *)&temp = *(HomingUpdateQuad *)(&random);
    f_30F090(&temp,&temp,f_33BB18(0.1f,0.2f)*0.016666668f);
    f_30F090(&trailMotion,&direction,-f_33BB18(0.00166666682343930006f,0.016666668f));
    f_30ED98(&temp,&temp,&trailMotion);
    f_30F090(&trailMotion,&direction,f_33BB18(0,1)*BF(state,0x30));
    f_30ED98(&trailMotion,&trailMotion,m+0x10);
    particle=f_35BC18(&trailMotion,&temp,60,127,0x606060,3,40000,1000,1,-0.0002f,0);
    if(particle) { unsigned char b=*D_L04_001FF25C; particle[3]=0x44; particle[2]=b; }
    f_35BC18(m+0x10,&temp,6,127,0xB0B0B0,3,40000,1000,1,-0.0004f,0);
    random.q = 0; random.v[2]=0.02f;
    f_30F270(&random,&random,m+0xC0);
    f_3425D0(&random,&random,m+0xC0,f_33BBE0());
    f_357930(&trailMotion,&random,0x4F007FFF,0x1FFFFFFF,5,1,20000);
    qcopy(&oldPos,m+0x10);
    f_30EDB0(&temp,&direction,&acceleration);
    if(target && *(short *)(target+0xAA)==0xBBC) {
        f_30ED98(m+0x10,m+0x10,&direction);
        f_30EDC8(&direction,m+0x10,&targetPos);
        threshold=0.5f;
        if(BI(BP(target,0x68),0x744)) threshold=1;
        if(f_30EEB8(&direction,&direction)<threshold) { f_40ACC8(); m[0xBC]=2; }
    } else {
        if(BF(m,0x10)<0 || BF(m,0x14)<0 || BF(m,0x18)<0) goto destroy;
        collision=f_3F9ED0(m,state,m+0x10,&direction,&velocity,&normal,&hitPos);
    }
    if(!m[0xBC]) goto finish;
    f_382EA0(11,0,m,0xBBC);
    if(target && *(short *)(target+0xAA)!=0xBBC) {
        lifeA=f_301948(m+0x10,16,m,2.0f,0);
        *(HomingUpdateQuad *)&temp = *(HomingUpdateQuad *)(m+0x10);
        f_33E8C0(m,&temp,D_L04_001C3B40,lifeA,collision,0x30000,3,1,2,1,1);
    }
    cross[0]=0; cross[4]=-normal.v[2]; cross[8]=normal.v[1]; cross[12]=0;
    cross[1]=normal.v[2]; cross[5]=0; cross[9]=-normal.v[0]; cross[13]=0;
    cross[2]=-normal.v[1]; cross[6]=normal.v[0]; cross[10]=0; cross[14]=0;
    cross[3]=0; cross[7]=0; cross[11]=0; cross[15]=0;
    f_30F9E8(crossSquared,cross,cross);
    for(i=0;i<10;i++) {
        angle=f_33BBE0();
        f_30F958(matA,crossSquared,1-f_30F3A0(angle));
        f_30F958(matB,cross,f_30F3B8(angle));
        f_30FA30(matB,matB,matA);
        f_30F5A8(matA);
        f_30FA30(matC,matB,matA);
        f_30F298(&temp,&hitPos,matC);
        f_30F090(&temp,&temp,f_33BB18(8.5f,16.5f)*0.016666668f);
        temp.v[2]+=0.083333336f;
        f_30ED98(&temp,&temp,&velocity);
        f_356A70(m+0x10,&temp,0x4F007FFF,0x1F00007F,f_33BAC0(60,120),1,-1,-1,0,40000);
    }
    random.q = 0;
    random.v[0]=f_33BB18(-1,1); random.v[1]=f_33BB18(-1,1); random.v[2]=f_33BB18(-1,1);
    *(HomingUpdateQuad *)&temp = *(HomingUpdateQuad *)(&random);
    f_30EDC8(&cameraMotion,D_L04_001B9680,m+0x10);
    camDist=f_30EEF8(&cameraMotion);
    cameraMotion.v[2]+=camDist*0.5f;
    f_30F090(&temp,&temp,camDist*0.00333333364687860012f);
    f_30F090(&cameraMotion,&cameraMotion,(camDist+camDist)*0.016666668f);
    f_30ED98(&temp,&temp,&cameraMotion);
    f_30F128(&temp,&temp,0.16666667f);
    f_3B4EA0(m+0x10,&temp,f_33BAC0(60,90),0,0);
    if(camDist<6) count=f_30FF98(camDist)/2;
    else count=3;
    fade=0;
    if(camDist<7) fade=7-camDist;
    for(j=0;j<count;j++) {
        speed=f_33BB18(8,10)*0.016666668f;
        colorsA.colors=D_L04_001B0458; colorsB=D_L04_001B0470;
        speed-=fade*0.016666668f;
        {
            unsigned int *ca=colorsA.colors.v+f_33BA80(6), *cb=colorsB.v+f_33BA80(6);
            int la=f_33BAC0(15,20), lb=f_33BAC0(25,30);
            f_355C20(m+0x10,&velocity,*ca,*cb,la,lb,400000,speed,0,0);
        }
        {
            int la=f_33BAC0(5,10), lb=f_33BAC0(15,20);
            f_355C20(m+0x10,&velocity,0x7FFFFFFF,0xFFFFFF,la,lb,400000,speed*0.5f,0,0);
        }
    }
    for(j=0;j<10;j++) {
        colorsA.colors=D_L04_001B0458; colorsC=D_L04_001B0470;
        random.q = 0;
        random.v[0]=f_33BB18(-1,1); random.v[1]=f_33BB18(-1,1); random.v[2]=f_33BB18(-1,1);
        *(HomingUpdateQuad *)&colorsB = *(HomingUpdateQuad *)(&random);
        f_30F090(&colorsB,&colorsB,f_33BB18(0,3)*0.016666668f);
        {
            unsigned int *ca=colorsA.colors.v+f_33BA80(6), *cb=colorsC.v+f_33BA80(6);
            f_3557A8(m+0x10,&colorsB,*ca,*cb,f_33BAC0(20,35),200000);
        }
    }
    if(D_L04_001B200C<0.95f && camDist>9) {
        f_3B2BC0(m,m+0x10,&velocity,4,15,127,127,127,32);
        f_3B2BC0(m,m+0x10,&velocity,4,24,127,127,127,32);
    }
    f_3B2BC0(m,m+0x10,&velocity,4,20,127,127,0,48);
    f_3B2BC0(m,m+0x10,&velocity,2,19,255,255,255,32);
    direction.v[0]=0;
    m[0xBC]=0;
    {
        HomingCamera *shake=(HomingCamera *)D_L04_001B9540;
        if(camDist<20) shake->shakeA=0.4f-camDist*0.0175f;
        else shake->shakeA=0.050000012f;
    }
    camera=(HomingCamera *)D_L04_001B9540;
    camera->shakeTimeA=25; camera->shakeClockA=0;
    f_3C2B38(&settings,m+0x10,0,0);
    f_30EDC8(&random,m+0x10,camera->focus);
    if(f_30EEF8(&random)<64) {
        f_30EDC8(&colorsA.vectors[1],camera->focus,&colorsA);
        f_30F090(&colorsA.vectors[1],&colorsA.vectors[1],1);
        f_4356D0(m,m+0x10,&colorsA.vectors[1],5);
        camera->shakeB=0.1f; camera->shakeTimeB=90;
        camera->shakeA=0.2f; camera->shakeTimeA=60; camera->shakeClockA=0;
    }
destroy:
    f_3317E8(m);
    return;
finish:
    if(target && *(short *)(target+0xAA)==0xBBC) return;
    if(f_30E9D8(state+0x24) || !BP(state,0x20) || f_30EFA8(m+0x10,BP(state,0x20)+0x10)>260) f_3317E8(m);
}
#undef BF
#undef BI
#undef BP

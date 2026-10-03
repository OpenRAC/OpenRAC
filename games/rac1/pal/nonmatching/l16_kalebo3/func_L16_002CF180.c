/* NON_MATCHING func_L16_002CF180 -- src/overlays/l16_kalebo3/vendor_002A50F0.c
 * Best so far: SIZE ours 3248 / retail 3124, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * Cannot land as written (#define in a candidate): rewrite that in plain C first.
 * What the last attempts found:
 *   14. BYTES84: explicit entry offset produces exact target indexing; class temporary fixes all FX field stores. 
 *   15. BYTES78: established scale-before-vector API fixes particle call schedule. Typed resident state view and w
 *   16. BYTES70: waypoint-first coordinates fix all false-branch FPRs. Typed position view, word-addressed path st
 *   17. COMPILE: broad path token substitution touched resident symbol; corrected in next candidate.
 *   18. BYTES70 unchanged: word path, typed position/socket views did not alter bytes. Use pointer-typed effect ow
 *   19. BYTES70 unchanged: typed effect owner and early active-owner local fold identically. Scope path to the pos
 *   20. BYTES70 unchanged: path phase scope folds identically. Budget spent; best p15.c copied to best.c, not inte
 *   Remaining: path=s2/temporary-position=s5 versus retail s5/s2 (entry save order plus pointer uses); source path
 */
#include "common.h"
#define B(p,o) (*(unsigned char *)((char *)(p)+(o)))
#define H(p,o) (*(short *)((char *)(p)+(o)))
#define U(p,o) (*(unsigned short *)((char *)(p)+(o)))
#define W(p,o) (*(int *)((char *)(p)+(o)))
#define F(p,o) (*(float *)((char *)(p)+(o)))
typedef int L16PathQuad __attribute__((mode(TI)));
#define Q(p,o) (*(L16PathQuad *)((char *)(p)+(o)))
#define P(p,o) (*(char **)((char *)(p)+(o)))
typedef struct { char pad0[0xD0]; int target[2]; } L16AttackTargets;
extern char *D_L16_001B0C30[];
extern char *D_L16_001601AC_m __asm__("D_L16_001601AC") MACRO_ADDR;
extern char *D_L16_001742D8 MACRO_ADDR;
extern char D_0013E633[];
typedef struct { char pad0[0x1C0]; int busy; char pad1C4[0x1EBC]; char *moby; } L16AttackPlayer;
typedef struct { char pad0[0xE1D]; L16AttackPlayer player; } L16AttackRoot;
extern L16AttackRoot D_0013E633_path __asm__("D_0013E633") MACRO_ADDR;
extern float D_L16_0015F660[4] MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern short D_L16_00161A88, D_L16_00161A8C, D_L16_00161A90, D_L16_00161A94;
extern void func_L16_002CFDB8(void *);
extern int func_002140B0(int);
extern float func_00214158(void);
extern int func_L16_002D7178(int, void *);
extern char *func_0020D348_m(int) __asm__("func_0020D348");
extern void func_L02_00265E58(void *);
extern int func_001F9850(int);
extern int func_001F9908(int *);
extern float func_L00_001FF860(float, float);
extern void func_00213DE0(void *, int, int, int);
extern float func_00214358(void *, int, float);
extern void func_L00_00250800(void *, int, void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9BD8(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern float func_L00_0025BC48(void *, void *, void *, float, float);
extern unsigned char *func_L16_002C75D0(int, void *, float *, int, float);
extern float func_0020D830(void *);
extern void func_L00_002607A8(void *, float);
extern void func_L00_0025A8E8(int, float, void *, int, float, float, int, int, int);
extern int func_0022ED80_i(int, int, void *) __asm__("func_0022ED80");
extern float func_001F9CB8(void *);
extern void func_L00_00260FB0(void *, void *, int, int, void *, int, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_L02_00265E88(void *, void *, void *, float);
extern void func_L00_0025A8C0(void *, void *, int, float, void *);
extern void func_L02_002661E8(void *, void *, void *);
extern void *func_L00_00265050(void *, int, void *, void *, int, int, void *, void *, float, void *);
extern void func_L00_00260108(void *, void *, int, float, float);
extern void func_L00_002584A8(void *, int, int);
extern void func_0020D678(void *);

/* Runs the path, throwing and particle attacks for the three enemy variants. */
void func_L16_002CF180(unsigned char *m) {
    float vectors[3][4];
    unsigned char fx[0x30];
    char *d = P(m, 0x78);
    float speed, gravity, x, y, angle;
    int next;
    func_L16_002CFDB8(m);
    { char *path = D_L16_001B0C30[W(d, 0xC4)];
    func_L00_00264B40(m, 2, d + 0x140, 2.5f);
    switch (m[0x20]) {
    case 0:
        B(d,0x2E)=1;
        if(W(d,0xC8)>0) H(m,0xB4)/=W(d,0xC8);
        m[0x20]=1; m[0x31]=0; U(m,0x34)|=1;
        if(func_002140B0(2)) U(m,0x34)|=0x8000;
        B(d,0x28)=1; H(d,0x24)=2; F(d,0x20)=2.0f;
        F(d,0x11C)=func_00214158();
        W(d,0xF8)=func_L16_002D7178(W(d,0xCC),m+0x10);
        qcopy(d+0xE0,m+0x10);
        switch(W(d,0xC0)) {
        case 0: P(d,0x104)=func_0020D348_m(0x35F); break;
        case 1: P(d,0x104)=func_0020D348_m(0x374); break;
        case 2: P(d,0x104)=func_0020D348_m(0x36C); func_L02_00265E58(d+0x120); break;
        }
        if(P(d,0x104)) {
            U(P(d,0x104),0x34)|=0x100;
            H(P(d,0x104),0x32)=0x40;
            B(P(d,0x104),0x30)=0;
            B(P(d,0x104),0x31)=1;
        }
        break;
    case 1: break;
    case 2:
        func_L16_002D7248(W(d,0xF8));
        Q(vectors[0],0)=Q(path,0x10);
        if(func_L16_002D00E8(m,vectors[0],F(m,0x48))) {
            W(d,0xF0)=func_001F9850(30);
            W(d,0xDC)=1; W(d,0x108)=1;
            if(W(d,0xC0)==0) m[0x20]=3;
            else if(W(d,0xC0)==1) m[0x20]=5;
            else m[0x20]=7;
        }
        break;
    case 3:
        if(func_001F9908((int *)(d+0xF0))) {
            Q(vectors[0],0)=Q(path+W(d,0xDC)*16,16);
            if(func_L16_002D00E8(m,vectors[0],func_L00_001FF860(F(path+W(d,0xDC)*16,16)-F(m,16),F(path+W(d,0xDC)*16,20)-F(m,20)))) {
                W(d,0xF0)=func_001F9850(30); m[0x20]=4;
                if(m[0x53]!=21) func_00213DE0(m,21,0,func_001F9850(30));
                if(func_002140B0(2)) goto sound;
                W(d,0xB4)=2;
            }
        }
        break;
    case 4:
        if(W(d,0xB4)==2) {
            int offset=((L16AttackTargets *)d)->target[W(d,0xDC)]<<7;
            char *entry=(char *)(offset+(int)D_L16_001601AC_m);
            qcopy(vectors[0],entry+0x30);
        }
        else qcopy(vectors[0],d+0x70);
        if(func_00215B18(m,15.0f) && P(d,0x104)) {
            speed=F(&D_L16_00161A8C,0)*D_0015EE6C;
            gravity=D_0015EE70*10.0f;
            vectors[0][2]=func_00214358(vectors[0],0,0.5f);
            func_L00_00250800(P(d,0x104),0,vectors[1]);
            func_001F9BF0(vectors[2],vectors[0],vectors[1]); vectors[2][2]=0.0f;
            func_L00_001FF4B0(vectors[2],vectors[2],speed);
            vectors[2][2]=func_L00_0025BC48(vectors[1],vectors[0],0,speed,-gravity);
            func_L16_002C75D0((int)m,vectors[1],vectors[2],func_001F9850(300),gravity);
        } else if(m[0x70]&2) {
            W(d,0xDC)=(W(d,0xDC)+1)&1; m[0x20]=3;
            if(m[0x53]!=23) func_00213DE0(m,23,0,func_001F9850(30));
        }
        if(func_0020D830(m)<15.0f) {
            x=F(m,16); y=F(m,20);
            x=vectors[0][0]-x; y=vectors[0][1]-y;
            Q(vectors[1],0)=Q(m,16);
        } else {
            int node=(W(d,0xDC)+1)&1;
            Q(vectors[1],0)=Q(m,16);
            x=F(path+node*16,16); y=F(path+node*16,20);
            x-=F(m,16); y-=F(m,20);
        }
        func_L16_002D00E8(m,vectors[1],func_L00_001FF860(x,y));
        break;
    case 5:
        if(m[0x53]==10) {
            if(m[0x70]&2) func_00213DE0(m,11,0,1);
        } else {
            char *pos=(char *)m+16;
            func_001F9BF0(vectors[0],path+(W(d,0xDC)*16+16),pos);
            func_L00_002607A8(vectors[0],F(&D_L16_00161A90,0)*D_0015EE6C);
            func_001F9BD8(pos,pos,vectors[0]);
            func_L00_00250800(m,W(&D_L16_00161A88,0),vectors[1]);
            func_L00_0025A8E8((int)m,0.333f,vectors[1],1,1.0f,1.0f,0,1,0);
            { L16AttackPlayer *g=(L16AttackPlayer *)((char *)&D_0013E633_path+0xE1D);
            if(D_L16_001742D8==g->moby && g->busy==0) func_0022ED80_i(6,0,m); }
            if(func_001F9CB8(vectors[0])<0.0001f) {
                if(m[0x53]!=9) func_00213DE0(m,9,0,func_001F9850(20));
                m[0x20]=6;
                W(d,0xF0)=func_001F9850(60);
                if(W(d,0xDC)==0) W(d,0x108)=1;
                else if(W(d,0xDC)==W(path,0)-1) W(d,0x108)=-1;
            }
        }
        break;
    case 6: {
        float *yaw=(float *)(m+0x48);
        next=W(d,0xDC)+W(d,0x108);
        angle=func_L00_001FF860(F(path+next*16,16)-F(m,16),F(path+next*16,20)-F(m,20));
        func_L00_0025CE58(yaw,d+0xFC,angle,D_0015EE70*12.566371f,D_0015EE70*12.566371f,D_0015EE6C*25.132742f);
        if(func_001F9908((int *)(d+0xF0))) {
            W(d,0xDC)=next;
            if(m[0x53]!=10) func_00213DE0(m,10,0,func_001F9850(10));
            m[0x20]=5;
        }
        break;
    }
    case 7:
        if(func_001F9908((int *)(d+0xF0))) {
            Q(vectors[0],0)=Q(path+W(d,0xDC)*16,16);
            if(func_L16_002D00E8(m,vectors[0],F(m,0x48))) {
                W(d,0xF0)=func_001F9850(30); m[0x20]=8;
                if(m[0x53]!=1) func_00213DE0(m,1,0,func_001F9850(30));
                if(func_002140B0(2)) {
                sound: {
                    char *p=D_L16_001B0C30[W(d,0xD8)];
                    func_L00_00260FB0(m,d+0x70,0,0,p+16,W(p,0),12.0f);
                }
                } else W(d,0xB4)=2;
            }
        }
        break;
    case 8:
        if(W(d,0xB4)!=2) {
            Q(vectors[0],0)=Q(m,16);
            func_L16_002D00E8(m,vectors[0],func_L00_001FF860(F(d,0x70)-F(m,16),F(d,0x74)-F(m,20)));
        }
        if(P(d,0x104)) {
            char *slots;
            func_L00_00250800(P(d,0x104),0,vectors[0]);
            slots=d+0x120;
            vectors[1][0]=func_001F9F90(func_L00_001FF860(vectors[0][0]-F(m,16),vectors[0][1]-F(m,20)));
            vectors[1][1]=func_001F9FA8(func_L00_001FF860(vectors[0][0]-F(m,16),vectors[0][1]-F(m,20)));
            vectors[1][2]=0.0f;
            func_L02_00265E88(slots,vectors[0],vectors[1],F(&D_L16_00161A94,0));
            vectors[1][3]=5627.925f; vectors[1][2]=1.0f;
            func_L00_0025A8C0(fx,m,0x10001,1.0f,vectors[1]);
            { unsigned short cls=U(m,0xA6);
            fx[0x18]=5; fx[0x19]=1; U(fx,0x1A)=cls; }
            func_L02_002661E8(slots,m,fx);
        }
        if(m[0x70]&2) {
            W(d,0xDC)=(W(d,0xDC)+1)&1; m[0x20]=7;
            if(m[0x53]!=3) func_00213DE0(m,3,0,func_001F9850(30));
        }
        break;
    case 9:
        Q(vectors[0],0)=Q(path+W(d,0xDC)*16,16);
        func_L16_002D00E8(m,vectors[0],F(d,0x10C));
        if(m[0x70]&2) {
            m[0x20]=B(d,0x110);
            if(m[0x53]!=W(d,0x114)) func_00213DE0(m,W(d,0x114),0,func_001F9850(20));
        }
        break;
    case 10: {
        char *pos=(char *)m+16;
        char *rot=(char *)m+0x40;
        int timer;
        float burst;
        F(d,0x20)=2.0f; timer=func_001F9850(60);
        burst=D_0015EE70*12.0f; W(d,0xF0)=timer;
        func_L00_00265050(m,0x655,pos,rot,0,0,D_L16_0015F660,D_L16_0015F660,burst,D_L16_0015F660);
        func_L00_00265050(m,0x656,pos,rot,0,0,D_L16_0015F660,D_L16_0015F660,burst,D_L16_0015F660);
        func_L00_00265050(m,0x657,pos,rot,0,0,D_L16_0015F660,D_L16_0015F660,burst,D_L16_0015F660);
        qcopy(vectors[0],pos); vectors[0][2]+=1.2f;
        func_L00_00260108(m,vectors[0],-1,0.5f,10.0f);
        func_L00_002584A8(m,0,-1); func_0022ED80_i(7,0,m);
        W(d,0xC8)--;
        if(W(d,0xC8)!=-1) {
            m[0x20]=11; qcopy(pos,d+0xE0); W(d,0xF0)=func_001F9850(60);
        } else {
            if(P(d,0x104)) func_0020D678(P(d,0x104));
            func_0020D678(m);
        }
        break;
    }
    case 11:
        if(func_001F9908((int *)(d+0xF0))) {
            W(d,0xFC)=0; m[0x20]=2;
            if(m[0x53]!=9) func_00213DE0(m,9,0,func_001F9850(10));
            F(d,0x20)=2.0f; U(m,0x34)|=0x1000;
        }
        F(m,0x18)=F(d,0xE8)+D_0015EE6C*3.0f*(float)W(d,0xF0);
        break;
    }
    }
}

/* GPT-6.1 Sol high, worker v0912, 29 October 2026.
 * Pinned game EE-GCC, -O2 -G0 -ffunction-sections: 856/856 bytes,
 * 0/28 USA v1.01 placements; 19 non-relocation instruction words differ.
 * Improved from 888 bytes: an explicit list test/back edge prevents the
 * duplicated initial checks; genuine floating-point control transfers retain
 * all selector reloads, without volatile. Scratch order first, second, right,
 * left recovers all four retail vector slots and the complete grid loop.
 * Residual: nine list-preheader words are allocated/scheduled differently;
 * ten selector-use words swap v0/v1. Named field views recover state-first
 * address addition. A byte list index, a declared volatile selector, and
 * integer-address table indexing do not change the remaining instructions.
 * Next: independently evidenced list-base/selector temporary lifetimes.
 * No linked build or other-release check was run for this revision.
 * Required declarations follow. This is not a match.
 */
typedef unsigned char u8;
typedef int s32;
typedef unsigned int u32;
typedef int ParticleWord __attribute__((mode(TI)));
typedef union { ParticleWord word; float f[4]; } ParticleVector;
typedef struct { float x,y,z,w; } V4_F213135aa;
typedef struct { char pad00[0x14]; u32 selector; } SurfaceControlSettings;
typedef struct { char pad00[0x50]; float x[4], y[4], z; } SurfaceGridControl;
extern void func_00282CC0(void *, void *, void *);
extern void func_00282CF0(void *, void *, void *);
extern void func_00282D30(void *, void *, float);

extern unsigned short *D_001EE5A0[];
extern int D_001B1B78 __attribute__((sda));
extern char *D_001B1B5C __attribute__((sda));
extern V4_F213135aa ref_00303210_00[];
extern int func_002A8508(int, int);

/* Regenerate the 8 by 8 mesh from four surface control points. */
void func_00303210(char *object)
{
    char *state=*(char **)(object+0x68);
    unsigned short *entry;
    char *control;
    SurfaceControlSettings *settings;
    ParticleVector first, second, right, left;
    float columnWeight;
    int column, row, surface;
    u8 list;
    if (func_002A8508((u8)object[0x21],-1)==5) {
        list=(u8)object[0x21];
        entry=D_001EE5A0[list];
        goto checkList;
listBody:
        control=(char *)((u32)D_001B1B5C+(*entry&0x7fff)*0x100);
        if (!control) goto doneList;
        if (*(short *)(control+0xaa)==0x1130) {
            settings=*(SurfaceControlSettings **)(control+0x68);
            if (settings->selector>3) settings->selector=0;
            ((SurfaceGridControl *)state)->x[settings->selector]=*(float *)(control+0x10);
            ((SurfaceGridControl *)state)->y[settings->selector]=*(float *)(control+0x14);
            ++entry;
            if (!settings->selector) *(float *)(state+0x70)=*(float *)(control+0x18);
        } else ++entry;
checkList:
        list=(u8)object[0x21];
        if (list==0xff || D_001B1B78<(int)list || !entry) goto doneList;
        if ((short)entry[-1]>=0 || entry==D_001EE5A0[list]) goto listBody;
doneList:
        ;
    } else {
        *(int *)(state+0x50)=0; *(int *)(state+0x60)=0;
        *(int *)(state+0x54)=0; *(int *)(state+0x64)=0;
        *(int *)(state+0x58)=0; *(int *)(state+0x68)=0;
        *(int *)(state+0x5c)=0; *(int *)(state+0x6c)=0;
        *(int *)(state+0x70)=0;
    }
    surface=*(int *)(state+0x40);
    for (row=0; row<8; ++row) {
        for (column=0; column<8; ++column) {
            first.f[0]=*(float *)(state+0x54)-*(float *)(state+0x50);
            first.f[1]=*(float *)(state+0x64)-*(float *)(state+0x60);
            first.f[2]=0; first.f[3]=1.0f;
            second.f[0]=*(float *)(state+0x58)-*(float *)(state+0x5c);
            second.f[1]=*(float *)(state+0x68)-*(float *)(state+0x6c);
            second.f[2]=0; second.f[3]=1.0f;
            columnWeight=(float)column*0.14285715f;
            func_00282D30(&first,&first,columnWeight);
            func_00282D30(&second,&second,columnWeight);
            left.f[0]=*(float *)(state+0x50); left.f[1]=*(float *)(state+0x60);
            left.f[2]=0; left.f[3]=1.0f;
            func_00282CC0(&left,&left,&first);
            right.f[0]=*(float *)(state+0x5c); right.f[1]=*(float *)(state+0x6c);
            right.f[2]=0; right.f[3]=1.0f;
            func_00282CC0(&right,&right,&second);
            func_00282CF0(&right,&right,&left);
            func_00282D30(&right,&right,(float)row*0.14285715f);
            func_00282CC0(&left,&left,&right);
            ref_00303210_00[surface*64+row*8+column].x=left.f[0];
            ref_00303210_00[surface*64+row*8+column].y=left.f[1];
            ref_00303210_00[surface*64+row*8+column].z=*(float *)(state+0x70);
        }
    }
}

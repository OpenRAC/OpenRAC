/* GPT-6.1 Sol high, 29 October 2026. Retained near miss; not linked.
 * Current pinned compiler: -O2 -G8 -ffunction-sections, dedicated G8 unit.
 * tools/check.py: 1040/1040 bytes, 0/1 USA v1.01 placements; 29 instruction
 * differences after masking relocation fields. Other releases were not checked.
 * Improvements: a measured 0x100-byte class-record view reproduces the first
 * two flag/enable stores' shared base; independent loop variables, explicit
 * phase/weight bases and conditional weight-row locals recover retail's saved
 * registers and almost all of its final phase loop.
 * Remaining: copy setup and countdown at +0x8C..+0xDC; case-4/5 finish scheduling
 * at +0x1D4..+0x1F0; phase base/counter initialization order at +0x354/+0x358.
 * The count header is separate from the copied rows, so qcopy_nc's independent
 * surrounding-memory requirement holds. No access qualifiers or barriers added.
 * Negative trials: raw TImode copy still counts down; -fno-strength-reduce
 * restores an ascending comparison but changes state allocation and gives
 * 1048 bytes; -fno-rerun-loop-opt does not fix the copy. A shared finishing
 * base fixes branch polarity but changes the flag-pointer registers; an
 * explicit class-index snapshot does not recover those registers.
 * Reopen with an independently measured ascending-copy-loop source witness
 * or a finishing-arm lifetime hypothesis. All required declarations follow.
 */
#include "common.h"

#define S16(p,o) (*(short *)((p)+(o)))
#define U16(p,o) (*(unsigned short *)((p)+(o)))
#define I32(p,o) (*(int *)((p)+(o)))
#define F32(p,o) (*(float *)((p)+(o)))
typedef struct { u8 pad0[0x10]; float position[4]; u8 pad20[0x11]; u8 enabled; u16 mask, flags; u8 rest[0xCA]; } JobaRecord461E8;
extern JobaRecord461E8 *D_L11_001B359C __attribute__((section(".sdata")));
extern u8 *D_L11_001FF580[];
extern float D_L11_001B187C __attribute__((sda)), D_L11_001B1880 __attribute__((sda));
extern float D_L11_001B1884 __attribute__((sda)), D_L11_001B1888 __attribute__((sda));
extern void func_00283360(void *);
extern float func_002AA2E8(float *,float,float);
extern float func_00282C48(float);
extern void func_00282CF0(void *,void *,void *);
extern void func_00282D30(void *,void *,float);
extern void func_00282CC0(void *,void *,void *);
extern void func_00282F20(void *,void *,float);
extern float func_002A7878(float,float);
void func_L11_004461E8(u8 *object)
{
    u8 *state = *(u8 **)(object+0x68);
    float previous = F32(state,0x1bc);
    float vector[4] __attribute__((aligned(16)));
    switch (S16(state,0x1fa)) {
    case 1: {
        int i;
        u8 *dst, *src;
        func_00283360((u8 *)&D_L11_001B359C[I32(state,0x1b8)]+0xc0);
        previous=1.0f;
        dst=D_L11_001FF580[I32(state,0x1c8)];
        src=D_L11_001FF580[I32(state,0x1cc)];
        {
            u8 *out = dst + 16;
            u8 *in = src + 16;
            for(i=0;i<I32(dst,0);i++,out+=16,in+=16) qcopy_nc(out,in);
        }
        D_L11_001B359C[I32(state,0x1b8)].flags&=0xfffe;
        D_L11_001B359C[I32(state,0x1b8)].enabled=1;
        D_L11_001B359C[I32(state,0x1b8)].mask=255;
        F32(state,0x1bc)=D_L11_001B187C;
        F32(state,0xd4)=0.5f;
        S16(state,0x1fa)=2;
        I32(state,0xd0)=0;
        /* Initialization deliberately advances the new animation immediately. */
    }
    case 2:
        func_002AA2E8((float *)(state+0x1bc),D_L11_001B1880,D_L11_001B1884);
        if (func_00282C48(F32(state,0x1bc)-D_L11_001B1880)<0.01f) S16(state,0x1fa)=3;
        break;
    case 4:
        func_002AA2E8((float *)(state+0x1bc),D_L11_001B187C,D_L11_001B1884);
        if (!(func_00282C48(F32(state,0x1bc)-D_L11_001B187C)<0.01f)) break;
        goto finish;
    case 5:
        F32(state,0x1bc)=D_L11_001B187C;
finish:
        D_L11_001B359C[I32(state,0x1b8)].flags|=1;
        D_L11_001B359C[I32(state,0x1b8)].enabled=0;
        S16(state,0x1fa)=0;
        break;
    }
    if (previous != F32(state,0x1bc)) {
        int i;
        u8 *dst, *vertex;
        dst=D_L11_001FF580[I32(state,0x1c8)];
        for (i=0;i<I32(dst,0);i++) {
            vertex=dst+16+i*16;
            func_00282CF0(vector,vertex,(u8 *)&D_L11_001B359C[I32(state,0x1b8)]+16);
            func_00282D30(vector,vector,F32(state,0x1bc)/previous);
            func_00282CC0(vertex,(u8 *)&D_L11_001B359C[I32(state,0x1b8)]+16,vector);
            F32(vertex,8)=F32((u8 *)&D_L11_001B359C[I32(state,0x1b8)],0x18);
        }
        func_00282F20((u8 *)&D_L11_001B359C[I32(state,0x1b8)]+0xc0,(u8 *)&D_L11_001B359C[I32(state,0x1b8)]+0xc0,F32(state,0x1bc));
        func_00282F20((u8 *)&D_L11_001B359C[I32(state,0x1b8)]+0xd0,(u8 *)&D_L11_001B359C[I32(state,0x1b8)]+0xd0,F32(state,0x1bc));
    }
    if (S16(state,0x1fa)==2) {
        int i;
        float *phase = (float *)(state + 0xD0);
        float *weight = (float *)(state + 0xD8);
        for(i=0;i<2;i++) {
            float *phase_row = phase + i;
            *phase_row+=D_L11_001B1888*0.01666666753590107f;
            if (*phase_row>1.0f) {
                float random;
                float *weight_row;
                float *opposite;
                *phase_row-=1.0f;
                random = func_002A7878(0.3330000042915344f,0.6660000085830688f);
                weight_row = weight + i;
                opposite = weight + ((i+1)&1);
                *weight_row = *opposite + random;
                if(*weight_row>1.0f) *weight_row-=1.0f;
            }
        }
    }
}

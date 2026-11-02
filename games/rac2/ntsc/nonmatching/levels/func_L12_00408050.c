/* Near miss (mid-16): matches except the table address D_L12_001A8F20 is re-materialised (lui/addiu) before each func_0033E598 call in retail while ours CSEs it into $s0. Tried array/Quadword/pointer-var declarations, -mno-split-addresses, -fno-gcse.
   Needs the unit flags: -O2 -G8 -ffunction-sections -frac2-qcopy -frac2-qcopy-barrier (unit src/levels/shared/L12_00408050-qcopy.c) */
typedef int Quadword __attribute__((mode(TI)));

extern unsigned char *func_00313CA0(int);
extern float func_0031E338(float, float);
extern void func_002F21A8(void *, void *);
extern void func_0033E598(void *, int, int, int, int, void *, int, float, float, float);
extern float D_L12_001B15B0 __attribute__((sda));
extern int D_L12_001B1520 __attribute__((sda));
extern int D_L12_001B1524 __attribute__((sda));
extern Quadword D_L12_001A8F20;

unsigned char *func_L12_00408050(unsigned char *a0, Quadword *a1, Quadword *a2, float speed)
{
    unsigned char *object = func_00313CA0(0xEA5);
    unsigned char *state;
    float scale;
    Quadword *dst;

    if (object != 0) {
        object[0x30] = 0xFF;
        *(short *)(object + 0x32) = 0xFF;
        object[0x31] = 1;
        object[0x20] = 0;
        *(long *)(object + 0x38) = *(long *)(a0 + 0x38);
        *(Quadword *)(object + 0x10) = *a2;
        state = *(unsigned char **)(object + 0x68);
        *(Quadword *)state = *a1;
        *(float *)(state + 0x14) = speed;
        *(float *)(state + 0x20) = 0.15000000596046448f;
        *(unsigned char **)(state + 0x10) = a0;
        *(unsigned short *)(object + 0x34) |= 0x208;
        dst = (Quadword *)(object + 0xF0);
        *dst = *(Quadword *)(a0 + 0xF0);
        func_002F21A8(object + 0xC0, dst);
        object[0x23] = 0x80;
        *(float *)(object + 0x2C) = D_L12_001B15B0;
        *(unsigned short *)(object + 0x34) |= 0x100;
        *(int *)(state + 0x24) = 0;
        scale = func_0031E338(8.0f, 10.0f);
        func_0033E598(object + 0x10, 0xF, D_L12_001B1520, 0, 10, &D_L12_001A8F20, 0, scale * 0.30000001192092896f,
                      scale, 0.0f);
        func_0033E598(object + 0x10, 0xF, D_L12_001B1524, 1, -10, &D_L12_001A8F20, 0, scale * 0.03999999910593033f,
                      scale * 0.4000000059604645f, 0.0f);
    }
    return object;
}

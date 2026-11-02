typedef int Rac2TI128 __attribute__((mode(TI)));

extern float func_002B04E0(char *a0, float *src, char *third);
extern void func_002AAA00(void *out, float f12, float f13, float f14, float f15, char *in, int a2);
extern void func_002833F0(void *out, void *in);
extern void func_00283738(char *a0, char *a1, void *a2);
extern float func_00282C48(float f);

float func_0031AB18(char *a0, char *a1, char *a2, char *a3)
{
    char scratch[0x50];
    float f;

    *(Rac2TI128 *)scratch = *(Rac2TI128 *)a2;
    f = func_002B04E0(a0, (float *)scratch, a3);
    *(int *)(scratch + 0x40) = 0;
    func_002AAA00(scratch + 0x40, f, *(float *)(a1 + 0x14), *(float *)(a1 + 0x18), 3.14159274f, a1 + 0xB4, 0);
    *(Rac2TI128 *)scratch = 0;
    *(float *)(scratch + 8) = *(float *)(a1 + 0xB4);
    func_002833F0(scratch + 0x10, scratch);
    func_00283738(a0 + 0xC0, a0 + 0xC0, scratch + 0x10);
    return func_00282C48(f) - func_00282C48(*(float *)(a1 + 0xB4));
}

typedef struct { char pad0[0x268]; float f268; float f26C; int f270; } S;
extern S ref_L02_002DCEA0_00;
void func_L02_002DCEA0(int a0, float f) {
    S *s = &ref_L02_002DCEA0_00;
    if (s->f26C == 0.0f) {
        s->f270 = a0;
        s->f26C = 1.0f;
        s->f268 = 1.0f / f;
    }
}

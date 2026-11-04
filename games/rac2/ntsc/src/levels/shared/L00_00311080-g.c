extern void func_L00_00310D30(unsigned char *, unsigned char *);

void func_L00_00311080(unsigned char *a0, unsigned char *a1, int a2, int a3, int t0, int t1)
{
    a1[0x22] = a0[0x43];
    a1[0x20] = a0[0x41];
    a1[0x23] = a2;
    a1[0x21] = a3;
    *(int *)(a1 + 0x24) = 0;
    *(float *)(a1 + 0x28) = 1.0f;
    *(float *)(a1 + 0x2C) = 1.0f / (float)t0;
    if (t1 != 0)
        func_L00_00310D30(a0, a1);
}

typedef int TI128 __attribute__((mode(TI)));

extern char *D_L06_001FE500[];
extern int func_002A8AF0(float *, float *, int);
extern void func_L02_0031E9E8(void *, float);
extern void func_00282CC0(void *, void *, void *);

int func_L06_0041FC58(char *a0, char *a1)
{
    char *src = a0 + 0x10;
    char *st = a1;
    int ret = 0;

    if (func_002A8AF0((float *)src,
                      (float *)((char *)D_L06_001FE500[*(int *)(st + 0x230)] + 0x10),
                      *(int *)D_L06_001FE500[*(int *)(st + 0x230)]) == 0) {
        *(TI128 *)(st + 0x5B0) = *(TI128 *)(st + 0x460);
        *(float *)(st + 0x5BC) = 1.0f;
        return 1;
    }
    {
        char *v = st + 0x5B0;

        func_L02_0031E9E8(v, 7.0f);
        func_00282CC0(v, v, src);
        *(int *)(st + 0x5BC) = 0;
        if (func_002A8AF0((float *)v,
                          (float *)((char *)D_L06_001FE500[*(int *)(st + 0x230)] + 0x10),
                          *(int *)D_L06_001FE500[*(int *)(st + 0x230)]) != 0) {
            *(float *)(st + 0x5BC) = 1.0f;
            ret = 1;
        } else {
            *(TI128 *)v = *(TI128 *)src;
        }
    }
    return ret;
}

/* func_00335798 in its own unit (flags in config/cflags.txt: -O2 -G8 -ffunction-sections -frac2-qcopy -frac2-qcopy-barrier). */
typedef int TI128 __attribute__((mode(TI)));

extern void *func_00270280(int);
extern void func_00270598(void *, void *, int);
extern int D_001B1504;
extern char D_001B5340[];

void func_00335798(TI128 *p0, TI128 *p1, void *p2)
{
    TI128 v[4];
    char *obj = (char *)func_00270280(0x1B);
    char *table = (char *)D_001B1504;
    short idx = *(short *)(obj + 0x84);
    char *e = *(char **)(table + idx * 32 + 0x1C);
    char *c;
    int one = 1;

    *(unsigned char *)(e + 0x1D) = one;
    *(void **)(e + 0x20) = p2;
    v[0] = *p0;
    v[1] = *p1;
    c = (char *)D_001B5340;
    v[2] = *(TI128 *)c;
    v[3] = *(TI128 *)(c + 0x10);
    func_00270598(obj, &v[3], one);
}

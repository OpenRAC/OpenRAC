/* Carried by port/tools/versionmap/carry_functions.py from the decompilation of Ratchet & Clank (PAL), src/core/0012A2F0.c (games/rac1): func_0012A558, matched C there.
 * Kind `same` in the version map (../../README.md): the two versions have the same instructions for it.
 * Names and addresses are this version's. Functional C, not matched C. */

extern void func_001190F8(int);
extern void func_001187D0(int, int);
extern int func_0011D660(void);
extern void func_0011D6A8(void);
extern void _Error_a_c12a418(void *, void *) __asm__("func_0012C328");
extern void func_0012BB38();
extern int add_dmac_handler2_r_c12a418(int, void *, int, void *) __asm__("func_001187C0");
extern void enable_dmac_atomic_r_c12a418(int) __asm__("func_00119160");
extern void do_csc2_dma_handler_r_c12a418(void) __asm__("func_0012A2D8");
extern char D_001539B0[];
typedef struct {
    volatile int done;
    int err;
    int size;
    int next;
    int total;
} CscRec_2A558_c12a418;
void _doCSC2(void *arg0, unsigned int arg1, int arg2) {
    char *p = (char *)arg0;
    int event[8];
    CscRec_2A558_c12a418 rec;
    CscRec_2A558_c12a418 *rp;
    int h;
    int r;
    rec.total = arg2 / 0x3FF + 1;
    rec.size = arg2;
    rec.next = (arg1 + 0xFFC00) & 0x0FFFFFFF;
    rec.err = 0;
    rec.done = 0;
    rp = &rec;
    while (*(volatile int *)0x10002010 < 0) {
    }
    h = add_dmac_handler2_r_c12a418(3, (void *)do_csc2_dma_handler_r_c12a418, 0, rp);
    *(volatile unsigned int *)0x1000E010 = 8;
    enable_dmac_atomic_r_c12a418(3);
    r = func_0011D660();
    {
        volatile unsigned int *madr = (volatile unsigned int *)0x1000B010;
        volatile unsigned int *qwc = (volatile unsigned int *)0x1000B020;
        volatile unsigned int *chcr = (volatile unsigned int *)0x1000B000;
        *madr = arg1 & 0x0FFFFFFF;
        *qwc = 0xFFC0;
        *chcr = 0x100;
    }
    if (r != 0) {
        func_0011D6A8();
    }
    *(volatile unsigned int *)0x10002000 = 0x700003FF;
    event[0] = 4;
    func_0012BB38(*(void **)(p + 0x858), event);
    while (rec.done < rec.total) {
    }
    if (rec.err != 0) {
        _Error_a_c12a418(p, D_001539B0);
    }
    while (*(volatile int *)0x10002010 < 0) {
    }
    func_001190F8(3);
    func_001187D0(3, h);
}
extern void _doCSC2(void *, unsigned int, int);

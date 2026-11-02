#include "include_asm.h"

#ifndef RAC2_T_V4_F6A70B950
#define RAC2_T_V4_F6A70B950
typedef int V4_F6a70b950 __attribute__((mode(TI)));
#endif

extern void func_0029FC20(char *, int, char *);

extern void func_00283678(void *, void *);

extern void func_002A10E8(void *);

extern void func_L00_002AA200(void *, void *, int, int, void *);

extern char ref_L00_002D5000_00[];

extern char ref_L00_002D5000_01[];

/* Fills a 0x40 stack block through the engine, copies its last 16 bytes into the
   record at +0x10, hands the record's +0xC0 and then the record itself to two more
   setters, queues one five-argument request and stamps four resident words. */
void func_L00_002D5000(char *a0, char *a1)
{
    char block[0x40];
    V4_F6a70b950 *dst;
    V4_F6a70b950 *src;

    func_0029FC20(a0, 4, block);
    dst = (V4_F6a70b950 *)(a1 + 0x10);
    src = (V4_F6a70b950 *)(block + 0x30);
    *dst = *src;
    func_00283678(a1 + 0xC0, block);
    func_002A10E8(a1);
    func_L00_002AA200(ref_L00_002D5000_00, ref_L00_002D5000_01,
                *(int *)(a1 + 0x24), 0, a0);
    *(void **)(a1 + 0x58) = ref_L00_002D5000_01;
    *(void **)(a1 + 0x5C) = ref_L00_002D5000_01;
    *(int *)(a1 + 0x44) = 0;
    *(int *)(a1 + 0x40) = 0;
}

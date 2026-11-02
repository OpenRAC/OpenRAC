/* func_0033B910: draw the 21-row menu panel and its frame lines. */
#include "common.h"

#ifndef RAC2_T_ENTRY_F624A0727
#define RAC2_T_ENTRY_F624A0727
typedef struct { char pad[8]; short n; void (*fn)(char *); } ENTRY_F624a0727;
#endif

typedef struct {
    int x;
    int y;
    int text;
    float scale;
} Row_0033B910;

extern void func_00336320(char *);
extern void func_00336A78(char *);
extern void func_00336828(char *);
extern void func_00335E38(void *, float, float, float, float);
extern void func_00336920(char *, int, int);
extern void func_00335F88(void *, float, float, float, float);
extern int *func_00335E20(char *);
extern float *func_00335E10(char *);
extern void func_0028FAB8(int, int, int, int, unsigned long long, int);
extern Row_0033B910 D_00265140[];
extern unsigned char D_001A7BC8[] MACRO_ADDR;
extern unsigned char D_001A7BE8[] MACRO_ADDR;
extern char D_001C4FA0[];

static __inline__ void SetColor_0033B910(char *w, int c)
{
    *func_00335E20(w) = c;
}

void func_0033B910(char *obj)
{
    int i;
    char *base;

    func_00336320(obj);
    func_00336320(obj + 0x4C);
    func_00336320(obj + 0x98);
    func_00336A78(obj + 0x1D8);
    func_00336A78(obj + 0x220);
    func_00336A78(obj + 0x268);
    func_00336828(obj + 0x2B0);
    for (i = 0; i < 21; i++) {
        func_00335E38(obj + 0x2EC, (float)D_00265140[i].x + (*(float **)(obj + 0x1C8))[0],
                      (float)D_00265140[i].y + (*(float **)(obj + 0x1C8))[1], 0.0f, 0.0f);
        func_00336920(obj + 0x2EC, 0xE99E, D_00265140[i].text);
        func_00335F88(obj + 0x2EC, D_00265140[i].scale, D_00265140[i].scale, 0.0f, 0.0f);
        if (D_001A7BE8[i] == 0)
            SetColor_0033B910(obj + 0x2EC, 0x70A0C0C0);
        else
            SetColor_0033B910(obj + 0x2EC, 0x7029A1FF);
        if (D_001A7BC8[i] != 0)
            func_00336828(obj + 0x2EC);
    }
    base = D_001C4FA0;
    if (*(int *)(base + 0x230) < 21) {
        char *first = obj + 0xE4;
        char *mid = obj + 0x130;
        char *bar = obj + 0x17C;
        char *q = obj + 0x114;
        char *r = first;
        int k;

        for (k = 2; k >= 0; k--) {
            ENTRY_F624a0727 *t = *(ENTRY_F624a0727 **)q;
            t->fn(r + t->n);
            r += 76;
            q += 76;
        }
        func_0028FAB8((int)(func_00335E10(first)[0] + 1.0f),
                      (int)(func_00335E10(mid)[1] + 6.0f),
                      (int)(func_00335E10(first)[0] + 1.0f),
                      (int)(func_00335E10(first)[1] - 4.0f), 0x180000FF, 0);
        func_0028FAB8((int)(func_00335E10(bar)[0] + 8.0f),
                      (int)(func_00335E10(bar)[1] + 2.0f),
                      (int)(func_00335E10(bar)[0] + 304.0f),
                      (int)(func_00335E10(bar)[1] + 2.0f), 0x180000FF, 0);
        func_0028FAB8((int)(func_00335E10(first)[0] + 1.0f),
                      (int)(func_00335E10(mid)[1] + 6.0f),
                      (int)(func_00335E10(first)[0] + 1.0f),
                      (int)(func_00335E10(first)[1] - 4.0f), 0x180000FF, 0);
        func_0028FAB8((int)(func_00335E10(bar)[0] + 8.0f),
                      (int)(func_00335E10(bar)[1] + 2.0f),
                      (int)(func_00335E10(bar)[0] + 304.0f),
                      (int)(func_00335E10(bar)[1] + 2.0f), 0x180000FF, 0);
    }
}

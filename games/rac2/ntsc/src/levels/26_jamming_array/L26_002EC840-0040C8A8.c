#include "common.h"

typedef struct {
    int a;
    float b;
    int c;
    int d;
} Quad_L26_0040C8A8;

extern void func_002B41A8(char *, char *, char *, char *, char *, char *, char *, char *,
                          char *, char *, char *);
extern void func_002A7750(char *, int, int);
extern void func_002B44C0(char *, char *, int);

void func_L26_0040C8A8(char *a0) {
    char *s1 = a0;
    char *s0 = *(char **)(s1 + 0x68);
    Quad_L26_0040C8A8 v;

    switch (*(u8 *)(s1 + 0x20)) {
    case 0:
        qzero_nc(&v);
        if (*(int *)(s0 + 0x120) != 0)
            v.b = (float)*(int *)(s0 + 0x120) * 1.7f;
        func_002B41A8(s1, s0 + 0x60, (char *)&v, 0, 0, 0, 0, 0, 0, 0, 0);
        if (*(int *)(s0 + 0x120) != 0) {
            *(int *)(s0 + 0xD0) = 0;
            *(int *)(s0 + 0x100) = 0;
            *(int *)(s0 + 0xD4) = 0;
        }
        func_002A7750(s1, 1, -1);
        break;
    case 1:
        func_002B44C0(s1, s0 + 0x60, 0);
        break;
    }
}

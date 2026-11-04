/* func_L00_002D1DA0 in its own unit (flags in config/cflags.txt: -O2 -G8 -ffunction-sections). */
#include "include_asm.h"

typedef struct {
    short key;
    unsigned char a;
    unsigned char b;
} Elem;

extern Elem D_L00_001A7D60 __attribute__((sda));
extern Elem D_L00_001A7D48 __attribute__((sda));
extern Elem D_L00_001A7D78 __attribute__((sda));

extern int func_L00_002A4EF0(int);
extern int func_L00_002ADEA0(int);
extern int func_002A7820(int, int);

int func_L00_002D1DA0(int a0, int *a1, int *a2)
{
    int buf[20];
    Elem *table;
    int count = 0;
    int idx;
    int key;

    key = func_L00_002A4EF0(a0);
    if (func_L00_002ADEA0(3) == 2) {
        table = &D_L00_001A7D60;
    } else if (func_L00_002ADEA0(3) == 3) {
        table = &D_L00_001A7D48;
    } else if ((func_L00_002ADEA0(3) ^ 4) != 0) {
        table = 0;
    } else {
        table = &D_L00_001A7D78;
    }
    if (table != 0) {
        for (idx = 0; table[idx].key != -1; idx++) {
            if (key == table[idx].key) {
                buf[count] = idx;
                count++;
            }
        }
    }
    idx = count - 1;
    if (count != 0) {
        int n = func_002A7820(0, idx);
        *a1 = table[buf[n]].a;
        *a2 = table[buf[n]].b;
        return 1;
    }
    *a1 = 1;
    return 0;
}

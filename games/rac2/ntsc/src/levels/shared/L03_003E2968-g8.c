#include "common.h"

extern int D_0024A7B0[14][6];
extern int D_L03_001B0B50 __attribute__((sda));
extern int func_002A77E0(int);

typedef struct {
    u8 pad[0x20];
    u8 path[16];
} PuzzleState29;

void func_L03_003E2968(u8 *p) {
    PuzzleState29 *q = *(PuzzleState29 **)(p + 0x68);
    int count = 3, start, old, inserted, i, a, b, j, k, n, found, scan, shift;

    q->path[0] = 0;
    switch (func_002A77E0(4)) {
    case 0:
        q->path[1] = 1;
        q->path[2] = 2;
        break;
    case 1:
        q->path[2] = 3;
        q->path[1] = 2;
        break;
    case 2:
        q->path[1] = 3;
        q->path[2] = 4;
        break;
    default:
        q->path[1] = 4;
        q->path[2] = 1;
        break;
    }

    while (count < D_L03_001B0B50) {
        start = func_002A77E0(count);
        old = count;
        inserted = 0;
        for (i = 0; i < count; i++) {
            a = (start + i) % count;
            b = (start + i + 1) % count;
            for (j = 0; j < 6; j++) {
                n = D_0024A7B0[q->path[a]][j];
                if (n == -1)
                    break;
                found = 0;
                for (scan = 0; scan < count; scan++)
                    if (n == q->path[scan])
                        found = 1;
                if (found)
                    continue;
                for (k = 0; k < 6; k++) {
                    if (D_0024A7B0[n][k] == q->path[b]) {
                        for (shift = count - 1; shift > a; shift--)
                            q->path[shift + 1] = q->path[shift];
                        q->path[a + 1] = n;
                        count++;
                        inserted = 1;
                        break;
                    }
                }
                if (inserted)
                    break;
            }
            if (inserted)
                break;
        }
        if (old == count) {
            D_L03_001B0B50 = count;
            break;
        }
    }
}

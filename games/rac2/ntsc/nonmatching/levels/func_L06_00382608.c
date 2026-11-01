/* Claude Opus 5.5 (xhigh), task v1673. Best: 608/584 bytes default, 596/584 with
 * -fno-rerun-loop-opt, 0/2 places. 50 and 57 of 146 retail instructions align; the earlier
 * draft aligned 44 and 49 (it reached 588 bytes with the flag, but with the wrong loop shape).
 *
 * New in this draft: `index += stride` as the for-increment and a separate digit counter. gcse
 * then places `i + step` on each switch path, before the digit loop on case 4, as in retail's
 * repeated `addu $s1,$s0,$s5`, and `i << 4` becomes the 0x3C stack slot.
 *
 * What still differs: retail hoists only (timer << 16) out of the outer loop. This C also hoists
 * the 1.2f constant, sp+0x10, sp+0x20 and flags & 7. loop.c keeps them in the loop only when
 * maybe_never is set before them, so during both loop passes retail seems to have a label or jump
 * at the top of the body that is gone from the final code. Measured control, not landable:
 * `if (stride == 77777) func_0026FEB8(D_L06_001AA660);` as the first statement of the body gives
 * retail's frame, slots 0x30-0x3C, in-loop constants and reloads: 119 of 146 align. Left over: the
 * guard itself, the constant 1 in $v0 against retail's $s1, mul.s placement and the mflo/mfhi and
 * trap order in the digit loop, which may follow from the guard's own blocks.
 * No wording tried gives the effect without code: a label alone is deleted by jump1, a dead counter
 * before loop, `if (...) break` and `if (...) return` are rolled into the exit test, null checks
 * are deleted before loop, and a guarded loop with a compound test is still rotated.
 * Next: find a real construct that survives to the second loop pass and then disappears.
 * Required declarations are retained below.
 */
#include "common.h"

extern void func_0026FEB8(const char *, ...);
extern const char D_L06_001AA660[];
extern char *D_L06_001FE500[];
extern float D_L06_001B9A90[];
extern void func_00282F20(void *, const void *, float);
extern void func_00282CC0(void *, const void *, const void *);
extern u8 *func_L03_00339128(void *, void *, int, int, u8, s16, u8, float);

void func_L06_00382608(void *data, int path, int owner, int flags, int stride, float scale)
{
    float previous[4] __attribute__((aligned(16)));
    float point[4] __attribute__((aligned(16)));
    float advance[4] __attribute__((aligned(16)));
    char *polygon;
    int timer;
    int index;

    if (path == -1) {
        func_0026FEB8(D_L06_001AA660);
        return;
    }
    polygon = D_L06_001FE500[path];
    timer = (flags & 0x40) ? -1 : 2;
    qcopy(previous, polygon + 0x10);
    for (index = 0; index < *(int *)polygon; index += stride) {
        func_00282F20(advance, D_L06_001B9A90, scale * 1.2f);
        qcopy(point, polygon + (index * 16 + 0x10));
        switch (flags & 7) {
        case 1:
            func_L03_00339128(point, data, 0, owner, 0x7F, timer, 0xFF, scale);
            break;
        case 2:
            func_L03_00339128(point, data, 1, owner, 0x7F, timer, 0xFF, scale);
            break;
        case 4:
            point[2] += scale * 0.5f;
            {
                int n = index;
                do {
                    func_L03_00339128(point, data, n % 10 + 2, owner, 0x7F, timer, 0xFF, scale);
                    n /= 10;
                    func_00282CC0(point, point, advance);
                } while (n != 0);
            }
            break;
        }
        qcopy(previous, polygon + (index * 16 + 0x10));
    }
}

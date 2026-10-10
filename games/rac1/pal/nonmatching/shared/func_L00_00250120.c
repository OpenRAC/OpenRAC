/* NON_MATCHING func_L00_00250120 -- src/overlays/shared/mobyfunc_0024FD50.c
 * Best so far: SIZE ours 160 / retail 164, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Unlinks the node in *slot from the owner's list (head at +0x60, next at +0x1C), calls func_001F99B0(*slot,0,0x
 *   Structure matches as static inline unlink_node (p2/p3/p8: same size 164, 16 words differ): only register alloc
 *   Retail keeps n in $v0 then copies it to $a0 (and $a1 in the loop) inside the else arm; ours keeps n in $a1 and
 *   mini9: reviewed p2/p3/p8 history; all three distinct helper/re-read wordings already produced identical 164-by
 *   Stopped without repeating them; needs a source form retaining the node in v0 before the else-arm argument copi
 *   s01 (5 runs, p9-p13): the static-inline unlink (best.c, p13 with an early return) stays at 16/164 bytes; the l
 *   Three wordings tie at 16 (best, p8, p13): stopped per QUEUE.md. Unblock: a source form that keeps n in the own
 */
extern void func_001F99B0(void *, int, int);

static inline void unlink_node(char *n, char *owner) {
    char *cur = *(char **)(owner + 0x60);
    char *nx = *(char **)(cur + 0x1C);

    while (nx != 0 && nx != n) {
        cur = nx;
        nx = *(char **)(cur + 0x1C);
    }
    if (nx == n) {
        *(char **)(cur + 0x1C) = *(char **)(n + 0x1C);
    }
}

// Unlinks the node held in *slot from the owner's list at 0x60 and frees it.
void func_L00_00250120(char *owner, char **slot) {
    char *n = *slot;

    if (n != 0) {
        if (*(char **)(owner + 0x60) == n) {
            *(char **)(owner + 0x60) = *(char **)(n + 0x1C);
        } else {
            unlink_node(n, owner);
        }
        func_001F99B0(*slot, 0, 0x40);
        *slot = 0;
    }
}

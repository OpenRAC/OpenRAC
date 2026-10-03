/* NON_MATCHING func_L00_00250120 -- src/overlays/shared/mobyfunc_0024FD50.c
 * Best so far: BYTES 16/164 (90.2% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Unlinks the node in *slot from the owner's list (head at +0x60, next at +0x1C), calls func_001F99B0(*slot,0,0x
 *   Structure matches as static inline unlink_node (p2/p3/p8: same size 164, 16 words differ): only register alloc
 *   Retail keeps n in $v0 then copies it to $a0 (and $a1 in the loop) inside the else arm; ours keeps n in $a1 and
 */
extern void func_001F99B0(void *, int, int);

static inline void unlink_node(char *n, char *owner) {
    char *cur = *(char **)(owner + 0x60);

    while (*(char **)(cur + 0x1C) != 0 && *(char **)(cur + 0x1C) != n) {
        cur = *(char **)(cur + 0x1C);
    }
    if (*(char **)(cur + 0x1C) == n) {
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

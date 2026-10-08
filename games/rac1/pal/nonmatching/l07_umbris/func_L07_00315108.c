/* NON_MATCHING func_L07_00315108 -- src/overlays/l07_umbris/vendor_00313D28.c
 * Best so far: SIZE ours 180 / retail 184, checked 2026-10-07.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   in f0 across the zero clamp, 0 in f1) is the allocator's choice; no source shape moved it.
 *   3. Default CSE reuses the entry low in the if-branch, retail recomputes it, but the
 *   per-function `-fno-cse-skip-blocks` that recomputes it also rematerialises the clamp constants.
 *   Candidate for the lead: `p1.c` (180/184, the nearest size). A per-function flag would need a
 *   combination that keeps the constants live while not CSE-ing the entry product; I did not find one
 *   in 8 runs. Not a plain-C match yet. Suggested next step if another worker takes it: try
 *   `-fno-cse-skip-blocks` with the clamp constants held in locals (`float half = 50.0f;`), and an
 *   extra copy at the join to get the `mov.s`.
 */
extern float D_0015EE60;

void func_L07_00315108(void *other, void *data, float current, float previous, float scale) {
    float base = D_0015EE60;
    float low = base * 0.06f;
    float amount = low;
    if (previous < current) {
        float change = current - previous;
        if (change > 50.0f) change = 50.0f;
        if (change < 0.0f) change = 0.0f;
        change /= 50.0f;
        amount = change * change * (base * 0.1f - low) + low;
    }
    *(float *)((char *)data + 0x1BC) = amount;
    *(float *)((char *)data + 0x94) = amount;
    *(float *)((char *)other + 0x58) = (*(float *)((char *)data + 0x80) / (D_0015EE60 * 0.06f)) * scale;
}

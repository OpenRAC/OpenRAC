/* GPT-6.1 Sol (high), 29 October 2026: FAILED.
 * Game profile: -O2 -G2 -ffunction-sections, standalone 001184D0.c.
 * USA v1.01: 120/120 bytes, 0/1 places, five instruction words differ.
 * Reusing the scan counter for the reversal fixes retail's base ($t1) and
 * reversal-counter ($t0) allocation, and every word from +0x38 onward agrees.
 * The length scan instead uses $t0 where retail uses $a3: +0xC, +0x10,
 * +0x18, +0x30 and +0x34; the transfer at +0x34 changes its source.
 * A pointer-advancing body added an entry copy and changed loop registers.
 * Making the handoff len = --i costs eight bytes; len = i - 1 is closer.
 * The older separate-counter draft had the right scan but nine differing
 * words from its base/counter swap. Next: split the scan and reversal live
 * ranges while preserving the entry delay slot and retail's base allocation.
 * Identity: in-place string reversal, called by func_00118548.
 * No external declarations are needed.
 */
#include "common.h"

char *func_001184D0(char *arg0) {
    int len;
    int i = 0;
    char c;

    if (arg0[0] != 0) {
        while (arg0[++i] != 0) {}
    }
    len = i - 1;
    if (len > 0) {
        i = 0;
        do {
            c = arg0[i];
            arg0[i] = arg0[len];
            arg0[len] = c;
            i++;
            len--;
        } while (i < len);
    }
    return arg0;
}

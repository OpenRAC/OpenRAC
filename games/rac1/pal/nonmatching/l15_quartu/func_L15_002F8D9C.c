/* NON_MATCHING func_L15_002F8D9C -- src/overlays/l15_quartu/vendor_002EDB50.c
 * Best so far: BYTES 1/12 (91.7% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   # func_L15_002F8D9C
 *   **What it does**: XORs an input value with 3, then sets result to 1 if less than 1 (unsigned), else 0. Returns
 *   **Where the difference is**: The retail assembly reads the input from $v0 (the return register) instead of fro
 *   **What would unblock it**: A normal C function prototype cannot express an argument coming in $v0. This would 
 */
// XOR with 3 and check if < 1 (unsigned)
int func_L15_002F8D9C(int a) {
    return ((unsigned)(a ^ 3)) < 1u;
}

/* NON_MATCHING func_L01_002B9440 -- src/overlays/shared/vendor_002B90A8.c
 * Best so far: BYTES 2/28 (92.9% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   # func_L01_002B9440
 *   **What it does**: Wrapper function that takes a pointer argument and calls func_L00_002A5158 with the argument
 *   **Where the difference is**: The generated code is 32 bytes while retail is 28 bytes. The function structure a
 *   **What would unblock it**: Understanding the exact signature and argument passing convention for func_L00_002A
 */
// Forward declaration
extern void func_L00_002A5158(char *);

// Wrapper function
void func_L01_002B9440(char *a) {
    func_L00_002A5158(a);
}

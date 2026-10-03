/* NON_MATCHING func_L00_0024EEF0 -- src/overlays/shared/missionfunc_0024EEF0.c
 * Best so far: SIZE ours 52 / retail 56, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   # func_L00_0024EEF0 Analysis
 *   ## What it does
 *   Initializes or resets function parameters, clearing two values at provided pointers when a second parameter is
 *   ## Where the difference is
 *   Generated code is 52 bytes vs retail 56 bytes. The instruction sequence differs in how branches are structured
 *   ## What would unblock it
 *   The 4-byte difference may be due to different branch prediction patterns or the bnel likely instruction requir
 */
extern short *D_L00_001870A0;

// Initialize function with conditional pointer writes
int func_L00_0024EEF0(int *arg4, int *arg5, int *arg6, int arg7) {
    int unused0 = 0;
    int unused1 = 1;
    short *ptr = D_L00_001870A0;
    if (arg5 != 0) {
        *arg4 = 0;
        *arg5 = 0;
    }
    if (arg6 != 0) {
        *arg6 = -1;
    }
    if (ptr == 0) {
        return 0;
    }
    return *ptr;
}

/* NON_MATCHING func_L00_0027A398 -- src/overlays/shared/pause_00277208.c
 * Best so far: SIZE ours 168 / retail 176, checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Skips linked menu entries excluded by two active pause filters and updates selected entry when changed.
 *   p1/p2/p3 compile identically despite explicit conditional, macro-address declaration, and ternary result: filt
 *   Stopped at three distinct identical wordings; filter alias/lifetime or function compiler optimization settings
 */
extern char D_L00_001BA070[] NOT_SDA;
// Skips linked entries excluded by the current pause filters.
int func_L00_0027A398(char *entry, int mode) {
 int skip = 0;
 int changed = 0;
 char *menu;
 int first;
 if (mode) return 0;
 menu = D_L00_001BA070;
 first = *(int *)(menu + 0x134);
 if (first) skip = (*(int *)(entry + 0x30) & 8) > 0;
 if (*(int *)(menu + 0x138) && (*(int *)(entry + 0x30) & 4)) skip = 1;
 if (skip) {
  changed = 1;
  do {
   entry = *(char **)(entry + 0x4C);
   skip = 0;
   if (first) {
    int flags = *(int *)(entry + 0x30);
    skip = (flags & 8) ? changed : 0;
   }
   if (*(int *)(menu + 0x138) && (*(int *)(entry + 0x30) & 4)) skip = changed;
  } while (skip);
 }
 if (changed) *(char **)(*(char **)(menu + 4) + 0x80) = entry;
 return 0;
}

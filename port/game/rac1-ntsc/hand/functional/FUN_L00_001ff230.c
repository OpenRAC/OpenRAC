/* Carried by port/tools/versionmap/carry_functions.py from the decompilation of Ratchet & Clank (PAL), nonmatching/functional/func_001F9BA0.c (games/rac1): func_001F9BA0, functional C there, not matched.
 * Kind `same` in the version map (../../README.md): the two versions have the same instructions for it.
 * Names and addresses are this version's. Functional C, not matched C. */
/* functional: high. fcheck: unchecked (never called in fcheck's capture run of the title, menu and level 0; OpenRAC tools/fcheck_gate.py, 2026-10-18). The least of three signed ints (retail: two pminw on the low words). */

int FUN_L00_001ff230(int a, int b, int c) {
    int m = a < b ? a : b;
    return m < c ? m : c;
}

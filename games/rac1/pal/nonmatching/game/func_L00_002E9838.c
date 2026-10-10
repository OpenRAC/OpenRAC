extern char *D_L00_00166F00;
extern int func_00208238(void);

/* Takes the slot held by key: -1 when the state's flag at 0x86 is set;
   otherwise, when the slot's word at 0x220 equals key, clears it and
   returns 1 (func_00208238), else 0. */
int func_L00_002E9838(int key) {
    char *g = D_L00_00166F00;
    char *slot;

    if (*(short *)(g + 0x86) != 0) {
        return -1;
    }
    slot = *(char **)(g + 0x70);
    if (*(int *)(slot + 0x220) == key) {
        *(int *)(slot + 0x220) = 0;
        return func_00208238();
    }
    return 0;
}

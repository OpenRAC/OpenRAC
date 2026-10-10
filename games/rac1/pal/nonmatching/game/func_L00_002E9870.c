extern char *D_L00_00166F00;
extern char *D_L00_0015F050 MACRO_ADDR;
extern int func_00208238(void);

/* Claims the slot in the current state for key. -1 when the state's flag at
   0x86 is set. An empty slot takes key (1). A slot already holding key gives 1.
   Otherwise the occupant and key are looked up in the 32-byte table at
   D_L00_0015F050 (by their index at 0x84); the key takes the slot (1) only
   when the occupant's priority byte, at its record's 0x1C, is below the
   key's. Else 0. */
int func_L00_002E9870(char *key) {
    char *g = D_L00_00166F00;
    char *slot;
    char *occ;
    char *rec_occ;
    char *rec_key;
    short oi;
    short ki;

    if (*(short *)(g + 0x86) != 0) {
        return -1;
    }
    slot = *(char **)(g + 0x70);
    occ = *(char **)(slot + 0x220);
    if (occ == 0) {
        *(char **)(slot + 0x220) = key;
        return 1;
    }
    if (occ == key) {
        return 1;
    }
    oi = *(short *)(occ + 0x84);
    ki = *(short *)(key + 0x84);
    rec_occ = *(char **)(D_L00_0015F050 + (int)oi * 32 + 0x1C);
    rec_key = *(char **)(D_L00_0015F050 + (int)ki * 32 + 0x1C);
    if ((unsigned char)rec_occ[0x1C] < (unsigned char)rec_key[0x1C]) {
        *(char **)(slot + 0x220) = key;
        return func_00208238();
    }
    return 0;
}

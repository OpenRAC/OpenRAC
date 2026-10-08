/* NON_MATCHING func_L08_002E30E8 -- src/overlays/l08_batalia/vendor_002E0258.c
 * Best so far: SIZE ours 668 / retail 776, checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
typedef struct {
    char *sub;
    char *parent;
    int joint;
    int pjoint;
} Joint_2e30e8;

/* Builds the boss's part table: spawns its body parts (classes 0x1B9..0x1C2) attached to the right
 * joints of their parents, then three rows of eight 0x1BB segments hanging off parts 7, 8 and 9. */
void func_L08_002E30E8(char *m) {
    char *d = *(char **)(m + 0x78);
    Joint_2e30e8 *e = (Joint_2e30e8 *)(d + 0x60);
    int k;
    e[0].sub = m;
    e[0].parent = 0;
    e[0].joint = 0;
    e[0].pjoint = 0;
    e[3].sub = func_L08_002E3010(m, 0x1C1, 0, 2);
    e[3].parent = m;
    e[3].joint = 0;
    e[3].pjoint = 2;
    e[1].sub = func_L08_002E3010(m, 0x1C2, 0, 3);
    e[1].parent = m;
    e[1].joint = 0;
    e[1].pjoint = 3;
    e[2].sub = func_L08_002E3010(m, 0x1C2, 0, 4);
    *(unsigned short *)(e[2].sub + 0x34) |= 0x8000;
    e[2].parent = m;
    e[2].joint = 0;
    e[2].pjoint = 4;
    e[7].sub = func_L08_002E3010(m, 0x1C0, 0, 0);
    e[7].parent = m;
    e[7].joint = 0;
    e[7].pjoint = 0;
    e[8].sub = func_L08_002E3010(m, 0x1C0, 1, 1);
    e[8].parent = m;
    e[8].joint = 1;
    e[8].pjoint = 1;
    e[9].sub = func_L08_002E3010(e[8].sub, 0x1C0, 1, 0);
    e[9].parent = e[8].sub;
    e[9].joint = 1;
    e[9].pjoint = 0;
    e[5].sub = func_L08_002E3010(e[9].sub, 0x1BD, 0, 0);
    e[5].parent = e[9].sub;
    e[5].joint = 0;
    e[5].pjoint = 0;
    e[6].sub = func_L08_002E3010(e[7].sub, 0x1BA, 1, 1);
    e[6].parent = e[7].sub;
    e[6].joint = 1;
    e[6].pjoint = 1;
    e[4].sub = func_L08_002E3010(m, 0x1B9, 0, 0);
    e[4].parent = e[6].sub;
    e[4].joint = 0;
    e[4].pjoint = 0;
    for (k = 0; k < 8; k++) {
        e[10 + k].sub = func_L08_002E3010(e[7].sub, 0x1BB, 0, k + 2);
        e[10 + k].parent = e[7].sub;
        e[10 + k].joint = 0;
        e[10 + k].pjoint = k + 2;
    }
    for (k = 0; k < 8; k++) {
        e[18 + k].sub = func_L08_002E3010(e[8].sub, 0x1BB, 0, k + 2);
        e[18 + k].parent = e[8].sub;
        e[18 + k].joint = 0;
        e[18 + k].pjoint = k + 2;
    }
    for (k = 0; k < 8; k++) {
        e[26 + k].sub = func_L08_002E3010(e[9].sub, 0x1BB, 0, k + 2);
        e[26 + k].parent = e[9].sub;
        e[26 + k].joint = 0;
        e[26 + k].pjoint = k + 2;
    }
}

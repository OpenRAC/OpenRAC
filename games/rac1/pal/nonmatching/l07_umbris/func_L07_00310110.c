/* NON_MATCHING func_L07_00310110 -- src/overlays/l07_umbris/vendor_002CE470.c
 * Best so far: BYTES 5/436 (98.8% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Resolves four table indices in an object's data (0x110,0x144,0x118,0x114) into pointers, logging via func_001E
 */
extern void func_L07_0030FF18(char *data);
extern int func_L07_0030FFA8(char *moby, char *data);
extern void func_L07_00310038(char *arg);
extern int func_001160D8(void);
extern int func_001E9730();
extern char *D_L07_001B0830[];
extern char *D_L07_00160058 MACRO_ADDR;
extern char *D_L07_0016016C MACRO_ADDR;
extern char D_L07_00211A10[];
extern char D_L07_00211A48[];
extern char D_L07_00211A80[];
extern char D_L07_00211AB0[];
extern void qcopy(void *, void *);

// Resolves the indices stored in an object's data block into pointers; logs and fails on -1.
int func_L07_00310110(char *moby, char *d) {
    char *p;
    char *q;
    char **t;
    int i = *(int *)(d + 0x110);
    int j, k, n;
    if (i == -1) {
        func_001E9730(D_L07_00211A10, (int)(moby - D_L07_00160058) >> 8);
        return 1;
    }
    t = D_L07_001B0830;
    p = t[i];
    *(char **)(d + 0x128) = p;
    func_L07_0030FF18(p);
    j = *(int *)(d + 0x144);
    if (j == -1) {
        func_001E9730(D_L07_00211A48, (int)(moby - D_L07_00160058) >> 8);
        return 1;
    }
    q = t[j];
    *(char **)(d + 0x148) = q;
    func_L07_0030FF18(q);
    k = *(int *)(d + 0x118);
    if (k == -1) {
        func_001E9730(D_L07_00211A80, (int)(moby - D_L07_00160058) >> 8);
        return 1;
    }
    *(char **)(d + 0x12C) = D_L07_00160058 + (k << 8) + 0x18;
    moby[0xBC] = func_L07_0030FFA8(moby, d);
    n = *(int *)(d + 0x114);
    if (n == -1) {
        func_001E9730(D_L07_00211AB0, (int)(moby - D_L07_00160058) >> 8);
        return 1;
    }
    qcopy(d + 0xF0, D_L07_0016016C + (n << 7) + 0x30);
    qcopy(d + 0xE0, moby + 0x10);
    ((void (*)(char *, char *))func_L07_00310038)(moby, d);
    *(int *)(d + 0x134) = func_001160D8() % 0x78;
    if (func_001160D8() & 1) {
        *(float *)(d + 0x140) = 0.34906584f;
    } else {
        *(float *)(d + 0x140) = -0.34906584f;
    }
    *(int *)(d + 0x14C) = 0;
    d[0x29] = 1;
    return 0;
}

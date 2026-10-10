/* NON_MATCHING func_L05_00254328 -- src/overlays/shared/help_00237B00.c
 * Best so far: SIZE ours 1272 / retail 1296, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Shared help routine: three walks over moby lists (classes 0x473, 0x474, 0x1D6) that score each entry against t
 */
extern int func_L00_0025A208(int *, int, int, int);
extern int func_L00_001FEF78(void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_001FA190(void *);
extern void func_001FA4A0(void *, void *);
extern void func_001F9EE8(void *, void *, void *);
extern float func_001F9B88(float);
extern int func_001F9850(int);
extern int func_0022ED80(int, int, int);
extern int func_L00_0025A2F0(int *, int, int, int);
extern float func_001FA888(int);
extern float func_001F9FA8(float);
extern int func_001FA898(float);
extern float func_001F9D10(void *, void *);
extern void func_L05_0023E348(int i);
extern char D_0013D50F[];
extern int D_L05_0015F6B0_m __asm__("D_L05_0015F6B0") MACRO_ADDR;

/* Walks three lists of moby ids from the level's dispatch data, scoring each entry against the hero. */
void func_L05_00254328(void) {
    float vec0[4], vec1[4];
    int list1, list2, list3;
    char *hero = (char *)D_0013E633 + 0xE1D;
    char *c, *d, *p, *pp;
    int t, rA, rB, rC, r17, r19, pk, a16, v, k;
    float fdiv, x;

    c = *(char **)(hero + 0x86C);
    if (c == 0) {
        return;
    }
    d = *(char **)(c + 0x78);
    if (*(int *)(d + 0x44) != -1) {
        func_L00_0025A208(&list1, *(int *)(d + 0x44), 0, 0);
        while (list1 != 0) {
            if (*(short *)(((char *)list1) + 0xA6) == 0x473 && func_L00_001FEF78(((char *)list1) + 0xBC)) {
                func_001F9BF0(vec0, ((char *)list1) + 0x10, (char *)D_0013E633 + 0xEED);
                func_001FA190(vec1);
                func_001FA4A0(vec1, ((char *)list1) + 0xC0);
                func_001F9EE8(vec0, vec0, vec1);
                if (func_001F9B88(vec0[0]) < 0.5f && func_001F9B88(vec0[1]) < 2.9f
                    && func_001F9B88(vec0[2]) < 2.9f) {
                    *(int *)(hero + 0x8C4) += *(int *)(*(char **)(((char *)list1) + 0x78));
                    *(unsigned char *)(((char *)list1) + 0xBC) = func_001F9850(60);
                    if (*(char **)(hero + 0x86C) != 0) {
                        func_0022ED80(1, 0, (int)*(char **)(hero + 0x86C));
                    }
                }
            }
            func_L00_0025A2F0(&list1, list1, 0, 0);
        }
    }

    t = func_001F9850(50);
    fdiv = func_001FA888(t);
    v = D_L05_0015F6B0_m;
    x = (float)(v % t) / fdiv;
    x = x + x;
    x = x * 3.14159265f;
    x = x + -3.14159265f;
    fdiv = func_001F9FA8(x);
    rA = func_001FA898(fdiv * 48.0f);
    r19 = rA + 0x68;
    rB = func_001FA898(fdiv * 64.0f);
    a16 = rB + 0xD0;
    rC = func_001FA898(fdiv * 24.0f);
    r17 = rC + 0x3C;
    if (!(a16 < 0x100)) {
        a16 = 0xFF;
    }

    if (*(int *)(d + 0x48) != -1 && *(short *)(hero + 0x8BC) == 0) {
        func_L00_0025A208(&list2, *(int *)(d + 0x48), 0, 0);
        if (list2 != 0) {
            pk = r17 << 16;
            pk = pk | 0x80000000;
            pk = pk | (a16 << 8);
            pk = pk | r19;
            while (list2 != 0) {
                    if (*(short *)(((char *)list2) + 0xA6) == 0x474) {
                    *(unsigned short *)(((char *)list2) + 0x34) = *(unsigned short *)(((char *)list2) + 0x34) | 0x10;
                    *(int *)(((char *)list2) + 0x90) = pk;
                    if (func_L00_001FEF78(((char *)list2) + 0xBC)) {
                        func_001F9BF0(vec0, ((char *)list2) + 0x10, (char *)D_0013E633 + 0xE9D);
                        func_001FA190(vec1);
                        func_001FA4A0(vec1, ((char *)list2) + 0xC0);
                        func_001F9EE8(vec0, vec0, vec1);
                        if (func_001F9B88(vec0[0]) < 2.7f && func_001F9B88(vec0[1]) < 1.25f
                            && func_001F9B88(vec0[2]) < 1.5f) {
                            *(int *)(hero + 0x8C4) += *(int *)(*(char **)(((char *)list2) + 0x78));
                            *(unsigned char *)(((char *)list2) + 0xBC) = func_001F9850(60);
                            if (*(char **)(hero + 0x86C) != 0) {
                                func_0022ED80(1, 0, (int)*(char **)(hero + 0x86C));
                            }
                        }
                    }
                }
                func_L00_0025A2F0(&list2, list2, 0, 0);
            }
        }
    }

    if (*(int *)(d + 0x4C) != -1) {
        func_L00_0025A208(&list3, *(int *)(d + 0x4C), 0, 0);
        pp = D_0013D50F + 0x21;
        while (list3 != 0) {
            if (*(short *)(((char *)list3) + 0xA6) == 0x1D6 && func_L00_001FEF78(((char *)list3) + 0xBC)) {
                if (func_001F9D10((char *)D_0013E633 + 0xEED, ((char *)list3) + 0x10) < 1.5f) {
                    *(unsigned char *)(((char *)list3) + 0xBC) = func_001F9850(300);
                    if (*(int *)(hero + 0x10B8) != 0x24) {
                        *(int *)(hero + 0x20B8) = 0x24;
                        *(unsigned char *)(hero + 0x20AC) = 0;
                        func_L05_0023E348(0);
                        *(unsigned char *)(hero + 0x20AC) = 1;
                    }
                    k = *(unsigned char *)(hero + 0x8CE) + 1;
                    *(unsigned char *)(hero + 0x8CE) = k;
                    if (!((k & 0xFF) < 4)) {
                        *(unsigned char *)(hero + 0x8CE) = 3;
                    }
                    *(int *)(hero + 0x20B8) = 0x24;
                    *(int *)(pp + 0x90) = *(int *)(pp + 0x90) + 1;
                }
            }
            func_L00_0025A2F0(&list3, list3, 0, 0);
        }
    }
}

/* func_00205270: select the world object's resource tables. a is a class/object id (looked up in
   D_001B3E40 for its slot); b is a flag, or -1 to take it from D_00160050. Sets the current index
   D_0016004C from the id table D_001CBE40, stores the object pointer in D_001B3580[slot] and its
   D_001B6500 word, calls func_00203B70, fills the 16 per-entry records of the current set, and when
   the id matches an entry of D_001864D0 (37 entries of 0x4C bytes) writes two 64-bit words of
   D_0019E7F0 into the object's sub-record. Not called in the capture run: checked statically. */
extern int D_001CBE40_s[] __asm__("D_001CBE40");
extern int D_001CBEA0_s[] __asm__("D_001CBEA0");
extern char D_001CBF60_s[] __asm__("D_001CBF60");
extern char D_001CC0E0_s[] __asm__("D_001CC0E0");
extern char D_001CAE40_s[] __asm__("D_001CAE40");
extern char D_001864D0_s[] __asm__("D_001864D0");
extern unsigned char D_0013E620_s[] __asm__("D_0013E620");
extern long D_0019E7F0_s[] __asm__("D_0019E7F0");
extern int D_0016004C_s __asm__("D_0016004C") MACRO_ADDR;
extern int D_00160048_s __asm__("D_00160048") MACRO_ADDR;
extern int D_00160044_s __asm__("D_00160044") MACRO_ADDR;
extern int D_00160050_s __asm__("D_00160050") MACRO_ADDR;
extern void func_0020C468_s(int, int) __asm__("func_0020C468");

void func_00205270(int a, int b) {
    int g1;
    int g2;
    int i;
    int j;
    int v;
    int off;
    int idx;
    int g1b;
    int g1c;
    int g4;
    char *p;
    char *obj;
    char *q0;
    char *w;
    char **slot;
    int *slotB;

    g1 = D_0016004C_s;
    if (g1 >= 0 && D_001CBE40_s[g1] == a) {
        return;
    }

    g2 = D_00160048_s;
    D_0016004C_s = 0;
    if (g2 > 0 && D_001CBE40_s[0] != a) {
        i = 1;
        while (i < g2) {
            if (D_001CBE40_s[i] == a) {
                break;
            }
            i++;
        }
        D_0016004C_s = i;
    }

    if (b == -1) {
        b = (D_00160050_s == 0);
    }
    v = D_001941C0[4] + b * 0x18000;
    D_00160050_s = b;
    func_00118D80(0);

    g1 = D_0016004C_s;
    func_0020C468_s(D_001CBEA0_s[g1], v);
    func_00118D80(0);

    idx = D_001B3E40[a];
    g1b = D_0016004C_s;
    slot = &D_001B3580[idx];
    slotB = &D_001B6500[idx];
    *slot = (char *)v;
    *slotB = *(int *)(v + 0x2C);
    func_00203B70((void *)v, (int)D_001CAE40_s, (g1b << 4) + (int)D_001CBF60_s, a);

    g1c = D_0016004C_s;
    g4 = D_00160044_s;
    p = D_001CC0E0_s + (g1c << 5);
    for (i = 0; i < 16; i++) {
        short val = *(short *)p;
        if (val >= 0) {
            obj = *slot;
            *(short *)(*(char **)(obj + 0x28) + (i << 5) + 0x1A) = val;
            obj = *slot;
            *(int *)(*(char **)(obj + 0x28) + (i << 5) + 0x1C) = g4;
        }
        p += 2;
    }

    for (j = 0; j < 0x25; j++) {
        if (*(int *)(D_001864D0_s + 0x10 + 0x4C * j) == a) {
            if (D_0013E620_s[j] == 0) {
                return;
            }
            obj = *slot;
            if (*(unsigned char *)(obj + 6) == 0) {
                return;
            }
            q0 = *(char **)obj + (*(unsigned char *)(obj + 7) << 4);
            w = *(char **)q0 + ((*(int *)(q0 + 4) - 4) << 4);
            {
                long d0 = D_0019E7F0_s[0];
                long d2 = D_0019E7F0_s[2];
                *(long *)(w + 0x20) = d0;
                *(long *)(w + 0x30) = d2;
            }
            return;
        }
    }
}

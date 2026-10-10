/* NON_MATCHING func_L10_002CB030 -- src/overlays/l10_orxon/vendor_00296BD8.c
 * Best so far: SIZE ours 1028 / retail 1036, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Update function (moby class 794, level 10), 1036 bytes: list walk over data+0x18 (5 records, count in cnt), th
 *   Differences left: register choice in the walk (retail keeps the walk counter and count in temporaries $a3/$t3;
 */
extern int func_001E9730();
extern f32 func_001FA888(s32);
extern s32 func_001FA898(f32);
extern int func_001F9908(int *);
extern void func_0020D678(void *);
extern void func_L10_002CAD18(void *, void *);
extern unsigned char D_0014171B[] NOT_SDA;
extern int D_0015EE84_m __asm__("D_0015EE84") MACRO_ADDR;
extern char *D_L10_00160058_b __asm__("D_L10_00160058") MACRO_ADDR;
extern char D_L10_001DD0A0[];
extern char D_L10_001DD0D0[];
extern char D_L10_001DD100[];

/* Update function for moby class 794 on level 10: walks the id list at data+0x18, then acts on the moby's state. */
void func_L10_002CB030(char *moby) {
    char *data = *(char **)(moby + 0x78);
    char *list = data + 0x18;
    char *e = 0;
    int state, cnt = 0, k;
    char *p;
    unsigned char *tb = D_0014171B + 0xAA35;
    if (*(int *)(data + 0x10) != -1) e = D_L10_00160058_b + (*(int *)(data + 0x10) << 8);
    if (((unsigned char *)moby)[0xB0] == 0xFF || tb[((unsigned char *)moby)[0xB0] + (D_0015EE84_m << 4)] != 0xFF) {
        if (((unsigned char *)moby)[0x20] != 0) {
            int kw;
            char *pw = list;
            for (kw = 4; kw >= 0; kw--, pw += 8) {
                int id = *(int *)pw;
                char *e2;
                if (id == -1) continue;
                e2 = D_L10_00160058_b + (id << 8);
                if (e2 == 0) continue;
                if (*(short *)(e2 + 0xA6) != *(short *)(pw + 4)) continue;
                if (((unsigned char *)e2)[0x20] == 0xFE) continue;
                if (((unsigned char *)e2)[0x20] == 0xFD) continue;
                if (!((*(int *)(D_0014171B + 0xAB75 + (((short)*(unsigned short *)(e2 + 0xB2) >> 5) * 4 + (D_0015EE84_m << 8))) >> (*(unsigned short *)(e2 + 0xB2) & 0x1F)) & 1)) continue;
                if (*(signed char *)(pw + 6) == -1) { cnt++; continue; }
                if (((unsigned char *)e2)[0x20] == *(signed char *)(pw + 6)) continue;
                cnt++;
            }
        }
    }
    state = ((unsigned char *)moby)[0x20];
    switch (state) {
    case 0: {
        float f1, f2, m;
        if (e != 0) {
            *(short *)(data + 0x14) = *(unsigned short *)(e + 0xA6);
            *(float *)(data + 0x44) = func_001FA888(((unsigned char *)e)[0x23]);
            if (*(int *)(data + 8) & 0x10) {
                *(u128 *)(moby + 0x10) = *(u128 *)(e + 0x10);
            } else if (*(int *)(data + 8) & 0x40) {
                *(u128 *)(e + 0x10) = *(u128 *)(moby + 0x10);
            }
            if (*(int *)(data + 8) & 0x20) {
                *(u128 *)(moby + 0x40) = *(u128 *)(e + 0x40);
            } else if (*(int *)(data + 8) & 0x80) {
                *(u128 *)(e + 0x40) = *(u128 *)(moby + 0x40);
            }
        } else {
            func_001E9730(D_L10_001DD0A0);
        }
        p = list;
        for (k = 0; k < 5; k++, p += 8) {
            if (*(int *)p == -1) {
                func_001E9730(D_L10_001DD100, ((int)moby - (int)D_L10_00160058_b) >> 8, k);
            } else {
                char *e2 = D_L10_00160058_b + (*(int *)p << 8);
                *(short *)(p + 4) = *(unsigned short *)(e2 + 0xA6);
                func_001E9730(D_L10_001DD0D0, ((int)moby - (int)D_L10_00160058_b) >> 8, k, *(int *)p, *(short *)(e2 + 0xA6));
            }
        }
        f2 = *(float *)data;
        f1 = *(float *)(data + 4);
        m = *(float *)(*(char **)(moby + 0x24) + 0x24);
        if (f2 < f1) *(float *)(moby + 0x2C) = f1 * m;
        else *(float *)(moby + 0x2C) = f2 * m;
        moby[0x20] = 1;
        return;
    }
    case 1:
        if (cnt != 0) {
            func_L10_002CAD18(moby, data);
            return;
        }
        if (e != 0 && *(signed char *)(data + 0x16) != -1) ((unsigned char *)e)[0x20] = *(unsigned char *)(data + 0x16);
        *(int *)(data + 0x40) = *(int *)(data + 0xC);
        moby[0x20] = 2;
        func_L10_002CAD18(moby, data);
        return;
    case 2: {
        float a, b;
        if (e != 0 && !(*(int *)(data + 8) & 2)) {
            a = func_001FA888(*(int *)(data + 0x40));
            b = func_001FA888(*(int *)(data + 0xC));
            ((unsigned char *)e)[0x23] = func_001FA898(*(float *)(data + 0x44) * a / b);
        }
        if (!(*(int *)(data + 8) & 4)) {
            if (e != 0) {
                signed char c = *(signed char *)(data + 0x17);
                if (c != -1 && ((unsigned char *)e)[0x20] == c) moby[0x20] = 3;
            }
        }
        if (!(*(int *)(data + 8) & 8)) {
            if (func_001F9908((int *)(data + 0x40))) moby[0x20] = 3;
        }
        func_L10_002CAD18(moby, data);
        return;
    }
    case 3:
        if (e != 0 && !(*(int *)(data + 8) & 1)) func_0020D678(e);
        func_0020D678(moby);
        return;
    case 4:
        return;
    default:
        return;
    }
}

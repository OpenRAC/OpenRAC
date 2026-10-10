/* NON_MATCHING func_L07_0030E370 -- src/overlays/l07_umbris/vendor_002CE470.c
 * Best so far: SIZE ours 1152 / retail 1168, checked 2026-10-08.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * No longer builds in its file (COMPILE failed, 2026-10-09): match its declarations to the file's first.
 * What the last attempts found:
 *   Umbris state machine (jump table on moby[0x20], 0..4 plus end): best p1.c is 1152 bytes against retail 1168, a
 *   Left: the table-name constant D_0014171B + 0xAA35 comes out as an immediate offset (retail keeps the symbol ex
 */
extern int func_0022ED80(int, int, int);
extern float func_001F9D48(void *, void *);
extern void func_001F9EC0(void *, void *, void *);
extern void func_001F9BD8(void *, void *, void *);
extern void func_L00_0028EBF0(int);
extern int D_0015EE84 MACRO_ADDR;
extern char D_0014171B[];
extern int D_L07_001845A8[];
extern char *D_L07_00160058 MACRO_ADDR;
extern float D_0015EE60 MACRO_ADDR;
extern short D_0013EE60;
extern char D_0013E633[];

typedef int u128 __attribute__((mode(TI)));

/* Umbris state machine for moby classes 1013, 1014 and 1064: tracks a target moby and spawns an effect when it is in range. */
void func_L07_0030E370(char *moby) {
    char *p = *(char **)(moby + 0x78);
    float v[4];
    u128 t10;
    u128 t20;
    int id;
    unsigned char s;
    float d;

    v[0] = *(float *)p;
    v[1] = *(float *)(p + 4);
    v[2] = *(float *)(p + 8);
    v[3] = 0;

    if ((unsigned char)moby[0xB0] != 0xFF) {
        if ((unsigned char)D_0014171B[(unsigned char)moby[0xB0] + (D_0015EE84 << 4) + 0xAA35] == 0xFF) {
            *(int *)(p + 0x10) = -1;
        }
    }

    switch ((unsigned char)moby[0x20]) {
    case 0:
        *(float *)p = *(float *)(moby + 0x10);
        *(float *)(p + 4) = *(float *)(moby + 0x14);
        *(short *)(p + 0xC) = -1;
        *(float *)(p + 8) = *(float *)(moby + 0x18);
        id = *(short *)(p + 0x14);
        if (id != -1) {
            D_L07_001845A8[id] = 0;
        }
        moby[0x20] = 1;
        break;

    case 1:
        id = *(int *)(p + 0x10);
        if (id != -1) {
            if (D_L07_00160058 + (id << 8) != 0) {
                s = (unsigned char)(D_L07_00160058 + (id << 8))[0x20];
                if (s != 0xFE && s != 0xFD && (int)s != *(short *)(p + 0x16)) {
                    return;
                }
            }
        }
        if ((signed char)p[0xE] != -1) {
            func_0022ED80((signed char)p[0xE], 0, (int)moby);
        }
        moby[0x20] = 2;
        break;

    case 2:
        id = *(int *)(p + 0x10);
        if (id != -1 && D_L07_00160058 + (id << 8) != 0) {
            s = (unsigned char)(D_L07_00160058 + (id << 8))[0x20];
            if (s != 0xFE && s != 0xFD && (int)s == *(int *)(p + 0x18)) {
                func_0022ED80((signed char)p[0xE], 0, (int)moby);
                moby[0x20] = 4;
                break;
            }
        }
        d = func_001F9D48(moby + 0x10, v);
        if (*(float *)(p + 0x1C) < d) {
            if ((signed char)p[0xF] != -1) {
                func_0022ED80((signed char)p[0xF], 0, (int)moby);
            }
            id = *(short *)(p + 0x14);
            if (id != -1) {
                D_L07_001845A8[id] = 1;
            }
            moby[0x20] = 3;
        } else {
            float k = *(float *)&D_0013EE60 * 0.01f;
            t20 = 0;
            ((float *)&t20)[1] = -k;
            t10 = t20;
            if (*(short *)(moby + 0xA6) == 0x3F5 || *(short *)(moby + 0xA6) == 0x429) {
                ((float *)&t10)[1] = -((float *)&t10)[1];
            }
            func_001F9EC0(&t20, &t10, moby + 0xC0);
            func_001F9BD8(moby + 0x10, moby + 0x10, &t20);
        }
        break;

    case 3:
        id = *(int *)(p + 0x10);
        if (id != -1 && D_L07_00160058 + (id << 8) != 0) {
            s = (unsigned char)(D_L07_00160058 + (id << 8))[0x20];
            if (s != 0xFE && s != 0xFD) {
                if ((int)s != *(int *)(p + 0x18)) {
                    return;
                }
                if ((signed char)p[0xE] != -1) {
                    func_0022ED80((signed char)p[0xE], 0, (int)moby);
                }
                moby[0x20] = 4;
            }
        }
        break;

    case 4:
        id = *(int *)(p + 0x10);
        if (id != -1 && D_L07_00160058 + (id << 8) != 0) {
            s = (unsigned char)(D_L07_00160058 + (id << 8))[0x20];
            if (s != 0xFE && s != 0xFD && (int)s != *(short *)(p + 0x16)) {
                d = func_001F9D48(moby + 0x10, v);
                if (d <= D_0015EE60 * 0.01f) {
                    *(float *)(moby + 0x10) = *(float *)p;
                    *(float *)(moby + 0x14) = *(float *)(p + 4);
                    *(int *)(moby + 0x1C) = 0;
                    *(float *)(moby + 0x18) = *(float *)(p + 8);
                    id = *(short *)(p + 0xC);
                    if (id != -1) {
                        char *t = D_0013E633 + 0x1D + id * 0x70;
                        if (*(int *)(t + 0x88) == (int)moby && *(unsigned char *)(t + 0x74) != 0) {
                            func_L00_0028EBF0(id);
                        }
                    }
                    *(short *)(p + 0xC) = -1;
                    if ((signed char)p[0xF] != -1) {
                        func_0022ED80((signed char)p[0xF], 0, (int)moby);
                    }
                    id = *(short *)(p + 0x14);
                    if (id != -1) {
                        D_L07_001845A8[id] = 0;
                    }
                    moby[0x20] = 1;
                } else {
                    t20 = 0;
                    ((float *)&t20)[1] = D_0015EE60 * 0.01f;
                    t10 = t20;
                    if (*(short *)(moby + 0xA6) == 0x3F5 || *(short *)(moby + 0xA6) == 0x429) {
                        ((float *)&t10)[1] = -((float *)&t10)[1];
                    }
                    func_001F9EC0(&t20, &t10, moby + 0xC0);
                    func_001F9BD8(moby + 0x10, moby + 0x10, &t20);
                }
                return;
            }
            func_0022ED80((signed char)p[0xE], 0, (int)moby);
            moby[0x20] = 2;
        } else {
            func_0022ED80((signed char)p[0xE], 0, (int)moby);
            moby[0x20] = 2;
        }
        break;

    default:
        break;
    }
}

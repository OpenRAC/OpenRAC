/* NON_MATCHING func_L13_002E5810 -- src/overlays/l13_gemlik/vendor_002C2638.c
 * Best so far: SIZE ours 1232 / retail 1252, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   What it does: moby class 231 update on level 13: a jump table on moby[0x20] (cases 0..4; 1 and 2 share one bod
 *   Remaining differences: (1) regalloc: data (pseudo 117, 54 refs) takes $16 and moby takes $18; retail has moby 
 */
extern int func_L00_0025A208(int *, int, int, int);
extern int func_L00_0025A2F0(int *, int, int, int);
extern void func_L00_00251328(void *, int, int, int);
extern int func_001F9850(int);
extern void func_0022ED80(int, int, int);
extern void func_L01_0026F090(int list, int state);
extern int func_001E9730_d(void *, ...) __asm__("func_001E9730");
extern int func_001F9938(void *);
extern int func_L13_002E56C8(char *m, char *arg, float *t);
extern char D_L13_001F5080[];
extern char D_L13_001F50A0[];
extern char D_L13_001F50C0[];
extern int D_L13_0015F6B0 MACRO_ADDR;

/* Moby class 231 update (level 13): a small state machine driven by moby[0x20] with a jump table. */
void func_L13_002E5810(char *moby) {
    char *data = *(char **)(moby + 0x78);
    int v5 = 0;
    int found;
    int st;
    int r16, r17, q;
    int gv;
    char *obj;
    char *objdata;

    if (*(short *)(data + 0x74) == 0) {
        if ((unsigned int)(*(unsigned char *)(moby + 0x20) - 1) < 2) {
            v5 = func_L13_002E56C8(moby, data, (float *)(data + 0x20));
        }
    }
    st = *(unsigned char *)(moby + 0x20);

    switch (st) {
    case 0:
        if (*(unsigned char *)(data + 0x79) != 0) {
            *(short *)(data + 0x76) = 0;
        } else {
            *(unsigned char *)(data + 0x7B) = 0;
            *(char **)(data + 0x80) = moby;
            func_L00_0025A208(&found, *(unsigned char *)(moby + 0x21), 0, 0);
            while (found != 0) {
                if (found != (int)moby) {
                    if (*(short *)(found + 0xA6) == 0xE7) {
                        char *o2 = *(char **)(found + 0x78);
                        o2[0x79] = 1;
                        *(char **)(o2 + 0x80) = moby;
                    }
                }
                *(unsigned char *)(data + 0x7B) = *(unsigned char *)(data + 0x7B) + 1;
                func_L00_0025A2F0(&found, found, 0, 0);
            }
            *(short *)(data + 0x76) = 0;
        }
        *(short *)(data + 0x70) = 0;
        *(short *)(data + 0x72) = 0;
        data[0x7A] = 0;
        func_L00_00251328(moby, 0x80, 0x80, 0x80);
        *(short *)(data + 0x74) = 0;
        moby[0x20] = 1;
        break;

    case 1:
    case 2:
        if (*(short *)(data + 0x74) == 0 && v5 != 0) {
            obj = *(char **)(data + 0x80);
            objdata = *(char **)(obj + 0x78);
            if (*(unsigned char *)(obj + 0x20) == 1) {
                data[0x7A] = 0;
                obj[0x20] = 2;
                if (*(int *)(objdata + 0x7C) > 0) {
                    *(short *)(objdata + 0x70) = func_001F9850(*(int *)(data + 0x7C));
                }
            }
            *(unsigned char *)(objdata + 0x7A) = *(unsigned char *)(objdata + 0x7A) + 1;
            func_0022ED80(3, 0, (int)moby);
            func_L00_00251328(moby, 0, 0xFF, 0);
            *(short *)(data + 0x74) = 1;
        }
        st = *(unsigned char *)(moby + 0x20);
        if (st != 2) {
            break;
        }
        if (!(*(unsigned char *)(data + 0x7A) < *(unsigned char *)(data + 0x7B))) {
            func_L01_0026F090(*(unsigned char *)(moby + 0x21), 4);
            func_0022ED80(1, 0, (int)moby);
            *(short *)(data + 0x70) = *(unsigned short *)(data + 0x84);
            gv = D_L13_0015F6B0;
            func_001E9730_d(D_L13_001F5080, *(short *)(moby + 0xB2), *(short *)(moby + 0xA6), gv);
            moby[0x20] = 4;
            break;
        }
        if (*(int *)(data + 0x7C) < 0) {
            break;
        }
        if (func_001F9938(data + 0x70) != 0) {
            data[0x7A] = 0;
            *(short *)(data + 0x74) = 0;
            func_0022ED80(2, 0, (int)moby);
            func_L01_0026F090(*(unsigned char *)(moby + 0x21), 3);
            gv = D_L13_0015F6B0;
            func_001E9730_d(D_L13_001F50A0, *(short *)(moby + 0xB2), *(short *)(moby + 0xA6), gv);
            moby[0x20] = 3;
            break;
        }
        if (func_001F9938(data + 0x72) == 0) {
            break;
        }
        func_0022ED80(0, 0, (int)moby);
        r17 = func_001F9850(0x2D);
        r16 = func_001F9850(5);
        q = *(short *)(data + 0x70) / func_001F9850(0x28);
        r16 = r16 + q;
        if (r17 < r16) {
            *(short *)(data + 0x72) = func_001F9850(0x2D);
        } else {
            r16 = func_001F9850(5);
            q = *(short *)(data + 0x70) / func_001F9850(0x28);
            *(short *)(data + 0x72) = r16 + q;
        }
        break;

    case 3:
        func_L00_00251328(moby, 0x80, 0x80, 0x80);
        data[0x7A] = 0;
        *(float *)(data + 0x20) = (float)*(short *)(data + 0x24);
        *(short *)(data + 0x74) = 0;
        moby[0x20] = 1;
        break;

    case 4:
        if (*(unsigned char *)(data + 0x79) == 0 && *(short *)(data + 0x70) != -1) {
            if (func_001F9938(data + 0x70) != 0) {
                data[0x7A] = 0;
                *(short *)(data + 0x74) = 0;
                func_0022ED80(2, 0, (int)moby);
                func_L01_0026F090(*(unsigned char *)(moby + 0x21), 3);
                gv = D_L13_0015F6B0;
                func_001E9730_d(D_L13_001F50C0, *(short *)(moby + 0xB2), *(short *)(moby + 0xA6), gv);
                moby[0x20] = 3;
                break;
            }
            if (func_001F9938(data + 0x72) != 0) {
                func_0022ED80(0, 0, (int)moby);
                r17 = func_001F9850(0x2D);
                r16 = func_001F9850(5);
                q = *(short *)(data + 0x70) / func_001F9850(0x28);
                r16 = r16 + q;
                if (r17 < r16) {
                    *(short *)(data + 0x72) = func_001F9850(0x2D);
                } else {
                    r16 = func_001F9850(5);
                    q = *(short *)(data + 0x70) / func_001F9850(0x28);
                    *(short *)(data + 0x72) = r16 + q;
                }
            }
        }
        if (func_001F9938(data + 0x76) == 0) {
            break;
        }
        *(short *)(data + 0x76) = func_001F9850(0x14);
        if (*(unsigned char *)(data + 0x78) != 0) {
            func_L00_00251328(moby, 0, 0xFF, 0);
            data[0x78] = 0;
        } else {
            func_L00_00251328(moby, 0, 0, 0xFF);
            data[0x78] = 1;
        }
        break;

    default:
        break;
    }
}

/* NON_MATCHING func_L01_002F4290 -- src/overlays/shared/vendor_002B90A8.c
 * Best so far: SIZE ours 516 / retail 520, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   NearestAreaMarkerUpdate: first-frame init (copy pos, pick nibble from table, decrement counter), then find nea
 *   Best p5.c (508 vs 520). Left: nibble extract compiles srl instead of retail sra+andi, base (D_0013E633+0xE1D) 
 */
typedef int u128_2F4290 __attribute__((mode(TI)));
extern void func_0020D678(void *);
extern int func_L00_0025A778(void *, void *, int);
extern int func_00215570(void *arg0, int arg1);
extern float func_001F9D48(void *, void *);
extern short D_0015EE84;
extern char *D_L01_001B0C30[];
extern short D_L01_00161B80;
extern unsigned char D_0014171B[] NOT_SDA;
extern char D_0013E633[];

// Area marker update: picks nearest marker to the player and registers it.
void func_L01_002F4290(char *m) {
    char *d = *(char **)(m + 0x78);
    u128_2F4290 vec;
    char *src;
    float dist;
    int found;
    if (m[0x20] == 0) {
        if (d == 0) {
            func_0020D678(m);
            return;
        }
        m[0x30] = 0xFF;
        *(u128_2F4290 *)d = *(u128_2F4290 *)(m + 0x10);
        *(unsigned short *)(m + 0x34) |= 0x41;
        m[0x20] = 1;
        if (m[0xBC] == 0 && *(int *)&D_0015EE84 < 0x14) {
            int c = *(int *)&D_L01_00161B80;
            int nib = D_0014171B[0xA8F5 + *(int *)&D_0015EE84 * 16 + c / 2];
            if (c & 1) {
                nib = nib & 0xF;
            } else {
                nib = (nib >> 4) & 0xF;
            }
            *(int *)&D_L01_00161B80 = c + 1;
            m[0xBC] = *(int *)&D_L01_00161B80;
            *(int *)(d + 0x18) = *(int *)(d + 0x18) - nib * 5;
            if (*(int *)(d + 0x18) <= 0) {
                func_0020D678(m);
                return;
            }
        }
    } else {
        *(int *)&D_L01_00161B80 = 0;
    }
    if (*(int *)(D_0013E633 + 0xE1D + 0x1090) != 0 && *(int *)(D_0013E633 + 0xE1D + 0x10B8) == 0x1B) {
        src = (char *)(*(int *)(*(int *)(D_0013E633 + 0xE1D + 0x1090) + 0x78) + 0x10);
    } else {
        src = D_0013E633 + 0xE9D;
    }
    vec = *(u128_2F4290 *)src;
    found = 0;
    if (*(int *)(d + 0x10) != -1) {
        char *t = D_L01_001B0C30[*(int *)(d + 0x10)];
        found = func_L00_0025A778(&vec, t + 0x10, *(int *)t);
    }
    if (!found) {
        if (*(int *)(d + 0x14) != -1) {
            found = func_00215570(&vec, *(int *)(d + 0x14));
        }
        if (!found && *(long *)(d + 0x10) != -1) {
            return;
        }
    }
    dist = func_001F9D48(m + 0x10, &vec);
    if (dist < *(float *)(D_0013E633 + 0xE1D + 0x2044)) {
        *(char **)(D_0013E633 + 0xE1D + 0x2040) = m;
        if (dist < 20.0f) {
            *(float *)(D_0013E633 + 0xE1D + 0x2044) = dist;
            *(int *)(D_0013E633 + 0xE1D + 0x2048) = 1;
        }
    }
}

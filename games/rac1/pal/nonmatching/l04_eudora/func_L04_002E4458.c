/* NON_MATCHING func_L04_002E4458 -- src/overlays/l04_eudora/vendor_002CB800.c
 * Best so far: BYTES 5/552 (99.1% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   UpdateMoby_1190 (gate): 4-state switch. p1.c matches except 5 bytes: in case 0 the table lookup `(D_0014171B +
 */
extern float func_001F9D10(void *, void *);
extern void func_L00_0025B178(void *);
extern void func_L02_002E2110(void *);
extern int func_L00_002676E8(void *, void *);
extern int func_00215570(void *, int);
extern int func_L00_00267290(void *, void *);
extern void func_L00_00261848(int);
extern void func_L00_002512D8(int);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern float func_001FA748(float, float);
extern int func_0020BFC8(int, int);
extern char D_L04_00166FC0[];
extern char D_L04_0016CA70[];
extern int D_0015EE84 MACRO_ADDR;
extern short D_L04_0015F6A8;
extern unsigned char D_0014171B[];
extern char D_0013E633[];

// Updates the gate moby: checks the player's distance, then steps its four states.
void func_L04_002E4458(char *m) {
    char *data = *(char **)(m + 0x78);
    float a[4];
    float b[4];
    int g;
    int c;
    if (*(unsigned char *)(m + 0x31) != 0) {
        if (func_001F9D10(m + 0x10, D_L04_00166FC0) < 30.0f) {
            func_L00_0025B178(m);
            *(unsigned char *)(m + 0x7F) = 0x18;
        }
    }
    func_L02_002E2110(m);
    switch (*(unsigned char *)(m + 0x20)) {
    case 0:
        c = *(unsigned char *)(m + 0xB0);
        g = D_0015EE84;
        if ((D_0014171B + 0xAA35)[g * 16 + c] == 0xFF) goto done;
        *(unsigned char *)(m + 0x20) = 1;
        func_L00_002676E8(m, data);
        break;
    case 1:
        if (func_00215570(D_0013E633 + 0xE9D, *(int *)(data + 0x140)) != 0) {
            *(float *)(data + 0xC) = 255.0f;
        } else {
            *(float *)(data + 0xC) = 1.0f;
        }
        if (func_L00_00267290(m, data) != 0) {
            *(unsigned char *)(m + 0x20) = 2;
        }
        break;
    case 2:
        if (*(short *)(data + 4) == 2) {
            func_L00_00261848(6);
            func_L00_002512D8(*(unsigned char *)(m + 0xB0));
            func_L00_001FF4B0(a, m + 0xC0, 2.0f);
            func_001F9BD8(a, a, m + 0x10);
            qcopy(b, m + 0x40);
            b[2] = func_001FA748(b[2], 3.14159f);
            qcopy(D_L04_0016CA70, a);
            qcopy(D_L04_0016CA70 + 0x10, b);
            D_L04_0016CA70[0x3A] = 1;
            func_0020BFC8(0, -1);
        done:
            *(unsigned char *)(m + 0x20) = 3;
            *(unsigned short *)(m + 0x34) |= 0x41;
            *(int *)(m + 0x94) = 0;
        } else if (*(int *)&D_L04_0015F6A8 != 2) {
            func_L00_00267290(m, data);
        }
        break;
    case 3:
        *(int *)(m + 0x94) = 0;
        *(unsigned short *)(m + 0x34) |= 0x41;
        break;
    }
}

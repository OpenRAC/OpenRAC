/* NON_MATCHING func_L00_0029BA20 -- src/overlays/shared/update_0029B6A0.c
 * Best so far: SIZE ours 1196 / retail 1208, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Vendor menu open (ReRAC OpenVendorMenu), 1208 bytes, shared across 19 levels. Best so far p4.c: 1196 bytes (12
 *   Differences left: the lui hi/%lo bases for D_L00_001CA7C0 and D_L00_00166D80 are scheduled differently (retail
 */
extern int D_L00_001CA7C0[];
extern char D_L00_001CA9E0[];
extern float D_L00_00161128;
extern float D_L00_00161120 MACRO_ADDR;
extern char D_L00_00166D80[];
extern char D_L00_00161190[];
extern char D_L00_001611A0[];
extern char D_L00_00180350[];
extern char D_L00_00180360[];
extern char D_L00_00165F80[];
extern char D_L00_00173F00[];
extern char D_L00_001C43B0[];
extern char D_0013D50F[];
extern char D_0013E633[];
extern int D_0015EE88 MACRO_ADDR;
extern int D_0015EE98 MACRO_ADDR;
extern float D_0015EE60 MACRO_ADDR;
extern float D_L00_0015F4FC MACRO_ADDR;
extern int D_L00_0015F6BC MACRO_ADDR;
extern short D_L00_0015F6A8;
extern int D_L00_0015F500 MACRO_ADDR;
extern int D_L00_00161F18;
extern int D_L00_00161F20;
extern int D_L00_00161F30;
extern int D_L00_00161F24;
extern int D_L00_00161248;
extern int D_L00_0016124C;
extern void func_L00_0023A658(void);
extern void func_L00_0023A690(void);
extern void func_L00_0023A788(void);
extern void func_L00_0023B0F8(void);
extern void func_L00_00236830(void);
extern void func_L00_0023B140(void);
extern void func_001F99B0(void *, int, int);
extern void *func_0020D348_m(int) __asm__("func_0020D348");
extern void func_001F9BC0(void *);
extern void func_L00_00251E30(void *);
extern void func_0012E528(int);
extern void func_00216EF0(int);
extern int func_0012DDC0(void);
extern void func_L00_0029B770(int);
extern void func_L00_0023B890(void);
extern int func_001FFB38(int, int, int, int, int, int, int);
extern int func_0022ED80(int, int, int);
extern void func_001F4E08(int);
extern void func_L00_00203FB8(void);
extern int func_L00_00222B80(int, int);
extern void func_L00_00233868(void);
extern void func_00213D28(void *, int, int);
extern void func_001F9EC0(void *, void *, void *);
extern void func_001F9BD8(void *, void *, void *);
extern float func_001FA748(float, float);
extern void func_L00_002EBF50(void *, void *, int, int, int);
extern void func_L00_002EBE88(void *);
extern void func_L00_002EBEE0(void *);
extern void func_0020D960(char *, int, unsigned char *);

/* Vendor menu opened on triangle (ReRAC: OpenVendorMenu): sets up the menu state and its moby records. */
void func_L00_0029BA20(int arg) {
    int *base;
    char *obj;
    int ee88;
    int i;
    unsigned char *rec;
    char *o;
    int v;

    base = D_L00_001CA7C0;
    func_001F99B0(D_L00_001CA7C0, 0, 0x220);
    func_001F99B0(D_L00_001CA9E0, 0, 0x40);
    obj = (char *)arg;
    ee88 = D_0015EE88;
    if (arg == 0) {
        o = (char *)func_0020D348_m(0xB);
        obj = o;
        *(unsigned short *)(o + 0x32) = 0x40;
        *(float *)(o + 0x10) = *(float *)((char *)&D_L00_00166D80 + 0x140) + D_L00_00161120;
        *(float *)(o + 0x14) = *(float *)((char *)&D_L00_00166D80 + 0x144);
        *(float *)(o + 0x18) = *(float *)((char *)&D_L00_00166D80 + 0x148) - D_L00_00161128;
        func_001F9BC0(o + 0x40);
        *(float *)(o + 0x48) = 3.1415927f;
        func_L00_00251E30(o);
        o[0x20] = 3;
        base[0x10] = 1;
        ee88 = D_0015EE88;
    }
    v = ee88 - 1;
    base[0x0E] = v;
    if (v < 0) base[0x0E] = 0;
    base[0x11] = 0;
    base[0x12] = -1;
    base[0x14] = -1;
    base[0x15] = -1;
    base[0x16] = 0;
    D_L00_00161F18 = 0;
    D_L00_00161F20 = 0;
    base[0x17] = 0;
    D_L00_00161F30 = 0;
    func_0012E528(0x1D);
    D_L00_00161F24 = 0;
    func_00216EF0(0);
    func_0012DDC0();
    func_L00_0029B770(base[0x10] != 0);
    {
        int h = base[0x84];
        int half = h / 2;
        int sel = h > 2 ? half : 1;
        base[0x16] = half;
        base[0x13] = (sel - half) * 0x28;
    }
    func_L00_0023B890();
    base[0x15] = func_001FFB38(0x12, 0x754E, (int)func_L00_0023A658, (int)func_L00_0023A690,
                               (int)func_L00_0023A788, (int)&D_0015EE98, 0x989680);
    if (*(int *)((char *)base + base[0x16] * 0x14 + 0xD4) == 1) {
        int v5 = *(int *)(D_L00_00173F00 + 0xD0);
        char *r = D_L00_001C43B0 + v5 * 0x18;
        base[0x14] = func_001FFB38(0x30, v5 + 0xEA60, (int)func_L00_0023B0F8, (int)func_L00_00236830,
                                   (int)func_L00_0023B140, (int)(D_0013D50F + 0x21 + v5 * 4),
                                   *(unsigned short *)(r + 0xE));
    }
    D_L00_0015F6BC = 1;
    {
        int *dd = (int *)D_L00_00173F00;
        base[5] = dd[1] + 0x60000;
        base[4] = dd[2] + 0x60000;
    }
    D_L00_001CA7C0[0] = 0;
    base[2] = 0;
    base[7] = (int)obj;
    func_0022ED80(3, 0, (int)obj);
    if (base[0x10] == 0) func_001F4E08(4);
    func_L00_00203FB8();
    D_L00_0015F4FC = 1.0f;
    *(int *)&D_L00_0015F6A8 = 5;
    D_L00_0015F500 = 0;
    func_L00_00222B80(0x64, 1);
    D_0013E633[0xE1D + 0x20A5] = 1;
    D_0013E633[0xE1D + 0x20AC] = 1;
    func_L00_00233868();
    if (base[0x10]) {
        func_00213D28(obj, 3, 0);
    } else {
        func_00213D28(obj, 2, 0);
    }
    *(float *)(obj + 0x58) = D_0015EE60 * 0.5f;
    if (base[0x10] == 0) {
        func_001F9EC0((char *)base + 0x60, &D_L00_00161120, obj + 0xC0);
        func_001F9BD8((char *)base + 0x60, (char *)base + 0x60, obj + 0x10);
        *(float *)((char *)base + 0x78) = func_001FA748(*(float *)(obj + 0x48), 3.1415927f);
        func_L00_002EBF50((char *)base + 0x60, (char *)base + 0x70, 1, 0, 0);
        func_L00_002EBE88((char *)base + 0x60);
        func_L00_002EBEE0((char *)base + 0x70);
    }
    func_001F9EC0(D_L00_00180350 + 0x0, &D_L00_00161190, obj + 0xC0);
    qcopy(D_L00_00180350 - 0x10, D_L00_001611A0);
    qzero(D_L00_00180350 + 0x20);
    qzero(D_L00_00180360);
    i = 0;
    rec = (unsigned char *)D_L00_00165F80;
    do {
        if (rec[1] == 0) func_0020D960(obj, i + 0x14, rec);
        i++;
        rec += 0x40;
    } while (i < 4);
    if (base[0x84] < 8) {
        int t = 7 - base[0x84];
        *(float *)(D_L00_00165F80 + 0xF4) = (float)(-0x681 * t);
        *(float *)(D_L00_00165F80 + 0xB4) = (float)(t * 0x681);
        *(float *)(D_L00_00165F80 + 0x34) = (float)(t * 0x681);
        *(float *)(D_L00_00165F80 + 0x74) = (float)(-0x681 * t);
    } else {
        *(int *)(D_L00_00165F80 + 0xF4) = 0;
        *(int *)(D_L00_00165F80 + 0x34) = 0;
        *(int *)(D_L00_00165F80 + 0x74) = 0;
        *(int *)(D_L00_00165F80 + 0xB4) = 0;
    }
    D_L00_00161248 = 0;
    D_L00_0016124C = 0;
}

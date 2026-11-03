typedef int TI128 __attribute__((mode(TI)));

extern void func_0029FC20(char *, int, char *);
extern void func_00282D30(char *, char *, float);
extern void func_002A05A8(char *);
extern void func_002A0F48(char *);
extern void func_00283678(char *, char *);
extern void func_002AB028(char *);
extern void func_002A10E8(char *);

void func_002AF1D0(char *a0, char *a1, int a2, int a3) {
    TI128 v[4];

    func_0029FC20(a0, a2, (char *)&v[0]);
    *(TI128 *)(a1 + 0x10) = v[3];
    if (a3 & 1) {
        func_00282D30((char *)&v[0], (char *)&v[0], -1.0f);
    }
    if (a3 & 2) {
        func_00282D30((char *)&v[1], (char *)&v[1], -1.0f);
    }
    if (a3 & 4) {
        func_00282D30((char *)&v[2], (char *)&v[2], -1.0f);
    }
    func_002A05A8(a1);
    if ((*(unsigned short *)(a1 + 0x34) & 4) == 0) {
        func_002A0F48(a1);
    }
    func_00283678(a1 + 0xC0, (char *)&v[0]);
    func_002AB028(a1 + 0xC0);
    func_002A10E8(a1);
    if (*(unsigned short *)(a0 + 0x34) & 1) {
        *(unsigned short *)(a1 + 0x34) |= 0x41;
    } else {
        *(unsigned short *)(a1 + 0x34) &= 0xFFBE;
    }
    *(unsigned short *)(a1 + 0x34) |= 6;
}

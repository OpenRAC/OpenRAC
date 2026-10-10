extern char D_0013E650[];
extern int D_0015EEE8;
extern int D_0015EEEC;
extern int D_0015EEF0;
extern void func_0012DB68(void);
extern void func_0012E348(int, int);
extern void func_0012E380(int);
extern void func_0012E3C0(int, int);
extern void func_0012E3F8(int, int, int);
extern void func_0012F0E8(int, int);
extern void func_002161E0(int);

/* SoundMasterVolumeInit(void): clears the volume table at D_0013E650 (64
   bytes, two words at +0x40/+0x44, then one word at +0x70 and a byte at
   +0x74 of each 0x70-byte record up to +0xD20), runs the sound setup calls,
   then derives the ten-step volume words at +0x48..+0x5C from D_0015EEF0
   and sets the levels with func_0012E348. */
void func_0022DBE8(void)
{
    char *b = D_0013E650;
    char *p;
    char *end;
    int *w;
    int i;
    int v;
    int e;
    int a;
    int c7;

    for (i = 0; i < 4; i++) {
        w = (int *)(b + i * 0x10);
        w[0] = 0;
        w[1] = 0;
        w[2] = 0;
        w[3] = 0;
    }
    *(int *)(b + 0x40) = 0;
    *(int *)(b + 0x44) = 0;
    p = b;
    end = b + 0xD20;
    *(int *)(p + 0x70) = 0;
    do {
        *(unsigned char *)(p + 0x74) = 0;
        p += 0x70;
        if (!(p < end)) {
            break;
        }
        *(int *)(p + 0x70) = 0;
    } while (1);

    func_0012DB68();
    func_0012E380(D_0015EEE8 == 0);
    func_0012E3C0(0, 1);
    func_0012F0E8(2, 4);
    func_0012E3F8(1, 0x18, 0x2F);
    func_0012E3F8(2, 0x18, 0x2F);
    func_0012E3F8(4, 0x18, 0x2F);

    v = D_0015EEF0;
    e = D_0015EEEC;
    a = (v * 8) / 10;
    c7 = (v * 8 - v) / 10;
    *(int *)(b + 0x4C) = e;
    *(int *)(b + 0x48) = a;
    *(int *)(b + 0x50) = a;
    *(int *)(b + 0x54) = c7;
    *(int *)(b + 0x58) = c7;
    *(int *)(b + 0x5C) = v;
    func_002161E0(10);
    func_0012E348(0, a);
    func_0012E348(1, e);
    func_0012E348(2, a);
    func_0012E348(3, c7);
    func_0012E348(4, c7);
    func_0012E348(5, v);
}

/* Advances the stage cursor by 0x10, mirrors the clock, builds the view matrix and restarts the two render packets. */
extern void func_0011AEA0(int);
extern void func_002FC468(unsigned int, void *, unsigned int);
extern int D_001A72D4 __attribute__((sda));
extern int D_001A72D0 __attribute__((sda));
extern unsigned char *D_001B22A8 __attribute__((sda));
extern unsigned char *D_001B219C __attribute__((sda));
extern char D_001B5340[];
extern int D_00215E80[];
extern void func_00283388(float *);
extern void func_00282D30(void *, void *, float);
extern void func_00283788(char *, char *, char *);
extern void func_002EF480(void);
extern void func_002EF1E8(void);
extern unsigned int func_00282BA8(void *, unsigned int, unsigned int);

void func_002EF3B0(void)
{
    float m[16];
    unsigned char *p = D_001B22A8;

    D_001B219C = p;
    p += 0x10;
    D_001B22A8 = p;
    D_001A72D0 = D_001A72D4;
    func_00283388(m);
    func_00282D30(m + 12, D_001B5340, -1024.0f);
    m[15] = 1.0f;
    func_00283788((char *)m, D_001B5340 - 0x100, (char *)m);
    func_002FC468(5, m, 4);
    func_002FC468(0x14D, m, 4);
    func_0011AEA0(0);
    func_002EF480();
    func_002EF1E8();
    func_00282BA8(D_00215E80, 0x3000, 0x40);
}

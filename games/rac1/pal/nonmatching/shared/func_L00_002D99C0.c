/* NON_MATCHING func_L00_002D99C0 -- src/overlays/shared/vendor_002D9438.c
 * Best so far: SIZE ours 828 / retail 832, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Second pass (lb1/p18, runs p10-p16): p13.c is best at SIZE 824 vs 832. It now gets the flag update right
 *   (`a = func_001F9850(0x30); fl = s.flag; if (a < v) fl = 1; s.flag = fl;` gives lw/movn/sw after the call as in
 *   Still differs: retail copies p+0x58 and p+0x60 into s-regs from a temp (addiu $2; daddu $17,$2,$0: +2 instrs),
 *   retail s-reg numbering of the loop givs is $22=i*2, $23=p+0x58+2i (ours swapped), retail ph[i] is addu base-fi
 *   cnt reloads into $4 not $2. An explicit q++ pointer (p16) spills p and is worse (844). Stopped: allocator/giv 
 *   Run 9 (p17: ps/ph assigned before the 9938 call, outside the if) is worse (800). An alignment scan of p13 vs r
 *   (build-sn/try/alignscan.py func_L00_002D99C0, run through docker) shows the only structural gap is the two ext
 *   (daddu $s1,$v0 / daddu $s2,$v1) of p+0x58 and p+0x60 that retail keeps live across the calls; everything else 
 */
typedef struct { float x, y, z, w; } Vy10 __attribute__((aligned(16)));
typedef struct { Vy10 a; Vy10 b; int cnt; int flag; } Sy10;
extern char D_L00_00166EC0[] NOT_SDA;
extern char D_L00_001B2400[] NOT_SDA;
extern short D_L00_00161B28;
extern short D_L00_00161B4C;
extern short D_L00_00161B50;
extern short D_L00_00161B54;
extern short D_L00_00161B58;
extern short D_L00_00161B60;
extern short D_L00_00161B70;
extern short D_L00_00161B80;
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_001F9938(void *);
extern int func_001F9850(int);
extern float func_001FA888(int);
extern float func_001F9B88(float);
extern int func_001FA8A8(int, int, float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern char *func_L00_00273E08(void *, int, int, int, int, int, int, float);

// Advance the four colour phases of a moby and emit two effect pieces for each one that is live.
int func_L00_002D99C0(char *m) {
    Sy10 s;
    char *p = *(char **)(m + 0x78);
    char *pos = m + 0x10;
    float z, f, g;
    short *ps;
    float *ph;
    int i = 0;
    int col, c2;
    short v;
    char *gd;
    char *o;
    func_001F9BF0(&s, D_L00_00166EC0, pos);
    s.flag = 0;
    func_L00_001FF4B0(&s, &s, -0.3f);
    func_L00_001FF4B0(&s.b, &s, 0.1f);
    s.cnt = 3;
    func_001F9BD8(&s, &s, pos);
    z = s.a.z;
    do {
        if (!func_001F9938(p + 0x58 + i * 2)) {
            ps = (short *)(p + 0x58);
            ph = (float *)(p + 0x60);
            v = ps[i];
            if (func_001F9850(0x30) < v) s.flag = 1;
            f = ph[i] + *(float *)((char *)&D_L00_00161B60 + i * 4);
            ph[i] = f;
            if (f >= 255.0f) ph[i] = f - 255.0f;
            else if (f <= 0.0f) ph[i] = f + 255.0f;
            g = func_001FA888(func_001F9850(0xFF) - ps[i]) / (float)func_001F9850(0xFF);
            col = func_001FA8A8(*(int *)&D_L00_00161B4C, *(int *)&D_L00_00161B50, func_001F9B88(0.5f - g));
            g = 1.0f - (float)ps[i] / func_001FA888(func_001F9850(*(int *)&D_L00_00161B28));
            f = func_001FA888(col >> 24) * (1.0f - g);
            col = (col & 0xFFFFFF) | (func_001FA898_r(f) << 24);
            s.a.z = z + *(float *)((char *)&D_L00_00161B80 + i * 4);
            o = func_L00_00273E08(&s, col, func_001FA898_r(ph[i]) & 0xFF, *(int *)&D_L00_00161B58, 0, 2, 1, *(float *)((char *)&D_L00_00161B70 + i * 4) * g);
            gd = D_L00_001B2400;
            if (o != 0) o[2] = **(char **)(gd + 0xF8);
            c2 = col | 0xFFFFFF;
            o = func_L00_00273E08(&s, c2, func_001FA898_r(ph[i]) & 0xFF, *(int *)&D_L00_00161B58, *(int *)&D_L00_00161B54, 2, 1, *(float *)((char *)&D_L00_00161B70 + i * 4) * (g * 0.7f));
            gd = D_L00_001B2400;
            if (o != 0) o[2] = **(char **)(gd + 0xF8);
            func_001F9BD8(&s, &s, &s.b);
        }
        i++;
    } while (--s.cnt >= 0);
    return s.flag;
}

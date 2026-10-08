/* NON_MATCHING func_L00_001EBDA0 -- src/overlays/shared/camera_001EB508.c
 * Best so far: BYTES 4/272 (98.5% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern char *D_L00_00166F00;
extern char D_L00_00167250_u[] __asm__("D_L00_00167250");
extern int D_L00_00169990[];
typedef struct { char pad0[0xC]; void (*fn)(char *); char pad10[4]; } CamEnt;   /* 0x14 bytes */
extern CamEnt D_L00_001EAC00_u[] __asm__("D_L00_001EAC00");
extern void func_001EC270_u(char *) __asm__("func_001EC270");
extern int func_001EC5B8(char *, char *);
extern void func_001EC210(char *);
extern void func_L00_001EB890(char *);
extern void func_L00_001EB508_v(void) __asm__("func_L00_001EB508");

int func_L00_001EBDA0(void) {
    char *best = D_L00_00166F00;
    char *slot;
    int *flag;
    int i;
    int found = 0;
    void (*fn)(char *);
    float *p;
    float *q;
    func_001EC270_u(best);
    slot = D_L00_00167250_u;
    flag = D_L00_00169990;
    for (i = 0x2F; i >= 0; i--) {
        if (*flag != 0) {
            if (slot != best && func_001EC5B8(slot, best) != 0) {
                best = slot;
                found = 1;
            }
        }
        slot += 0xA0;
        flag++;
    }
    if (found) {
        func_L00_001EB890(best);
    }
    fn = D_L00_001EAC00_u[*(short *)(best + 0x8C)].fn;
    func_001EC210(best);
    if (fn) {
        fn(best);
    }
    p = (float *)(best + 0x30);
    q = (float *)(best + 0x64);
    q[0] = p[0];
    q[1] = p[1];
    q[2] = p[2];
    func_L00_001EB508_v();
    return -1;
}

/* NON_MATCHING func_L07_00319080 -- src/overlays/l07_umbris/vendor_00313D28.c
 * Best so far: SIZE ours 420 / retail 428, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Effect reset: if flag D_L07_00161BFC set, clears it, builds two vectors, copies pos into D_L07_0020DDC0, runs 
 *   Best p5 (size 428 = retail, 89 bytes differ): all s-registers rotated (retail: counter and m share s0, p17=s1,
 *   MACRO_ADDR on D_L07_00161C20 produced the retail nop-in-delay-slot form; for(i=0;i<9;i++) let counter share m'
 */
typedef int u128 __attribute__((mode(TI)));
typedef struct { short idx; short pad; float f; int pad2[3]; char *moby; } Ent;
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9BC0(void *);
extern float func_00214158(void);
extern void func_002156E0(void *, void *, void *, float);
extern Ent D_ents[] __asm__("D_L07_0020CB00");
extern char D_dst[] __asm__("D_L07_0020DDC0");
extern char D_src[] __asm__("D_L07_0020DDB0");
extern short D_cnt[] __asm__("D_L07_0020DE88");
extern char D_L07_00161C20[];
extern char D_L07_00161C30[];
extern float D_L07_00161C40;
extern short D_L07_00161BFC;
extern short D_L07_00161C00;
extern short D_L07_00161C0C;
extern short D_L07_00161C10;
extern short D_L07_00161C14;

// Resets the effect state when its flag is set: rebuilds the table, rotates a vector, clears entries.
void func_L07_00319080(char *m, void *pos) {
    if (*(int *)&D_L07_00161BFC != 0) {
        float a[4];
        float b[4];
        float c[4];
        float d[4];
        char *p17, *p18;
        short *p19;
        int i;
        float r;
        char *dst;
        char *src;
        short *cp;
        *(int *)&D_L07_00161BFC = 0;
        func_L00_001FF4B0(a, m + 0xC0, 2.8f);
        func_L00_001FF4B0(b, m + 0xE0, 1.0f);
        dst = D_dst;
        *(u128 *)dst = *(u128 *)pos;
        src = D_src;
        cp = D_cnt;
        D_cnt[0] = 0;
        p17 = dst + 0x10;
        p18 = src + 0x10;
        p19 = cp + 1;
        for (i = 0; i < 9; i++) {
            func_001F9BD8(p17, p18, a);
            p18 += 0x10;
            p17 += 0x10;
            *p19 = 0;
            p19++;
        }
        func_001F9BC0(D_L07_00161C20);
        *(u128 *)c = 0;
        *(u128 *)d = 0;
        c[3] = 1.0f;
        c[2] = 0.1f;
        c[1] = 1.0f;
        d[0] = 1.0f;
        r = func_00214158();
        func_002156E0(D_L07_00161C30, c, d, r);
        D_L07_00161C40 = r;
        *(float *)&D_L07_00161C0C = 0.006f;
        *(int *)&D_L07_00161C10 = 0;
        *(int *)&D_L07_00161C14 = 0;
        if (*(int *)&D_L07_00161C00 != 0) {
            *(int *)&D_L07_00161C00 = 0;
            for (i = 199; i >= 0; i--) {
                D_ents[i].moby = 0;
            }
        }
    }
}

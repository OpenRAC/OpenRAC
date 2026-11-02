/* func_002D1590 - first C reconstruction, USA v1.01, 28 Oct 2026.
   The flags were traced to D_001ABA30..D_001ABA38 (the first and last use
   $gp-relative storage). `python3 tools/check.py func_002D1590 --diff`
   reports 296 bytes vs. 308 under the default `-G0` profile, 0/28. All retail
   conditions and flag writes are represented, but branch layout and
   address/register scheduling differ. A temporary `-G8` sibling-unit check
   (28 Oct 2026) measured 288/308, 0/28; it selects GP-relative stores for the
   flag bytes but leaves the condition ordering and flag-address
   materialisation substantially different. A 29 Oct 2026 -G8 probe that flattened
   the outer gate into an early return measured 296/308, 0/28, so it regressed
   against the retained 288-byte -G8 form. Marker remains; no match is claimed. */
extern int D_001A8CFC __attribute__((section(".data")));
extern unsigned char D_001A7A0A __attribute__((section(".data")));
extern unsigned char D_001A7BEA __attribute__((section(".data")));
extern unsigned char D_001395E9 __attribute__((section(".data")));
extern unsigned char D_001A7BEC __attribute__((section(".data")));
extern unsigned char D_001A7BF3 __attribute__((section(".data")));
extern unsigned char D_001A7C02 __attribute__((section(".data")));
extern unsigned char D_001A7BFE __attribute__((section(".data")));
extern unsigned char D_001A7BFF __attribute__((section(".data")));
extern unsigned char D_001395B8[] __attribute__((section(".data")));
extern unsigned char D_001ABA30 __attribute__((sda));
extern unsigned char D_001ABA31 __attribute__((section(".data")));
extern unsigned char D_001ABA32 __attribute__((section(".data")));
extern unsigned char D_001ABA33 __attribute__((section(".data")));
extern unsigned char D_001ABA34 __attribute__((section(".data")));
extern unsigned char D_001ABA35 __attribute__((section(".data")));
extern unsigned char D_001ABA36 __attribute__((section(".data")));
extern unsigned char D_001ABA37 __attribute__((section(".data")));
extern unsigned char D_001ABA38 __attribute__((section(".data")));
extern unsigned int D_001AA438 __attribute__((sda));
int func_002D1590(void) {
    if (D_001A8CFC && D_001A7A0A) {
        if (D_001A7BEA && D_001395E9)
            D_001ABA30 = 1;
        if (D_001A7BEC && (D_001395B8[0x4D] || D_001395B8[0x52]))
            D_001ABA31 = 1;
        if (D_001A7BF3 && D_001395B8[0x57]) D_001ABA32 = 1;
        if (D_001A7BF3 && D_001395B8[0x3D]) D_001ABA33 = 1;
        if (D_001A7BF3 && D_001395B8[0x5C]) D_001ABA34 = 1;
        if (D_001A7C02) D_001ABA35 = 1;
        if (D_001A7BFE) D_001ABA36 = 1;
        if (D_001A7BFF) D_001ABA37 = 1;
        if (D_001AA438) D_001ABA38 = 1;
    }
    return 0;
}

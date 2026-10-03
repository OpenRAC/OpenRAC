/* NON_MATCHING func_L11_003173C8 -- src/overlays/l11_pokitaru/vendor_00312BD8.c
 * Best so far: SIZE ours 304 / retail 308, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Turns a moby toward a point (func_L00_0025CE58), then if in range 8..22 and state bytes equal, writes a speed 
 *   p5.c matches every instruction except one: SIZE 304 vs 308. Retail reads D_0015EE6C twice the lui way (macro f
 *   The assembler takes the symbol size from the LAST `.extern` for the name (p2/p3: `.extern ,4` last -> all lui 
 *   Other findings: state bytes at 0x52/0x53 are unsigned char; func_0020D830 takes the moby (char *).
 */
extern float func_L00_001FF860(float, float);
extern float func_001FA748(float, float);
extern float func_L00_0025CE58(float *p, float *v, float a, float b, float c, float d);
extern float func_0020D830(char *);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern int func_L00_00259B88(char *, char *, float *, char *, float);
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C_m __asm__("D_0015EE6C") MACRO_ADDR;
extern short D_0015EE6C_s __asm__("D_0015EE6C");
extern short D_L11_0016238C;

/* turns a moby toward a point and, while in range, spawns a puff */
void func_L11_003173C8(char *m, float *p) {
    char *data = *(char **)(m + 0x78);
    float v[3];
    char tmp[16];
    float ang = func_L00_001FF860(p[0] - *(float *)(m + 0x10), p[1] - *(float *)(m + 0x14));
    float t = func_001FA748(ang, *(float *)(data + 0x24C));
    float dist;
    float b = D_0015EE70 * 8.726646f;
    func_L00_0025CE58((float *)(m + 0x48), (float *)(data + 0x250), t, b, b, D_0015EE6C_m * 12.566371f);
    dist = func_0020D830(m);
    if (*(unsigned char *)(m + 0x52) == *(unsigned char *)(m + 0x53)) {
        if (8.0f < dist && dist < 22.0f) {
            *(float *)(data + 0x1A4) = *(float *)&D_L11_0016238C * *(float *)&D_0015EE6C_s;
            v[0] = func_001F9F90(*(float *)(m + 0x48)) * 2;
            v[1] = func_001F9FA8(*(float *)(m + 0x48)) * 2;
            v[2] = 0;
            func_L00_00259B88(m, data + 0x180, v, tmp, 1.0f);
        }
    }
}

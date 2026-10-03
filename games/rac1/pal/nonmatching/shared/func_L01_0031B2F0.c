/* NON_MATCHING func_L01_0031B2F0 -- src/overlays/shared/vendor_0031AD00.c
 * Best so far: SIZE ours 344 / retail 348, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L01_0031B2F0 (reverb box sound update): vector from listener to box, transformed; if all |b[i]| <= 1 star
 *   p0.c is right except the size: SIZE 344/348. Retail keeps the two `call func_L01_002A2C90; sb $0,3($16)` tails
 *   Would unblock: whatever defeats cross-jump of the identical call+store tail in retail's source (unknown; maybe
 */
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9EC0(void *, void *, void *);
extern float func_001F9B88(float);
extern float func_001FA888(int);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_L01_002A2C90(int a, int b, int c, int d, int e);
extern char D_0013E633[];

/* reverb box sound update: starts or stops the sound as the listener enters or leaves the box */
void func_L01_0031B2F0(char *moby) {
    float a[4];
    float b[4];
    unsigned char *data = *(unsigned char **)(moby + 8);
    func_001F9BF0(a, D_0013E633 + 0xE9D, moby + 0x40);
    a[3] = 0;
    func_001F9EC0(b, a, moby + 0x50);
    if (func_001F9B88(b[0]) <= 1.0f && func_001F9B88(b[1]) <= 1.0f && func_001F9B88(b[2]) <= 1.0f) {
        float t = (b[0] + 1.0f) * 0.5f;
        int v = func_001FA898_r(func_001FA888(*(int *)(data + 4)) * t);
        int w = *(int *)(data + 4);
        if (!(w < v)) w = v;
        func_L01_002A2C90((int)moby, data[0], w, data[1], data[2]);
        data[3] = 1;
    } else if (data[3] != 0) {
        if (b[0] <= 0.0f) {
            func_L01_002A2C90((int)moby, 0, 0, 0, 0);
            data[3] = 0;
        } else {
            func_L01_002A2C90(0, data[0], *(int *)(data + 4), data[1], data[2]);
            data[3] = 0;
        }
    }
}

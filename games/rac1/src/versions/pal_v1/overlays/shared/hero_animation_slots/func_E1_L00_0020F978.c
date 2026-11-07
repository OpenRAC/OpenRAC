extern int func_L00_002347B8(void);
extern int D_E1_0015F5A4 MACRO_ADDR;
int func_E1_L00_0020F978(void) {
    unsigned char *h;
    int x;
    int y;

    x = func_L00_002347B8();
    if (x == 0) {
        x = D_E1_0015F5A4;
        if (x == 0) {
            h = (unsigned char *)g_hero;
            if (h[0x20A4] == 0) {
                y = *(int *)(h + 0x208C);
                if (y < 3u || y == 4) {
                    return h[0x12E4] == 0;
                }
            }
        }
    }
    return 0;
}

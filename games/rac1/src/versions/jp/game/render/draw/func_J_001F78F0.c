void func_J_001F78F0(int color, float x, float y, char *str, int n, float scale) {
    int mode = GetEffectTex(1);

    ((void (*)(float, float, int, char *, int, float, int, void *))func_J_001F75A8)(x, y, color, str, n, scale, mode,
                                                                                    normal_font_metrics);
}

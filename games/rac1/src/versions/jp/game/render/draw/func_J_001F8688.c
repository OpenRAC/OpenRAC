extern void func_J_001F81F8(void *win_p, long a1, void *text_p, int len, long a4, unsigned char *glyphs);

void func_J_001F8688(void *a, void *b, void *c, void *d) {
    int mode = GetEffectTex(1);

    func_J_001F81F8(a, b, c, d, mode, normal_font_metrics);
}

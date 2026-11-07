extern void func_J_001F6EE8(void *, void *, void *, void *, void *, long, unsigned char *);

int func_J_001F7B40(int a, int b, int c, int d, int e) {
    int p = a - (measure_text_width_regular((unsigned char *)d, e) >> 1);
    long mode = GetEffectTex64(1);

    func_J_001F6EE8((void *)p, (void *)b, (void *)c, (void *)d, (void *)e, mode, normal_font_metrics);
    return p;
}

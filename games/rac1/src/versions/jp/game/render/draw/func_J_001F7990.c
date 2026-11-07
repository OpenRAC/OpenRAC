extern void func_J_001F6EE8(void *, void *, void *, void *, void *, long, unsigned char *);

void func_J_001F7990(char *a, void *b, void *c, unsigned char *d, int e) {
    char *p = a - measure_text_width_regular(d, e);
    long mode = GetEffectTex64(1);

    func_J_001F6EE8(p, b, c, d, (void *)e, mode, normal_font_metrics);
}

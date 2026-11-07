extern int func_J_001F6B60(unsigned char *str, int count, signed char *table);

int measure_text_width_small(unsigned char *arg0, int arg1) {
    return func_J_001F6B60(arg0, arg1, (signed char *)small_font_metrics);
}

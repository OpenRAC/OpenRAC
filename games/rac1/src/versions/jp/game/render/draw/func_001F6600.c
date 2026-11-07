extern int func_J_001F6B60(unsigned char *str, int count, signed char *table);

int measure_text_width_regular(unsigned char *arg0, int arg1) {
    return func_J_001F6B60(arg0, arg1, (signed char *)normal_font_metrics);
}

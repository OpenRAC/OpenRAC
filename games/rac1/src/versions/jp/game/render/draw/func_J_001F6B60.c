/* NTSC-J measure_text_width: dakuten marks 0xBD and 0xBE take their width from the character before them. */
int func_J_001F6B60(unsigned char *str, int count, signed char *table) {
    int width = 0;
    int i = 0;
    unsigned char *p;

    if (count != 0 && *str != 0) {
        p = str;
        do {
            int c = *p;
            int w = table[(c << 2) + 3];

            if (w != 0) {
                int is_mark = (unsigned char)c == 0xBD || (unsigned char)c == 0xBE;
                int mark = c;

                if (is_mark) {
                    int prev = p[-1];
                    int sum;

                    c = table[prev * 4 + 0x203] - table[prev * 4 + 3] + 4;
                    if (mark == 0xBE) {
                        sum = width + 1;
                        sum += c;
                    } else {
                        sum = width + c;
                    }
                    width = sum;
                } else {
                    width += w;
                }
            }
            i++;
            if (i == count) {
                break;
            }
            p++;
        } while (*p != 0);
    }
    return width;
}

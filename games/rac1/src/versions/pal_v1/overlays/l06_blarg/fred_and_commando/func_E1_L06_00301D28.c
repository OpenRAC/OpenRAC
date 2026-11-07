extern int D_E1_L06_0015ED88 MACRO_ADDR;
extern int D_E1_L06_00162038[3] SDATA(D_E1_L06_00162038);
extern int D_E1_L06_001516EC;
extern int D_E1_L06_0016203C MACRO_ADDR;
extern int D_E1_L06_00162040 MACRO_ADDR;
typedef struct {
    char pad[0x38];
    int a[6];
    int b[6];
    int c[3][6];
} CommandoVoices;
void func_E1_L06_00301D28(unsigned char *m, int kind) {
    int sel = -1;
    CommandoVoices *d = *(CommandoVoices **)(m + 0x78);
    int n, i, r;
    char *c;

    switch (kind) {
        case 0: {
            int l = D_E1_L06_0015ED88;
            if (l != 0)
                l = l - 1;
            sel = d->a[l];
            break;
        }
        case 1:
            n = random_integer_below(3);
            c = (char *)d->c;
            i = 0;
            if (D_E1_L06_00162038[n] != 0)
                goto top;
            D_E1_L06_00162038[n] = kind;
            goto done;
            do {
            top:
                r = (n + 1) % 3;
                n = r;
                if (i == 2) {
                    D_E1_L06_00162038[0] = 0;
                    D_E1_L06_0016203C = 0;
                    D_E1_L06_00162040 = 0;
                }
                i++;
                if (i >= 4)
                    goto done;
            } while (D_E1_L06_00162038[n] != 0);
            D_E1_L06_00162038[n] = 1;
        done: {
            int l = D_E1_L06_0015ED88;
            if (l != 0)
                l = l - 1;
            sel = *(int *)(c + (n * 0x18 + l * 4));
            break;
        }
        case 2: {
            int l = D_E1_L06_0015ED88;
            if (l != 0)
                l = l - 1;
            sel = d->b[l];
            break;
        }
    }
    if (sel != -1)
        D_E1_L06_001516EC = sel + 0x4E20;
}

extern unsigned short *ref_L02_003CE9A8_00[];
extern char *ref_L02_003CE9A8_01 __attribute__((sda));

void func_L02_003CE9A8(char *self)
{
    if (*(unsigned char *)(self + 0x21) != 0xFF) {
        unsigned short *list = ref_L02_003CE9A8_00[*(unsigned char *)(self + 0x21)];
        if (list != 0) {
            char *base = ref_L02_003CE9A8_01;
            unsigned short *entry = list;
            int key;
            do {
                char *object = (char *)((unsigned int)base + ((*entry & 0x7FFF) << 8));
                if (object[0x20] >= 0 && *(short *)(object + 0xAA) == 0xA4E)
                    *(short *)(*(char **)(object + 0x68) + 0x316) = 1;
                key = *(short *)entry++;
            } while (key >= 0);
        }
    }
}

extern unsigned short *ref_L04_003E5298_00[];
extern char *ref_L04_003E5298_01 __attribute__((sda));

void func_L04_003E5298(char *self)
{
    if (*(unsigned char *)(self + 0x21) != 0xFF) {
        unsigned short *list = ref_L04_003E5298_00[*(unsigned char *)(self + 0x21)];
        if (list != 0) {
            char *base = ref_L04_003E5298_01;
            unsigned short *entry = list;
            int key;
            do {
                char *object = (char *)((unsigned int)base + ((*entry & 0x7FFF) << 8));
                if (object[0x20] >= 0 && *(short *)(object + 0xAA) == 0x8BA)
                    *(short *)(*(char **)(object + 0x68) + 0x2A6) = 1;
                key = *(short *)entry++;
            } while (key >= 0);
        }
    }
}

extern unsigned short *ref_L17_003E4860_00[];
extern char *ref_L17_003E4860_01 __attribute__((sda));

void func_L17_003E4860(char *self)
{
    if (*(unsigned char *)(self + 0x21) != 0xFF) {
        unsigned short *list = ref_L17_003E4860_00[*(unsigned char *)(self + 0x21)];
        if (list != 0) {
            char *base = ref_L17_003E4860_01;
            unsigned short *entry = list;
            int key;
            do {
                char *object = (char *)((unsigned int)base + ((*entry & 0x7FFF) << 8));
                if (object[0x20] >= 0 && *(short *)(object + 0xAA) == 0xB88)
                    *(short *)(*(char **)(object + 0x68) + 0x406) = 1;
                key = *(short *)entry++;
            } while (key >= 0);
        }
    }
}

extern unsigned short *ref_L26_003DC128_00[];
extern char *ref_L26_003DC128_01 __attribute__((sda));

void func_L26_003DC128(char *self)
{
    if (*(unsigned char *)(self + 0x21) != 0xFF) {
        unsigned short *list = ref_L26_003DC128_00[*(unsigned char *)(self + 0x21)];
        if (list != 0) {
            char *base = ref_L26_003DC128_01;
            unsigned short *entry = list;
            int key;
            do {
                char *object = (char *)((unsigned int)base + ((*entry & 0x7FFF) << 8));
                if (object[0x20] >= 0 && *(short *)(object + 0xAA) == 0xC8E)
                    *(short *)(*(char **)(object + 0x68) + 0x226) = 1;
                key = *(short *)entry++;
            } while (key >= 0);
        }
    }
}

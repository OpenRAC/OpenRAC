extern unsigned char D_0013E633[] NOT_SDA;
extern void func_L00_0028EBF0(int);

/* For each of 8 slots: the handle in +0x2238 is compared with the entry at
   D_0013E633 + 0x1D + k * 0x70 (k from +0x2218; -1 is empty) and, when the entry
   matches and its byte at +0x74 is set, func_L00_0028EBF0(k) runs. The slot is
   then set to -1. A zero handle compares the entry's +0x88 with +0x2080. */
void func_L00_00207220(void) {
    char *s = (char *)D_0013E633 + 0xE1D;
    char *base = (char *)D_0013E633 + 0x1D;
    int *arr1 = (int *)(s + 0x2238);
    int *arr2 = (int *)(s + 0x2218);
    int i, k, t;

    for (i = 0; i < 8; i++) {
        t = arr1[i];
        k = arr2[i];
        if (t != 0) {
            if (k != -1) {
                char *e = base + k * 0x70;
                if (*(int *)(e + 0x88) == t && *(unsigned char *)(e + 0x74) != 0) {
                    func_L00_0028EBF0(k);
                }
            }
        } else {
            if (k != -1) {
                char *e = base + k * 0x70;
                if (*(int *)(e + 0x88) == *(int *)(s + 0x2080) && *(unsigned char *)(e + 0x74) != 0) {
                    func_L00_0028EBF0(k);
                }
            }
        }
        arr2[i] = -1;
    }
}

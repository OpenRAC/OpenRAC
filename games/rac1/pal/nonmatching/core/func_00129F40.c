extern long func_00129CA0(void *s, void *t, void *o1, void *o3, void *o4);
extern void func_0012A7E8(void *arg0, void *arg1);

/* Transcribed from the assembly. Two calls to func_00129CA0 (one for each record,
   with the flag at +0x88 set to 1 for the second), a third that copies fields of
   both records into the object and calls func_00129948, then either a halving of
   p1+0x10 with a tail call to func_00129C78, or an early return. Retail's $2 return
   value is not read by the caller (func_00129600 declares this function void), so
   the early returns just return. */
void func_00129F40(char *s, char *a1, char *a2) {
    char *p1;
    char *p2;
    char *t1;
    char *t2;
    char *base;
    long f22;
    long v2;
    long v3;
    int r;
    int w;

    p1 = a1;
    p2 = a2;
    f22 = 0;
    if (*(int *)(s + 0x174) == 2) {
        t1 = p1;
        t2 = p2;
        f22 = 0x40;
    } else {
        t1 = p2;
        t2 = p1;
    }

    base = *(char **)(s + 0x858);
    func_00129CA0(s, t1, base + 0x10, base + 0x18, base + 0x20);

    base = *(char **)(s + 0x858);
    w = *(int *)(base + 0x10);
    *(long *)(s + 0x88) = 1;
    *(int *)(s + 0x80) = w;
    func_00129CA0(s, t2, base + 0x28, base + 0x30, base + 0x38);

    base = *(char **)(s + 0x858);
    w = *(int *)(base + 0x28);
    *(long *)(s + 0x88) = 1;
    *(int *)(s + 0x80) = w;
    v2 = *(long *)(base + 0x20) | f22;
    v3 = *(long *)(base + 0x38);
    *(int *)(s + 0xCC) = *(int *)(t1 + 0x5C);
    *(long *)(base + 0x20) = v2;
    v3 = v3 | f22;
    *(int *)(s + 0xD0) = *(int *)(t1 + 0x60);
    *(long *)(base + 0x38) = v3;
    *(int *)(s + 0xB4) = *(int *)(t1 + 0x44);
    *(int *)(s + 0xB8) = *(int *)(t2 + 0x48);
    *(int *)(s + 0xC0) = *(int *)(t1 + 0x50);
    *(int *)(s + 0xC4) = *(int *)(t2 + 0x54);
    r = func_00129948(s, p1);
    if (r == 0) {
        return;
    }
    if (*(int *)(p1 + 0x28) != 1) {
        return;
    }
    if (*(int *)(p2 + 0x28) != 1) {
        return;
    }
    *(int *)(p1 + 0x10) = (int)((unsigned int)*(int *)(p1 + 0x10) << 1);
    if (*(int *)(s + 0xB0) != 0) {
        func_0012A7E8(s, p1);
    } else {
        func_001299E8((void *)s, (void *)p1);
    }
    *(int *)(p1 + 0x10) = *(int *)(p1 + 0x10) >> 1;
    func_00129C78(s);
}

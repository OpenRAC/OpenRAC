extern void *func_002AB220(void *);
extern float func_002B0360(void *);
extern int D_001A8CA0 __attribute__((sda));

/* Compares the object's float at 0x18 with the partner's float at 0x90 and, when the
   object is still smaller, with the found object's float at 0x40 less half; the
   float that is compared comes from the helper at 0x10 when the resident flag is
   set and from the object itself otherwise. */
int func_002B63C0(void *a0, void *a1)
{
    void *s0;
    int s2;
    float f;

    if (*(float *)((char *)a0 + 0x18) <= *(float *)((char *)a1 + 0x90)) {
        return 0;
    }
    s0 = func_002AB220(a0);
    if (s0 == 0) {
        return 0;
    }
    s2 = 0;
    if (D_001A8CA0 != 0) {
        f = func_002B0360((char *)a0 + 0x10);
        if (f < *(float *)((char *)s0 + 0x40) - 0.5f) {
            s2 = 1;
        }
    } else {
        f = *(float *)((char *)a0 + 0x18);
        if (f < *(float *)((char *)s0 + 0x40) - 0.5f) {
            s2 = 1;
        }
    }
    return s2;
}

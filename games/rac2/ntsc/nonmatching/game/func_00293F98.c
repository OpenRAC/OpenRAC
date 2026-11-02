/* func_00293F98 (216 bytes, 28 places) -- NEAR MISS, not integrated.
   Current USA v1.01 probe on 29 October 2026: 216 bytes, 0/28 (freshly
   replayed with the retained source as a temporary standalone unit). Reloading the
   record pointer inside the loop restores retail's 0x20-byte frame and keeps
   the same three callee-saved registers plus $ra. The first argument setup
   still differs: retail loads entry+0x3E28 before table+0x3E24 so their sum
   occupies the call delay slot; this candidate loads the table field first.
   Reversing the sum operands makes the compiler issue the entry load before
   the table load, matching retail order, but still assigns them to `$v0` and
   `$a3` rather than retail's `$a3` and `$a1`; the delay-slot sum uses the wrong
   register pair. Splitting those two loads into sequential locals leaves the
   same assignments and remains 216 bytes, 0/28. Table/address materialization
   and loop pointer/register choices also differ.

   The earlier candidate kept the record in a local and compiled to 220 bytes
   with a 0x30-byte frame. The source below is the improved reload form. */

extern int F46b87d1_D_0014B540[];

extern int F46b43b72_D_001B8840[];

extern void F46b43b72_FUN_002B7B18();

extern void F46b43b72_FUN_002B7CE0(int value);

extern void func_0027D330(int);

/* The record the world state points at: the offset of its first part and the
   number of the parts that follow. */
struct PartRec {
    int flags;
    int count;
};

void func_00293F98(int index, int flag)
{
    char *entry = (char *)F46b87d1_D_0014B540 + (index << 3);
    char *state = (char *)F46b43b72_D_001B8840;
    int i;

    F46b43b72_FUN_002B7B18(*(int *)(state + 0x70),
                          *(int *)(entry + 0x3E28) + *(int *)((char *)F46b87d1_D_0014B540 + 0x3E24),
                          *(int *)(entry + 0x3E2C));
    if (flag != 0) {
        F46b43b72_FUN_002B7CE0(0);
        func_0027D330(flag);
        *(int *)(state + 0x30) = index;
        F46b43b72_FUN_002B7CE0(1);
        for (i = 0; i < 0x46; i++) {
            char *rec = *(char **)(state + 0x70) + i * 8;
            if (*(int *)(rec + 4) == 0)
                break;
            *(int *)(state + 0x74 + 4 * i) = *(int *)(state + 0x70) + *(int *)rec + 0x800;
        }
    }
}

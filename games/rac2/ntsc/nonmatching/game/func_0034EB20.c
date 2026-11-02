/* func_0034EB20 - 188 bytes, 28 places (the boot and every level).

   Near miss: the whole body matches byte for byte - the frame, the four calls
   and the loop - except for the order in which the prologue saves the
   callee-saved registers:

       retail:  sd $s3,0x78($sp)  sd $s2,0x70($sp)  sd $s0,0x60($sp)  sd $s1,0x68($sp)
       this C:  sd $s3,0x78($sp)  sd $s0,0x60($sp)  sd $s1,0x68($sp)  sd $s2,0x70($sp)

   The register assignment itself is right ($s3 = the object, $s0 = the upload
   slot pointer, $s1 = the block pointer, $s2 = the counter); only the order in
   which the four saves are emitted differs, i.e. the order in which the
   compiler fixed the four pseudo-regs to the four callee-saved registers.

   Measured: the order does not follow the order of the declarations, of the
   assignments, of the loop form (while / for) or of the for-init list - it
   stayed s3, s0, s1, s2 in every spelling tried.  The retail's order needs
   the counter fixed before the two pointers.

   The object holds the pending-slot count at +0x2E20, the 4-byte upload slots
   at +0x2E00 and the 0x400-byte blocks at +0xE00; the local 0x60-byte buffer
   is the one the retail passes as `sp` to func_00126288 and func_001265B0.
*/

extern void func_002FC3C0(int);
extern void func_00126288(void *, short, short, int, int, int, short, short);
extern void func_001265B0(void *, void *);
extern void func_001244B8(int, int);

typedef struct {
    short value;
    short pad;
} UploadSlot;

void func_0034EB20(void *object)
{
    unsigned char buffer[0x60];
    char *base = (char *)object;
    int i;
    UploadSlot *slots;
    unsigned char *blocks;

    func_002FC3C0(1);
    for (i = 0, blocks = (unsigned char *)(base + 0xE00), slots = (UploadSlot *)(base + 0x2E00);
         i < *(int *)(base + 0x2E20); i++) {
        func_00126288(buffer, slots->value, 1, 0, 0, 0, 0x10, 0x10);
        func_0011AEA0(0);
        slots++;
        func_001265B0(buffer, blocks);
        blocks += 0x400;
        func_001244B8(0, 0);
    }
    *(int *)(base + 0x2E20) = 0;
}

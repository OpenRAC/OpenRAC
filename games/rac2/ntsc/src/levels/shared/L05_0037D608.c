/* Run the object's cache-slot list: for every record, hand the slot (offset from the cache
   pointer, read again at each site) to the engine with a random vector. The cache pointer is
   an sda word so each use is the assembler macro form (lui + lw), never hoisted. */
typedef unsigned char u8;

extern int func_002ED688(unsigned char *);
extern float func_002A7940(void);
extern void func_002AF000(float *, float, float, float);
extern void func_00282CC0(void *, void *, void *);
extern void func_002EE920(void *, float *);

extern short *D_L05_001FB220[];
extern u8 *D_L05_001B251C __attribute__((sda));

void func_L05_0037D608(void *object, void *argument, float value)
{
    unsigned short *q = (unsigned short *)D_L05_001FB220[((unsigned char *)object)[0x21]];
    unsigned short *p;

    if (q != 0) {
        float vector[4];

        p = q;
        do {
            int offset = (*p & 0x7FFF) << 8;

            if (func_002ED688(D_L05_001B251C + offset) != 0) {
                float a = func_002A7940();
                float b = func_002A7940();

                func_002AF000(vector, value, a, b);
                func_00282CC0(vector, vector, argument);
                func_002EE920(D_L05_001B251C + offset, vector);
            }
        } while ((short)*p++ >= 0);
    }
}

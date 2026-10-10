/* NON_MATCHING func_L00_0024DFC4 -- src/overlays/shared/memcard_0024DFB0.c
 * Best so far: SIZE ours 100 / retail 96, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
unsigned int func_L00_0024DFC4(unsigned char *start, unsigned char *end)
{
    unsigned int crc = 0xEDB88320;
    while (start < end) {
        int i;
        crc ^= *start++ << 8;
        for (i = 7; i >= 0; i--) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1F45;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc & 0xFFFF;
}

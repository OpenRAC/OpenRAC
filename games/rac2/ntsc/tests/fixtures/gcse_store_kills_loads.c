/* Authored controls for the gcse memory model (GCC 2.95.3: every store kills every load); no game code. */
typedef struct { unsigned short count; unsigned short pad; unsigned int maximum; unsigned int levels; } Record;
extern Record records[];
extern unsigned int frame;
extern unsigned int level;
extern int first, second;
extern void call(int);

/* Two updates of different records: the second compare reads `frame' after stores to the
   first record. With alias analysis gcse would keep the first load in a register; with every store
   killing every load it is loaded again. */
void two_records(void)
{
    if (first > 50 && records[12].count < 0xFFFF) records[12].count++;
    if (frame > records[12].maximum) records[12].maximum = frame;
    records[12].levels = (records[12].levels | (1u << level)) | 0x80000000u;
    if (second > 2 && records[29].count < 0xFFFF) records[29].count++;
    if (frame > records[29].maximum) records[29].maximum = frame;
}

/* No store between the loads: the first load is reused. */
int no_store(int v)
{
    int a = frame + v;
    if (v) a += level;
    return a + frame;
}

/* A call between the loads kills them. */
int across_call(int v)
{
    int a = frame;
    call(v);
    return a + frame;
}

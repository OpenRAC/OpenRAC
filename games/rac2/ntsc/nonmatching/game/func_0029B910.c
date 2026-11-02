/* Retained complete C reconstruction, requalified on current master
 * 2026-10-28; not linked or counted.
 * Profile: -O2 -G8 -ffunction-sections.
 * Private placement checks: us: 0/28, us2: absent, pal: absent.
 * The observed loop reads four mission IDs; all four words are now within the
 * declared table. Historical screening measured 272/328 bytes.
 * Current in-unit `tools/check.py` result is 280/328 bytes, 0/28 in USA v1.01.
 * Separate output induction pointers and record/global address scheduling
 * still differ. A pointer-induction rewrite on 2026-10-28 compiled to 288/328
 * and remained 0/28; it produced no useful improvement, so the 280-byte
 * indexed-array form remains the retained best.
 */

extern char *D_001A8CFC[] __attribute__((sda));
extern int D_001A9AC8[4] __attribute__((sda));
extern int func_002ED8A0(int);

typedef struct {
    unsigned short id;           /* +0x00 */
    short pad02;
    int next;                    /* +0x04 */
    int pad08[4];
    int state;                   /* +0x18 */
    short count;                 /* +0x1C */
    short pad1E;
    unsigned short level;        /* +0x20 */
    short pad22;
    int flag;                    /* +0x24 */
} MissionRec;

typedef struct {
    int first;                   /* +0x00 */
    int second;                  /* +0x04 */
    int color;                   /* +0x08 */
    float angle;                 /* +0x0C */
    short id;                    /* +0x10 */
    short pad12;
    int text;                    /* +0x14 */
} ListEntry;

extern MissionRec D_00262940[];
extern ListEntry D_00256418[];
extern void func_0033A9D0(char *, int, int);

void func_0029B910(void) {
    int i;
    if (D_001A8CFC[0] != 0) {
        for (i = 0; i < 4; i++) {
            int idx = func_002ED8A0(D_001A9AC8[i]);
            if (idx >= 0) {
                MissionRec *rec = &D_00262940[idx];
                D_00256418[i].second = rec->count;
                D_00256418[i].color = rec->flag >= 1 ? 0x6029A1FF : 0x60F0F0B0;
                D_00256418[i].id = rec->level;
                func_0033A9D0(D_001A8CFC[0] + 0x3F138, i, 0);
            } else {
                func_0033A9D0(D_001A8CFC[0] + 0x3F138, i, 2);
            }
        }
    }
}

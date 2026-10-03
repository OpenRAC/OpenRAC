/* NON_MATCHING func_L00_002902A0 -- src/overlays/shared/space_0028FB78.c
 * Best so far: SIZE ours 628 / retail 624, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_002902A0: for each table entry of the current mode (D_0013E130 record's short at +0x26 picks D_L00_00
 *   The instruction sequence and structure match (do-while over arr[sel], `next = i + 1` copied back, `tbl + i * 1
 *   Also retail forms the address of D_0013E130 as lui + addiu + lh 0x26($2) (not folded into the symbol offset), 
 */
extern int func_001F4868(int);
extern void func_001F9C30(void *, void *, float);
extern int func_002140B0(int);
extern float func_001FA888(int);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9EC0(void *, void *, void *);
extern void func_L00_001FD1D8(void *, void *, int);
extern char D_L00_001BDD40[];
extern char D_L00_001BDDC0[];
extern char D_L00_001BDDE0[];
extern float D_L00_001BDD20[][2];
extern float D_L00_001BDE00[][4];
extern short D_L00_00160640;

typedef struct {
    float v[3];
    float w;
} TEnt;

typedef struct {
    float pos[4][4];
    int col[4];
    float uv[4][2];
    long tag[4];
} Quad;

/* Draws the level's four-corner quad sprites for each table entry of the current mode, around the moby. */
void func_L00_002902A0(char *m) {
    Quad q;
    float v[4];
    float *pv;
    TEnt *tbl;
    int sel;
    char *g;
    int i;
    int j;
    int next;
    int k;
    int color;
    float scale;

    g = D_0013E130_a;
    sel = *(short *)(g + 0x26);
    tbl = (TEnt *)D_L00_001BDD40;
    if (sel == 1) {
        tbl = (TEnt *)D_L00_001BDDC0;
    } else if (sel == 2) {
        tbl = (TEnt *)D_L00_001BDDE0;
    }
    q.tag[1] = func_001F4868(5);
    pv = v;
    q.tag[2] = 0xFF9000000260L;
    q.tag[3] = 0x8000000048L;
    q.tag[0] = 0;
    for (j = 0; j < 4; j++) {
        q.uv[j][0] = D_L00_001BDD20[j][0];
        q.uv[j][1] = D_L00_001BDD20[j][1];
    }
    func_001F9C30(pv, m, 0.0009765625f);
    i = 0;
    if (((int *)&D_L00_00160640)[*(short *)(D_0013E130_a + 0x26)] > 0) {
        do {
            k = *(unsigned char *)(m + 0xBC);
            if (*(short *)(m + 0xB2) != 0) {
                k += func_002140B0(*(short *)(m + 0xB2));
            }
            scale = func_001FA888(k) * (tbl[i].w / 40.0f);
            color = (k << 24) | 0x2058B0;
            if (*(short *)(m + 0xA6) == 0x215) {
                color = (k << 24) | 0x308000;
            }
            next = i + 1;
            for (j = 0; j < 4; j++) {
                q.col[j] = color;
                func_001F9C30(q.pos[j], D_L00_001BDE00[j], scale);
                func_001F9BD8(q.pos[j], q.pos[j], (char *)tbl + i * 16);
                func_001F9EC0(q.pos[j], q.pos[j], D_0013E130 + 0xC0);
                func_001F9BD8(q.pos[j], q.pos[j], pv);
            }
            func_L00_001FD1D8(&q, 0, 0);
            i = next;
        } while (i < ((int *)&D_L00_00160640)[*(short *)(D_0013E130_a + 0x26)]);
    }
}

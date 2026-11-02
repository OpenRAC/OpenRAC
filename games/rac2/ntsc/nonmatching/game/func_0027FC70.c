/* GPT-6.1 Sol (high), task v1064; NOT MATCHING.
   Current pinned compiler: 1320/1312 bytes, 0/28 places, frame 0x140.
   Recheck in a separate unit with these per-file flags:
     -O2 -G8 -ffunction-sections -Wa,-G32 -mno-split-addresses
   This improves the previous 1324-byte retained candidate: declaring the
   measured eight-color palette's bound plus -Wa,-G32 gives retail's GP
   addressing; nosda screen extents and -mno-split-addresses reproduce the
   entry and final absolute-load forms. A short color index prevents the
   unwanted conditional move; using a while loop for line scanning gives
   retail's index/width registers and its bottom-of-line termination shape.
   Still different: balancing/retry blocks rotate above the scan, color-test
   annul/reload selection, loop padding, and the final mode reload (retail
   has both a GP load on the length-exhaustion edge and an absolute reload).
   An explicit retry variable keeps the wrap allocation/block order much
   closer, but grows to 1328 or 1336 bytes. Plain retry labels reach 1312
   bytes with worse allocation and Wsave held in a register instead of the
   retail stack slot. -g and -fno-thread-jumps leave the closest loop shape
   unchanged; -fno-rerun-loop-opt loses required loop padding, and disabling
   strength reduction grows it to 1340 bytes. Do not repeat these unchanged.
   Next useful experiment: recover the original balancing-loop structure
   without an extra materialized retry flag. No volatile casts or barriers.
   Adapted from OpenRAC/rac1-decomp src/game/draw.c FontPrintWindow
   (GPL-3.0), retaining RAC2 conditional scissoring and length accounting.
   Required declarations are below; no shared helper changes are needed.
*/

typedef unsigned char u8;
typedef unsigned long u64;
extern int D_001B1668 __attribute__((nosda));
extern int D_001A89B0[8] __attribute__((sda));
extern int func_0027F128(u8 *, int, u8 *);
extern void func_0027F640(u64, u8 *, int, u64, u8 *, float, float, float);
extern void func_0027F2A8(int, int, u64, u8 *, int, u64, u8 *);
extern int D_001A8BB0 __attribute__((nosda));
extern int D_001B166C __attribute__((sda));
extern int D_001A7340 __attribute__((nosda));
extern int D_001A7344 __attribute__((nosda));
extern void func_002FC620(int, int, int, int);

struct FontWindow {
    short y_min, y_max, x_min, x_max, x_anchor, y_start;
    short out_w, out_h, line_h;
    unsigned short flags;
    short x_sub, y_sub;
};

/* FontPrintWindow */
void func_0027FC70(unsigned short *win_p, u64 a1, u8 *text_p, int len, u64 a4, u8 *glyphs)
{
    struct FontWindow *win = (struct FontWindow *)win_p;
    unsigned char *text = (unsigned char *)text_p;
    short start[32];
    short endi[32];
    short colr_arr[32];
    int W, Wsave, line, i, brk, width, n0, last, balanced;
    int k, y, flags, adv, c;
    short colr;

    if (D_001A8BB0 != 1)
        func_002FC620(win->x_min, win->x_max - 1, win->y_min, win->y_max - 1);
    D_001B166C = 1;
    if ((win->flags ^ 1) & 1) {
        W = win->x_max - win->x_anchor;
    } else {
        int l = win->x_anchor;
        int a = win->x_max - l;
        W = l - win->x_min;
        if (a < W)
            W = a;
        W = W * 2;
    }
    Wsave = W;
    colr = 0;
    balanced = 0;
    n0 = 0;
    last = 0;

    for (;;) {
        line = 0;
        i = 0;
        while (i != len && text[i] != 0) {
            start[line] = i;
            colr_arr[line] = colr;
            width = 0;
            brk = i;
            {
                while (width < W) {
                    if (text[i] == 0x20 || text[i] < 0x10)
                        brk = i;
                    if (D_001B1668 != 0 && (unsigned int)(text[i] - 8) < 8)
                        colr = text[i] - 8;
                    if (text[i] < 2)
                        break;
                    adv = ((signed char *)glyphs)[text[i] * 4 + 3];
                    i++;
                    if (adv != 0)
                        width += adv;
                }
            }
            endi[line] = brk;
            if ((short)brk == start[line])
                endi[line] = i;
            i = endi[line];
            if (text[i] == 0x20 || text[i] < 0x10)
                endi[line] = endi[line] - 1;
            line++;
            if (text[i] == 0) {
                last = width;
                break;
            }
            i++;
        }
        if (balanced || line < 2)
            break;
        if (n0 == 0)
            n0 = line;
        if (n0 < line) {
            W = Wsave;
            balanced = 1;
        } else if (last < W / 3) {
            W -= 16;
        } else {
            break;
        }
    }

    {
        struct FontWindow *dw = win;
        int cnt, w;
        dw->out_w = 0;
        y = dw->y_start;
        dw->out_h = line * dw->line_h;
        if (dw->flags & 2)
            y -= (line * dw->line_h) >> 1;
        for (k = 0; k < line; k++, y += dw->line_h) {
            if (y + dw->line_h < dw->y_min)
                continue;
            if (dw->y_max < y)
                continue;
            cnt = endi[k] - start[k] + 1;
            if (len > 0 && cnt > len) cnt = len;
            w = func_0027F128(text + start[k], cnt, glyphs);
            if (dw->out_w < w)
                dw->out_w = w;
            if (!(dw->flags & 4)) {
            D_001A89B0[0] = a1;
            if (dw->flags & 8) {
                float fx;
                if (dw->flags & 1)
                    fx = (dw->x_anchor - (w >> 1)) + dw->x_sub * 0.0625f;
                else
                    fx = dw->x_anchor + dw->x_sub * 0.0625f;
                func_0027F640((long)D_001A89B0[colr_arr[k]], text + start[k], cnt, a4, glyphs, fx, y + dw->y_sub * 0.0625f, 1.0f);
            } else if (dw->flags & 1) {
                func_0027F2A8(dw->x_anchor - (w >> 1), y, (long)D_001A89B0[colr_arr[k]], text + start[k], cnt, a4, glyphs);
            } else {
                func_0027F2A8(dw->x_anchor, y, (long)D_001A89B0[colr_arr[k]], text + start[k], cnt, a4, glyphs);
            }
            }
            if (len > 0) {
                len -= cnt;
                if (len <= 0) break;
            }
        }
    }
    D_001B166C = 0;
    if (D_001A8BB0 != 1)
        func_002FC620(0, D_001A7340 - 1, 0, D_001A7344 - 1);
}

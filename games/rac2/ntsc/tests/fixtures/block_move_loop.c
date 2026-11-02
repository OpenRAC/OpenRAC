/* Block-move loops for the post-DBR short-loop padding (tests/test_block_move_loop.py). */

typedef struct { unsigned char b[0x400]; } Page;      /* byte aligned: runtime alignment test, two loops */
typedef struct { long long d[0x80]; } AlignedPage;    /* 8-byte aligned: one ld/sd loop */

extern Page page_dst;
extern AlignedPage aligned_dst;

void copy_page(Page *src)
{
    page_dst = *src;
}

void copy_aligned_page(AlignedPage *src)
{
    aligned_dst = *src;
}

/* A genuinely short loop keeps its padding: four instructions with the delay slot. */
void clear_words(int *p, int n)
{
    do {
        *p++ = 0;
    } while (--n);
}

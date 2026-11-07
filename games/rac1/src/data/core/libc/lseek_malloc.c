/* Data owned by src/core/libc/lseek_malloc.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

extern char __malloc_av_[];
extern void *malloc_split_remainder_bin_head[];
extern char D_0012FC70[];

void *D_0012F890[] DATA_SECTION(D_0012F890) = {
    __malloc_av_,
    __malloc_av_,
};

void *D_0012F898[] DATA_SECTION(D_0012F898) = {
    malloc_split_remainder_bin_head,
    malloc_split_remainder_bin_head,
};

void *D_0012FC78[] DATA_SECTION(D_0012FC78) = {
    D_0012FC70,
    D_0012FC70,
};

u32 __malloc_max_sbrked_mem[2] DATA_SECTION(D_0012FCA8) = {0};

u32 __malloc_max_total_mem[2] DATA_SECTION(D_0012FCB0) = {0};

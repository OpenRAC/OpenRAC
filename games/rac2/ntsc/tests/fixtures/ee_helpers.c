#include "common.h"

void copy_quad(void *dst, const void *src) { qcopy(dst, src); }
void copy_quad_nc(void *dst, const void *src) { qcopy_nc(dst, src); }
void clear_quad(void *dst) { qzero(dst); }

int read_after_copy(int *dst, const void *src)
{
    *dst = 7;
    qcopy(dst, src);
    return *dst;
}

int read_after_clear(int *dst)
{
    *dst = 7;
    qzero(dst);
    return *dst;
}

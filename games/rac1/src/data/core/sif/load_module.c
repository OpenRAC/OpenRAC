/* Data owned by src/core/sif/load_module.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

extern u32 D_00152A50[];

int lf_bind_state DATA_SECTION(D_0012FDB0) = {
#include "data_inc/D_0012FDB0.inc"
};

void *lf_expected_version DATA_SECTION(D_0012FDB4) = D_00152A50;

u32 D_00152A50[] DATA_SECTION(D_00152A50) = {
#include "data_inc/D_00152A50.inc"
};

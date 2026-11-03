/* Data owned by src/core/kernel/tty_queue.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

char tty_packet_too_large_msg[] DATA_SECTION(D_00152810) =
#include "data_inc/D_00152810.inc"
    ;

u32 tty_receive_error_msg[] DATA_SECTION(D_00152838) = {
#include "data_inc/D_00152838.inc"
};

u32 tty_send_error_msg[] DATA_SECTION(D_00152850) = {
#include "data_inc/D_00152850.inc"
};

char tty_wlen_error_msg[] DATA_SECTION(D_00152868) =
#include "data_inc/D_00152868.inc"
    ;

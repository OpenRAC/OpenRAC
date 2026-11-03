#ifndef CORE_STORAGE_WAD_H
#define CORE_STORAGE_WAD_H

/* Functions defined in src/core/storage/wad.c. */

#include "common.h"

extern int load_disc_sectors_into_global_buffer(void);
extern int read_file_entry_with_retry(int);
extern int wad_GetSectors_FiiPv(int, int, void *);

#endif /* CORE_STORAGE_WAD_H */

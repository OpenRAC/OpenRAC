#ifndef CORE_MC_MEMORY_CARD_H
#define CORE_MC_MEMORY_CARD_H

/* Functions defined in src/core/mc/memory_card.c. */

#include "common.h"

extern int sceDbcReceiveData(int, int, int *, char *);
extern int scePad2Read(int, void *);
extern int sce_dbc_init(void);
extern int sce_mc_close(int);
extern int sce_mc_delete(int, int, char *);
extern int sce_mc_format(int, int);
extern int sce_mc_get_dir(int, int, char *, int, int, void *);
extern int sce_mc_get_info(int, int, int *, int *, int *);
extern int sce_mc_init(void);
extern int sce_mc_mkdir(int, int, int);
extern int sce_mc_read(int, void *, int);
extern int sce_mc_seek(int, int, int);
extern int sce_mc_sync(int, int *, int *);
extern int sce_mc_unformat(int, int);
extern int sce_mc_write_buffer(int, unsigned char *, int);
extern int sce_pad2_create_socket(void *, char *);
extern int sce_pad2_get_button_profile(int, void *);
extern int sce_pad2_get_state(int);
extern int sce_pad2_init(void);

#endif /* CORE_MC_MEMORY_CARD_H */

#ifndef GAME_RENDER_VUCHAIN_H
#define GAME_RENDER_VUCHAIN_H

/* Functions defined in src/game/render/vuchain.c. */

#include "common.h"

extern void DMAC_VIF1_Disable(void);
extern void DMAC_VIF1_Enable(void);
extern void VU0_loadMicroProgram(void *);
extern void VU1_addDataRef(void *, int);
extern void VU1_addGSregister(int, long);
extern void VU1_gsRegsNormal(void);
extern void VU1_initChain(void);
extern void VU1_sendChain(void);
extern void VU1_setScissor(int, int, int, int);
extern void VU1_swapChain(void);
extern void VU1_syncChain(int);
extern void VU1_texFlush(void);
extern void append_dma_tag(int);
extern void calculate_dma_transfer_address(void);
extern void vu1_add_vif_code(int);
extern void vu1_gs_regs_alt(void);
extern void vu1_gs_regs_font(void);
extern void write_vif_unpack_packet(int, void *, int);

#endif /* GAME_RENDER_VUCHAIN_H */

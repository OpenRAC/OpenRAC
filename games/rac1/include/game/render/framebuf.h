#ifndef GAME_RENDER_FRAMEBUF_H
#define GAME_RENDER_FRAMEBUF_H

/* Functions defined in src/game/render/framebuf.c. */

#include "common.h"

extern void AA_BlurPass(void);
extern void PutDispBuffer(void);
extern void PutDrawBufferLarge(void);
extern void PutDrawBufferSmall(void);
extern void SetBackgroundColor(int, int, int);
extern void SetupFS_AA_buffer(int, int, int, int, int, int);
extern void append_gif_transfer_packet(void);
extern void append_palette_transfer_packet(void);
extern void draw_centered_capture_background(int, int);
extern void draw_framebuffer_rect(int, int, int, int, int, int, int);
extern void setup_render_target(int, int, int);

#endif /* GAME_RENDER_FRAMEBUF_H */

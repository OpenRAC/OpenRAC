#ifndef GAME_RENDER_DRAW_H
#define GAME_RENDER_DRAW_H

/* Functions defined in src/game/render/draw.c. */

#include "common.h"

extern int AddDrawCallback(void *, void *);
extern void func_001F2608(void);
extern void DoGifPaging(void);
extern void DrawDebugProfiler(void);
extern void DrawTexturedQuad(int, int, int, int, int, int, int, int, long, long);
extern void DrawUIFrame(int, int, int, int, int);
extern void FadeToBlack(int);
extern int FontPrintCenter(int, int, long, void *, int);
extern int FontPrintCenterLarge(int a, int b, int c, int d, int e);
extern int FontPrintCenterSmall(int, int, long, void *, int);
extern void FontPrintLarge(int, int, long, void *, int);
extern void FontPrintSmall(int, int, long, void *, int);
extern void FontSetWindow(void *, int, int, int, int, int, int, int, int);
extern void func_001F65A8(void);
extern void func_001F6598(void);
extern void InitViewContext(void);
extern void func_001F2560(void *, int);
extern void func_001F2558(void *, int);
extern void ResetDrawGlobals(void);
extern void ResetGsRegisters(void);
extern void ResetGsRegistersPr(void);
extern void SetPalMode(void);
extern void SetupGifPaging(int);
extern void UpdateFog(void);
extern void UpdateViewContext(void);
extern void append_rotated_sprite_quad(float, float, float, float, float, int, int, int, int, int, int, int, float,
                                       float);
extern void append_subpixel_textured_screen_quad(float, float, float, float, int, int, int, int, unsigned long, long);
extern void begin_draw_frame(void);
extern void func_001F0F30(void);
extern void configure_graphics_projection(int, int, float, float, float, float, float);
extern void configure_graphics_render_target_and_projection(int, int, int, float);
extern void dispatch_callback_list_1(void);
extern void draw_outlined_rect(int, int, int, int, int);
extern void emit_rgba_draw_packet(int, int, int, int);
extern void enqueue_callback_list_4(void *, void *);
extern void font_print_right(int, int, int, void *, int);
extern void font_print_right_large(int, int, unsigned long, void *, int);
extern void font_print_right_small(int, int, long, void *, int);
extern void font_print_window_regular(void *, long, void *, int);
extern void font_print_window_small(void *, long, void *, int);
extern void font_queue_vu_state(void);
extern void initialize_alpha_lookup_table(void);
extern int measure_text_width(void *, int, void *);
extern int measure_text_width_regular(void *, int);
extern int measure_text_width_small(void *, int);
extern void prepare_debug_profiler_render(void);
extern void print_debug_text(int, int, int, void *);
extern int print_debug_text_centered(int, int, int, void *);
extern void project_to_screen(float *, void *);
extern void queue_framebuffer_upload(int);
extern void reset_graphics(void);

#endif /* GAME_RENDER_DRAW_H */

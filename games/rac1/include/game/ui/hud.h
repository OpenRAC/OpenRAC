#ifndef GAME_UI_HUD_H
#define GAME_UI_HUD_H

/* Functions defined in src/game/ui/hud.c. */

#include "common.h"

extern s32 GetIconFrame(s32, s32);
extern void func_00200468(s32, s32, s32, s32, s32, s32);
extern void *Hud_HeapAlloc(int, int, void *, int);
extern void Hud_HeapReset(void);
extern void Hud_SendResidentBank(int, int, int);
extern void func_001FFDA0(int, int);
extern void Hud_sendTexture(int, int, int, int, int, int);
extern void LinkHudBank(int, int);
extern void append_power_of_two_textured_screen_sprite(long, int, int, int, int, int, int, int, int, int);
extern void apply_pending_animation(void *);
extern void copy_text_to_shared_buffer(int);
extern void draw_boot_image(int);
extern void draw_framed_text(int, int, int, int);
extern void draw_hud_rect(int, int, int, int, unsigned long, int);
extern void func_00201640(int, int, int, int, long, long);
extern void draw_hud_rect_depth(int, int, int, int, long, int, int);
extern void draw_hud_sprite_flipped(int, int, int, int, int, int);
extern void draw_hud_sprite_rect(int, int, int, int, int, int, int, int, int, int);
extern void draw_hud_sprite_subpixel(s32, s32, s32, s32, s32, s32);
extern void draw_hud_sprite_uv(int, int, int, int, int, int, int, int);
extern void draw_rotated_sprite(float, float, float, float, float, int, int, long);
extern void draw_stretchable_ui_frame(s32, s32, s32, s32, s32);
extern void init_hud(void);
extern int queue_animation_update(int, int, int, int, int, int, int);
extern s32 remove_hud_item(s32);
extern void update_hud(void);
extern void func_001FFFA0(void);

#endif /* GAME_UI_HUD_H */

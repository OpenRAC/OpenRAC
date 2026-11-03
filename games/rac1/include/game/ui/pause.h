#ifndef GAME_UI_PAUSE_H
#define GAME_UI_PAUSE_H

/* Functions defined in src/game/ui/pause.c. */

#include "common.h"

extern void PauseAllSounds(int arg0);
extern int acquire_menu_stream_buffer(int);
extern void clear_scene_state_buffers(void);
extern void *create_menu_preview_moby(int);
extern int delete_moby(int);
extern void initialize_transfer_command(void);
extern void load_level_zero_preserving_progress(int);
extern void render_level_effects_and_screen_sprites(void);
extern void reset_active_camera_position_and_basis(void);
extern void submit_graphics_setup_command_stream(u8 *);
extern void tick_space_transition(void);
extern void update_menu_system(void);

#endif /* GAME_UI_PAUSE_H */

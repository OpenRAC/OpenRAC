#ifndef OVERLAYS_SHARED_NPC_DIALOG_AND_PAD_HISTORY_H
#define OVERLAYS_SHARED_NPC_DIALOG_AND_PAD_HISTORY_H

/* Functions defined in src/overlays/shared/npc_dialog_and_pad_history.c. */

#include "common.h"

extern int DialogRequest_Init(void *, void *);
extern int Npc_TryStartDialog(void *, void *);
extern int check_pad_press_separation(int, int, int, int);
extern int get_dialogue_entry(int, int);
extern int pad_pressed_within(int, int, void *);
extern float read_pad_float_history(int);
extern void set_moby_dialogue_record_value(void *, int);

#endif /* OVERLAYS_SHARED_NPC_DIALOG_AND_PAD_HISTORY_H */

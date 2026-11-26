/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/* Aranos Prison's help prompts and its wrench upgrade (level 14, 0x003F4F98): the decompilation's
 * C (src/levels/14_aranos_prison/L14_003F4F98.c) with USA v2.00's test of the upgrade
 * (src/us2/levels/14_aranos_prison/L14_003F64DC.c; PAL's L14_003F9660.c is the same). USA v1.01
 * counts the wrench upgraded from level 2 on, which challenge mode's wrench (3 from the start, 4
 * after Tabora) always is: the upgrade hides itself at once and never gives level 5, the
 * OmniWrench 12000. v2.00 and PAL test against the level the upgrade gives (2, or 5 in challenge
 * mode), and so does the port ("prison-wrench", port/game/rac2/differences.json).
 *
 * hostgen puts this after the matched file's C (prep.use_overrides), so its types, declarations
 * and helpers are the file's own. */

/* True once the gadget in slot 10 has been upgraded to LIMIT. */
static inline int prisonWrenchUpgradedTo(int limit)
{
    return (g_item_l14_materials + g_item_slot_to_record[10])->level >= limit;
}

/* Drives the prison's help prompts: links the target moby, plays the hints at the trigger zones
   and records which help messages were already shown. */
void func_L14_003F4F98(char *obj)
{
    PrisonHelpState *st = *(PrisonHelpState **)(obj + 0x68);
    float vel[4] __attribute__((aligned(16)));
    int limit = 2;

    if (flag_is_byte_set())
        limit = 5;

    switch (*(unsigned char *)(obj + 0x20)) {
    case 0:
        *(short *)(obj + 0x96) = 0;
        *(unsigned char *)(obj + 0x30) = 0xFF;
        if (st->target >= 0)
            st->moby = D_L14_001B259C_direct + (st->target << 8);
        else
            st->moby = 0;
        if (!prisonWrenchUpgradedTo(limit)) {
            if (st->moby)
                st->active = 1;
            else
                st->active = 0;
        } else {
            if (st->moby) {
                *(unsigned short *)(st->moby + 0x34) |= 1;
                st->moby[0x31] = 0;
            }
            st->active = 0;
        }
        moby_set_state(obj, 1, -1);
        break;
    case 1:
        if (flag_set_first_pending(obj))
            st->count = 0;
        if (IsPointInCuboid(g_hero_position, st->zone[0])) {
            if (!prisonWrenchUpgradedTo(limit))
                moby_set_state(obj, 2, -1);
        }
        if (st->active) {
            qcopy(vel, g_identity_l14_matrix);
            vel[1] -= 0.1f;
            moby_rotate_about_point(st->moby, vel, 0.05f, 0.0f, 0.0f);
        }
        break;
    case 2:
        if (flag_set_first_pending(obj))
            st->count = 0;
        if (g_shop_mode != 2 && st->count >= 6) {
            if (flag_is_byte_set())
                menu_seek_chain_entry(10, 5);
            else
                menu_seek_chain_entry(10, 2);
            gfx_run_screen_update_steps(2);
            if (st->moby) {
                *(unsigned short *)(st->moby + 0x34) |= 1;
                st->moby[0x31] = 0;
            }
            st->active = 0;
            moby_set_state(obj, 1, -1);
        }
        break;
    }

    if (IsPointInCuboid(g_hero_position, st->zone[1]) && !(g_help_prompt_flags & 2))
        prisonHelpTimed(0xA2C, 131);
    if (IsPointInCuboid(g_hero_position, st->zone[2])) {
        prisonHelp(0xA09, 96);
        g_subtitle_history[97].count = 0xFFFF;
    }
    if (IsPointInCuboid(g_hero_position, st->zone[3]) && g_help_l14_prompt_flags[2])
        prisonHelp(0xA0A, 97);
    if (IsPointInCuboid(g_hero_position, st->hintZone) && !(g_moby_group_flags[st->row + g_current_map_index * 16] & 0x80) &&
        g_subtitle_history[94].count < 2)
        prisonHelp(0xA07, 94);
    st->count++;
}

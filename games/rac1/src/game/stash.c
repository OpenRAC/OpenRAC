#include "common.h"
#include "callees.h"
#include "records.h"
#include "structs.h"

/*
 * stash.cpp in the original source; text 0x233FF8-0x234380.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/DECOMP_PROGRESS.md. Compiled as C for now.
 */

/* Declarations in scope here before the split. */
extern char voice_pool[];
extern int bank_sound_count;
extern void FastMemCopy(void *, void *, int);
extern char backup_cam[];
extern char gCamBackupState[];
extern void *cur_cam NOT_SDA;
extern DispatchRec camera_types[];
extern int draw_config[];
extern void FastMemSet();
extern void BuildOcclVisibility(void);
extern int occl_update NOT_SDA;
extern char occl_visibility[];
extern long gs_display_settings[3];
extern int space_load_in_progress;
extern short draw_enable_mask;
extern void append_gif_transfer_packet(void);
extern void DrawDebugProfiler(void);
extern int draw_callback_count;
extern int draw_callback_args[];
extern int draw_callback_funcs[];
extern short GlobalStateFlag;
extern int measure_text_width(unsigned char *arg0, int arg1, void *arg2);
extern unsigned char normal_font_metrics[];
extern unsigned char small_font_metrics[];
extern unsigned char large_font_metrics[];
extern void FontPrint(void *, void *, void *, void *, void *, int,
                          unsigned char *);
extern int measure_text_width_regular(unsigned char *, int);
extern int measure_text_width_small(unsigned char *, int);
extern int GetEffectTex(int);
extern void PutDrawBufferLarge(void);
extern void InitViewContext(void);
extern void UpdateViewContext(void);
extern int bit_swap_lut[];
extern long background_rgbaq NOT_SDA;
extern int Help_FindIndex(void);
extern char s_paradox_this_message_does_not[];
extern short help_msgs;
extern int hud_heap_base NOT_SDA;
extern int hud_state NOT_SDA;
extern int Hud_GetIconIndex(int);
typedef struct {
    char b[0x13];
} Cfg13;
extern Cfg13 message_text_buffer NOT_SDA;
extern Cfg13 message_too_long_label NOT_SDA;
extern int libc_strlen(void);
extern void libc_strcpy(Cfg13 *, void *);
extern short ResourceCounter;
extern void draw_stretchable_ui_frame(int, int, int, int, int);
extern void CreatePointLight(int);
extern void DetachPointLight(int);
extern void update_world_object_animation(void *);
extern int render_sequence NOT_SDA;
extern int vu1_chain_buffer_2 NOT_SDA;
extern int current_vu_chain;
extern int map_slot_use[];
extern void move_map_entry_slot(int a, int b);
extern PadSlots level_map_selection_slots __asm__("D_001A01F0");
extern int level_map_selection[];
extern int *menu_level_indices;
extern unsigned char menu_item_enabled_0 NOT_SDA;
extern unsigned char g_menu_item_enabled_1 NOT_SDA;
extern unsigned char menu_item_enabled_9 NOT_SDA;
extern short menu_selection_count;
extern unsigned char menu_item_enabled_16 NOT_SDA;
extern unsigned char menu_item_enabled_17 NOT_SDA;
extern unsigned char menu_item_enabled_18 NOT_SDA;
extern unsigned char menu_item_enabled_19 NOT_SDA;
extern unsigned char menu_item_enabled_25 NOT_SDA;
extern int map_region_enabled_3 NOT_SDA;
extern unsigned char menu_item_enabled_41 NOT_SDA;
extern int hero_control_mode NOT_SDA;
extern unsigned char menu_item_enabled_36 NOT_SDA;
extern unsigned char menu_item_enabled_37 NOT_SDA;
extern unsigned char menu_item_enabled_38 NOT_SDA;
extern unsigned char menu_item_enabled_55 NOT_SDA;
extern unsigned char menu_item_enabled_56 NOT_SDA;
extern unsigned char menu_item_enabled_57 NOT_SDA;
extern unsigned char menu_item_enabled_68;
extern unsigned char menu_item_enabled_64 NOT_SDA;
extern unsigned char menu_item_enabled_65 NOT_SDA;
extern unsigned char menu_item_enabled_66 NOT_SDA;
extern unsigned char menu_item_enabled_67 NOT_SDA;
extern unsigned char menu_item_enabled_69 NOT_SDA;
extern unsigned char menu_item_enabled_77 NOT_SDA;
extern unsigned char menu_item_enabled_102 NOT_SDA;
extern unsigned char menu_item_enabled_103 NOT_SDA;
extern unsigned char menu_item_enabled_104 NOT_SDA;
extern unsigned char menu_item_enabled_105 NOT_SDA;
extern unsigned char menu_item_enabled_115 NOT_SDA;
extern int max_map_coverage_size[];
extern void DebugEnter_Nop(void);
extern int compress_map_coverage(void *dst, int size, int a, int b);
extern void downsample_map_coverage(void *dst);
extern short g_current_level_index;
extern int g_current_level_index_far __asm__("D_0015EE84") NOT_SDA;
extern int map_coverage_active[] NOT_SDA;
extern void decode_compressed_occlusion_map(void *, unsigned char *, int);
extern void expand_map_coverage(void *, unsigned char *);
extern char memory_card_state[];
extern short mode_freeze_state;
extern int mode_freeze_flags;
extern int mc_gamedata[];
extern int mc_leveldata[];
extern int memcard_GetDataSize(int *p);
extern int memcard_PrepData(void *dst, int i, int *table);
extern int sce_mc_init(void);
extern int STUB_printf();
extern char memcard_error_message[];
extern int race_gates_24_status NOT_SDA;
extern unsigned char menu_item_enabled_12 NOT_SDA;
extern int race_gates_47_flag NOT_SDA;
extern unsigned char gSpecialItems[];
extern unsigned char gHaveHeliPack NOT_SDA;
extern int race_gates NOT_SDA;
extern int race_gates_66_status NOT_SDA;
extern unsigned char menu_item_enabled_73 NOT_SDA;
extern int race_gates_70_status NOT_SDA;
extern unsigned char menu_item_enabled_85 NOT_SDA;
extern int race_gates_87_status NOT_SDA;
extern unsigned char race_gates_base NOT_SDA;
extern int item_available NOT_SDA;
extern unsigned char menu_item_enabled_20 NOT_SDA;
extern unsigned char gemlik_available NOT_SDA;
extern unsigned char gHaveMorphORay NOT_SDA;
extern unsigned char gHaveHologuise NOT_SDA;
extern int moby_render_slots[];
extern void copy_matrix_rotation_2(void *, void *) __asm__("func_001FA460");
extern void FastVecScale(void *, void *, float);
extern void FastVecAdd(void *, void *, void *);
extern void matrix_mul_vec3(void *, void *, void *);
extern int g_hero_moby NOT_SDA;
extern float shadow_light_dir[] NOT_SDA;
extern float FastCos(float);
extern float FastSin(float);
extern void flush_cache(int);
extern char normal_lut[];
extern int render_packet_end;
extern short vu1_chain_safe_end;
extern char moby_backup_buffer[];
extern int func_001160D8(void);
extern float random_angle_radians(void);
extern float random_float_between(float, float);
extern void BuildSphericalOffset(void *, float, float, float);
extern void fast_vec_normalize(void *, void *, float);
extern void copy_matrix_rotation(void *);
extern void MatrixToQuaternion(void *, void *);
extern void matrix_copy_rows(void *, void *);
extern float MobyGetFrameValue(void);
extern float round_float_to_decimal_places(int, float);
extern unsigned char gold_bolts_collected[];
extern unsigned char gold_weapon_purchased[];
extern unsigned char gSkillPoints[];
extern void snd_StreamSafeCdCallback(void *);
extern void finish_audio_stream_read(int);
extern short cd_read_state[];
extern void snd_ContinueVAGStream(void *);
extern void snd_StreamSafeCdBreak(void);
extern char load_file_start_failed_string[];
extern int snd_StreamSafeCdRead(int, int, int, void *);
extern int start_audio_stream_read_v(void) __asm__("func_00217628");
extern void sce_gs_sync_v(int);
extern void music_Update(void);
extern void snd_reset_state_and_flush_commands(void);
extern void snd_FlushSoundCommands(void);
extern void snd_reenable_command_flush(void);
extern int snd_StreamSafeCdGetError(void);
extern void music_start_track_by_id(short, short, short);
extern void music_PreseekTrack(short, short, short);
extern short music_flagged_track_started NOT_SDA;
extern char g_controller_state[];
extern int pad_socket_param NOT_SDA;
extern void sce_dbc_init(void);
extern void sce_pad2_init(int);
extern int sce_pad2_create_socket(void *, void *);
extern void poll_pad_device_state(void *);
extern int pause_action_list_mode;
extern char pause_action_list_pal[];
extern char pause_action_list_short[];
extern char menu_system[] NOT_SDA;
extern char animation_asset_read_active[];
extern int selected_level_index;
extern int selected_mission_id;
extern int selected_mission_index;
extern void update_mission_list(void *);
extern int collect_mission_ids(void *, void *, void *, int);
extern int pause_iop_sprite_lists[];
extern char heli_pack_menu_table[];
extern char no_helipack_menu_table[];
extern int pause_memory_card_state NOT_SDA;
extern float FastAddRots(float, float);
extern char *current_menu_page NOT_SDA;
extern void DrawMobyList(int, int);
extern char gCamera[];
extern void rotate_preview_moby_x(void *);
extern void *create_menu_preview_moby_a(int) __asm__("func_00226720");
extern int delete_moby(int);
extern void VU1_addGSregister(int, int);
extern void draw_map_overlay(void);
extern void SetupGifPaging(int);
extern void DoGifPaging(void);
extern void func_001F68E8_c(int, int, long, void *, int)
    __asm__("func_001F68E8");
extern void *msg_string_id(int) __asm__("func_001FE540");
extern short panel_clear_color;
extern void draw_hud_rect_blended(int, int, int, int, long, long);
extern int GetIconFrame(int, int);
extern void HudSprite(int, int, int, int, int, int);
extern void DrawTexturedQuad(int, int, int, int, int, int, int, int, long,
                          long);
extern short fs_aa_buffer[];
extern long transition_overlay_tex0;
extern int acquire_menu_stream_buffer(int);
extern int release_menu_stream_buffer(int);
extern int pad0_pressed_unmasked NOT_SDA;
extern char pause_default_action_list[];
extern char *pause_current_action_list NOT_SDA;
extern unsigned char resident_class_slot_by_id[] NOT_SDA;
extern void *CreateMoby(void);
extern void pack_render_command_fields(void *, int, int, int, int);
typedef struct {
    int key;
    int flags;
} PadBind;
extern PadBind menu_stream_buffers_t[] __asm__("D_001D6448");
extern int get_menu_stream_buffer_size(int handle);
extern int menu_stream_buffers[];
extern char preview_resource_bindings[] NOT_SDA;
extern char *moby_class_resources[] NOT_SDA;
extern int scene_state_buffer_0[];
extern int scene_state_buffer_1[];
extern int scene_state_buffer_2[];
extern char camera_position_a[] __asm__("D_00187180");
extern char coll_hit_pos[];
extern int sound_probe_ignore;
extern void random_spherical_offset(void *, float, float);
extern void FastVecAdd_a(void *, void *, void *) __asm__("func_001F9BD8");
extern void FastVecScale_a(void *, void *, float) __asm__("func_001F9C30");
extern void FastVecSub_a(void *, void *, void *) __asm__("func_001F9BF0");
extern int coll_line_fix_a(void *, void *, int, int, int) __asm__("func_001EFE10");
extern char camera_position[];
extern void FastVecSub(void *, void *, void *);
extern void coll_line_fix(void *, void *, int, int, int);
extern float FastVecDist(int, void *);
extern void calculate_voice_distance_volume(void *, float, float, float);
extern void transform_vector(void *, void *, void *);
extern float vec_length_xy(void *);
extern float fast_arc_tan_a(float, float) __asm__("func_001FA058");
extern void truncate_float_to_s32(float);
extern void sce_cd_sync_s(int);
extern int snd_BankLoadByLoc(void *, int);
extern void snd_StreamSafeCheckCDIdle(int);
extern void snd_StreamSafeCdSync(int);
extern void snd_ResolveBankXREFS(void);

extern void sceSifInitRpc(int arg0);

void initialize_sif_rpc(void) {
    sceSifInitRpc(0);
}

extern int sceSifBindRpc(void *client, int id, int mode);
extern int sce_sif_check_stat_rpc(void *client);
extern int sif_call_rpc(void *, int, int, void *, int, void *, int, void *, void *); /* sceSifCallRpc */

/*
 * Authentic structs from IOPSTASH.IRX STABS debug symbols:
 *   StashInfo: base, size, cd[10], free (cur), block (count)
 *   StashBlock: ram (unk_00), qwc (unk_04), comment (unk_08), pad (unk_0C)
 */
typedef StashInfo StashState_00234018;
extern StashState_00234018 stash_state_alias __asm__("D_001DD530");
typedef StashBlock Rec10_00234018;
extern Rec10_00234018 stash_entries_alias[] __asm__("D_001DD568");

/* init_stash: binds the IOP stash RPC server (0x11, no-wait mode; a bind
   error hangs), waits for the bind and retries after a delay loop until
   the server answers, then asks it (RPC 2, sceSifCallRpc's nine
   arguments) for the buffer's base and size and clears the 0x40 slots.
   The server check reads through a pointer set inside the loop: loop.c
   then hoists the struct's full address into a register of its own, as
   retail has it, while the client's address stays a constant. */
void init_stash(void) {
    StashState_00234018 *s;
    int reply[4];
    int i;

    for (;;) {
        if (sceSifBindRpc(stash_state_alias.cd, 0x11, 1) < 0) {
            for (;;) {
            }
        }
        while (sce_sif_check_stat_rpc(stash_state_alias.cd) != 0) {
        }
        s = &stash_state_alias;
        if (*(int *)((char *)s->cd + 0x24) != 0) {
            break;
        }
        i = 0xFFFF;
        while (i--) {
        }
    }
    sif_call_rpc(stash_state_alias.cd, 2, 0, 0, 0, reply, 0x10, 0, 0);
    stash_state_alias.base = reply[0];
    stash_state_alias.size = reply[1];
    stash_state_alias.free = reply[0];
    stash_state_alias.block = 0;
    for (i = 0; i < 0x40; i++) {
        stash_entries_alias[i].ram = 0;
        stash_entries_alias[i].qwc = 0;
    }
}
__asm__(".section .text\n\tnop\n");

extern StashBlock stash_entries[];

/* The stash: a buffer on the IOP side (base, size, from func_00234018's
   RPC), the SIF RPC client at D_001DD538, and a bump allocator over the
   buffer that hands out up to 0x40 slots in D_001DD568. */
extern StashInfo stash_state;
extern int sce_sif_set_dma(void *, int);

/* Stash_SendData: DMA arg1 quadwords from arg0 to the stash's next free
   address (func_00118E20 takes a {src, dst, size, mode} record), reserve
   arg2 quadwords there, and return the slot. `h->block++` read into the
   slot index is what keeps retail's copy of the old count. */
int Stash_SendData(int arg0, int arg1, int arg2, int arg3) {
    StashInfo *h = &stash_state;
    int dma[4];
    int n;

    if (h->size - (h->free - h->base) < arg2 * 16) {
        return -1;
    }
    if (h->block == 0x40) {
        return -2;
    }
    dma[0] = arg0;
    dma[1] = h->free;
    dma[2] = arg1 * 16;
    dma[3] = 0;
    sce_sif_set_dma(dma, IOP_STASH_FETCH);
    n = h->block++;
    stash_entries[n].ram = h->free;
    stash_entries[n].qwc = arg2;
    stash_entries[n].comment = arg3;
    h->free += arg2 * 16;
    return n;
}

extern int stash_get_reserved_qwords(unsigned int);
extern int sif_call_rpc(void *, int, int, void *, int, void *, int, void *, void *); /* sceSifCallRpc */
extern char stash_rpc_client_address[];

/* stash_receive_data(dest, slot, offset, size, mode): size -1 means the
   slot's whole length (func_00234350). -3 for a bad or empty slot, -1
   if offset + size runs past the slot. Otherwise the data is fetched
   from the stash (slot base + offset quadwords) to dest by the stash
   client's RPC 1 (IOP_STASH_FETCH), at most 0xFFFF quadwords per call; returns 0. The RPC
   is sceSifCallRpc, with nine arguments: end function and end
   parameter 0, the last on the stack. */
int stash_receive_data(void *dest, unsigned int slot, int offset, int size, int mode) {
    int src;
    int chunk;

    if (size == -1) {
        size = stash_get_reserved_qwords(slot);
    }
    if (slot >= 0x40) {
        return -3;
    }
    if (stash_entries[slot].qwc == 0) {
        return -3;
    }
    if (stash_entries[slot].qwc < offset + size) {
        return -1;
    }
    src = stash_entries[slot].ram + offset * 16;
    while (size != 0) {
        chunk = (size > 0xFFFF) ? 0xFFFF : size;
        sif_call_rpc(stash_rpc_client_address, IOP_STASH_FETCH, mode, &src, 0x10, dest, chunk * 16, 0, 0);
        size -= chunk;
        dest = (char *)dest + chunk * 16;
        src += chunk * 16;
    }
    return 0;
}

int stash_get_reserved_qwords(unsigned int arg0) {
    if (arg0 >= 0x40) {
        return -3;
    }
    return stash_entries[arg0].qwc;
}

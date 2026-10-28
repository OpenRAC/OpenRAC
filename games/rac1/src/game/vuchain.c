#include "common.h"
#include "callees.h"
#include "records.h"
#include "structs.h"

/*
 * vuchain.cpp in the original source; text 0x2347F0-0x2352C8.
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
extern void VU1_addGSregister(int, long);
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
typedef struct {
    int unk_00;
    int unk_04;
    int unk_08;
    int unk_0C;
} Rec10;
extern Rec10 stash_entries[];
typedef struct {
    short a;
    short b;
} TexRemap;
typedef struct {
    char *items;
    int count;
} TexChunk;
extern TexChunk tfrag_gif_chunk_list[];
extern TexRemap tfrag_tex_remap_table[];


#define VIF0_STAT ((volatile unsigned int *)0x10008000)
#define VIF0_FBRST ((volatile unsigned int *)0x10008020)
#define VIF0_BASE ((volatile unsigned int *)0x10008030)

/* VU0_loadMicroProgram(long *): waits for VIF0 to go idle, resets it,
 * points it at the (uncached-masked) micro program address, kicks off
 * MSCALL 0x45 by writing the code word directly, then waits for VIF0 to
 * go idle again. */
void VU0_loadMicroProgram(long *prog) {
    while (*VIF0_STAT & 0x100) {
        spin_wait(0x10);
    }
    *VIF0_FBRST = 0;
    *VIF0_BASE = (unsigned int)prog & 0x0FFFFFFF;
    *VIF0_STAT = 0x145;
    while (*VIF0_STAT & 0x100) {
        spin_wait(0x10);
    }
}

extern int g_current_level_index_m __asm__("D_0015EE84") MACRO_ADDR;
extern int current_vu_chain_m __asm__("D_0016100C") MACRO_ADDR;
extern int vu_chain_table[];

/* Advances the index at D_0016100C, wrapping at 0x13. The old note had
   the condition backwards: `slti; movz x,$0,t` is `if (idx >= 0x13)
   idx = 0`. Both globals are MACRO_ADDR, which gives the one-register
   load and the $gp store in the jr slot. */
void calculate_dma_transfer_address(void) {
    int idx = g_current_level_index_m;
    if (idx >= 0x13) {
        idx = 0;
    }
    current_vu_chain_m = vu_chain_table[idx];
}

/*
 * The VU1 chain state. D_00161000 (the write pointer), D_00161010 (the
 * buffer index) and the two limits D_0015F718/D_0015F71C are all reached
 * with retail's one-instruction macro form, so they are MACRO_ADDR: the
 * assembler expands them through $at, or through $gp where the access
 * lands in a delay slot.
 */
extern int *g_render_packet_cursor MACRO_ADDR;
extern int previous_render_packet_cursor MACRO_ADDR;
extern int current_vu_chain;
extern int vu1_chain_buffer_index MACRO_ADDR;
extern int vu1_chain_reserved_bytes;
extern int render_packet_end MACRO_ADDR;
extern int vu1_chain_safe_end MACRO_ADDR;
extern int vu1_chain_buffers[2];
extern int level_resource_buffers[];

extern int vu1_chain_reserved_bytes_m __asm__("D_0015F698") MACRO_ADDR;

/* `end` reads D_00161000 back right after storing it (CSE folds the
   load away), which changes how the stores are scheduled: that, not
   any order of the four stores, gives retail's. */
void VU1_initChain(void) {
    int base = level_resource_buffers[1];
    int end;

    vu1_chain_buffers[0] = base;
    vu1_chain_buffers[1] = level_resource_buffers[2];
    vu1_chain_buffer_index = 0;
    g_render_packet_cursor = (int *)base;
    end = (int)g_render_packet_cursor + current_vu_chain_m - vu1_chain_reserved_bytes_m;
    render_packet_end = end;
    vu1_chain_safe_end = end - 0x2000;
}

/* VU1_swapChain(void) */
void VU1_swapChain(void) {
    int idx = 1 - vu1_chain_buffer_index;
    int base = vu1_chain_buffers[idx];
    int end;

    previous_render_packet_cursor = (int)g_render_packet_cursor;
    vu1_chain_buffer_index = idx;
    g_render_packet_cursor = (int *)base;
    end = (int)g_render_packet_cursor + current_vu_chain_m - vu1_chain_reserved_bytes_m;
    render_packet_end = end;
    vu1_chain_safe_end = end - 0x2000;
}

extern int vu1_chain_peak_bytes MACRO_ADDR;
extern short vif_chain_flags;
extern char vu1_chain_overflow_msg[];
extern int *sce_dma_get_chan(int);
extern void sce_dma_send(int *, int);

/* VU1_sendChain. D_00160FE0 is volatile, so its store stays out of the
   delay slot (reorg never moves a volatile access there). */
void VU1_sendChain(void) {
    int size;
    int err;
    int *chan;

    *(volatile int *)&vif_chain_flags |= 0x1F;
    size = (int)g_render_packet_cursor - vu1_chain_buffers[vu1_chain_buffer_index];
    err = 0;
    if (vu1_chain_peak_bytes < size) {
        vu1_chain_peak_bytes = size;
        if (current_vu_chain_m < size) {
            STUB_printf(vu1_chain_overflow_msg);
            err = 1;
        }
    }
    if (err == 0) {
        g_render_packet_cursor[0] = 0x70000000;
        g_render_packet_cursor[1] = 0;
        g_render_packet_cursor[2] = 0;
        g_render_packet_cursor[3] = 0;
        chan = sce_dma_get_chan(1);
        *chan |= 0xC0;
        flush_cache(0);
        sce_dma_send(chan, vu1_chain_buffers[vu1_chain_buffer_index]);
    } else {
        *(volatile int *)&vif_chain_flags = 0;
    }
}

extern short vif_chain_flags;              /* SDA, gp -0x5D20 */
extern char vu1_sync_timeout_msg[];
extern void reset_graphics(void);

/* VU1_syncChain(int) */
void VU1_syncChain(int mask) {
    int i = 0;
    while ((*(int *)&vif_chain_flags & mask) != 0) {
        spin_wait(0x400);
        if (i > 100000) {
            STUB_printf(vu1_sync_timeout_msg);
            reset_graphics();
            break;
        }
        i++;
    }
}

/*
 * VU1_addDataRef(void *, int). Retail reloads D_00161000 before every
 * field write -- the writes could alias the pointer itself -- which is
 * exactly what writing through the global (rather than through a local
 * copy) produces.
 */
void VU1_addDataRef(void *data, int qwc) {
    g_render_packet_cursor[0] = qwc | 0x30000000;
    g_render_packet_cursor[1] = (int)data;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0;
    g_render_packet_cursor += 4;
}

LINKER_REMNANT("asm/remnants/text", func_00234B98);

/* Append a DMA packet that unpacks qwc quadwords from src to VU1 memory address arg0. */
void write_vif_unpack_packet(int arg0, void *src, int qwc) {
    g_render_packet_cursor[0] = qwc | 0x10000000;
    g_render_packet_cursor[1] = 0;
    g_render_packet_cursor[2] = 0x1000404;
    g_render_packet_cursor[3] = arg0 | (qwc << 16) | 0x6C000000;
    g_render_packet_cursor += 4;
    FastMemCopy(g_render_packet_cursor, src, qwc * 16);
    g_render_packet_cursor += qwc * 4;
}

LINKER_REMNANT("asm/remnants/text", func_00234C48);

/* Append an empty DMA cnt tag carrying the VIF code arg0. */
void vu1_add_vif_code(int arg0) {
    g_render_packet_cursor[0] = 0x10000000;
    g_render_packet_cursor[1] = 0;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = arg0;
    g_render_packet_cursor += 4;
}

/*
 * VU1_addGSregister(unsigned int, unsigned long). Real name per the
 * Lombyte's project NTSC project (github.com/mateuszklysz/Lombyte's project); their NTSC
 * build's version, VU1_addGSregister__FUiUlb, takes a third `bool` arg
 * and is 4 bytes larger (0xb4 vs this PAL build's 176 bytes) -- PAL
 * evidently dropped the bool, confirmed by every real call site in this
 * codebase using only 2 args.
 */
void VU1_addGSregister(int arg0, long arg1) {
    g_render_packet_cursor[0] = 0x10000002;
    g_render_packet_cursor[1] = 0;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000002;
    g_render_packet_cursor[4] = 0x8001;
    g_render_packet_cursor[5] = 0x10000000;
    g_render_packet_cursor[6] = 0xE;
    g_render_packet_cursor[7] = 0;
    *(long *)((char *)g_render_packet_cursor + 0x20) = arg1;
    g_render_packet_cursor[10] = arg0;
    g_render_packet_cursor[11] = 0;
    g_render_packet_cursor += 0xC;
}

LINKER_REMNANT("asm/remnants/text", func_00234D50);

extern int g_screen_extent[];

/* VU1_setScissor(x0, x1, y0, y1): clamps the rectangle to the screen
   (D_0013E600[0]/[1] are its width and height) and queues it as GS
   register 0x40 (SCISSOR_1: x0 | x1 << 16 | y0 << 32 | y1 << 48) in the
   same packet VU1_addGSregister (func_00234C98) builds. Each clamp is a
   `cond ? bound : arg` select assigned back to the argument, and the
   fields are sign-extended to 64 bits, not zero-extended. */
/* VU1_setScissor(x0, x1, y0, y1): clamps the rectangle to the screen
   (D_0013E600[0]/[1] are its width and height) and queues it as GS
   register 0x40 (SCISSOR_1: x0 | x1 << 16 | y0 << 32 | y1 << 48) in the
   same packet VU1_addGSregister (func_00234C98) builds. */
void VU1_setScissor(int x0, int x1, int y0, int y1) {
    x0 = x0 < 0 ? 0 : x0;
    x1 = x1 > g_screen_extent[0] - 1 ? g_screen_extent[0] - 1 : x1;
    y0 = y0 < 0 ? 0 : y0;
    y1 = y1 > g_screen_extent[1] - 1 ? g_screen_extent[1] - 1 : y1;
    g_render_packet_cursor[0] = 0x10000002;
    g_render_packet_cursor[1] = 0;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000002;
    g_render_packet_cursor[4] = 0x8001;
    g_render_packet_cursor[5] = 0x10000000;
    g_render_packet_cursor[6] = 0xE;
    g_render_packet_cursor[7] = 0;
    *(long *)(g_render_packet_cursor + 8) =
        (long)x0 | ((long)x1 << 16) | ((long)y0 << 32) | ((long)y1 << 48);
    g_render_packet_cursor[10] = 0x40;
    g_render_packet_cursor[11] = 0;
    g_render_packet_cursor += 0xC;
}

__asm__(".section .text\n\tnop\n");

LINKER_REMNANT("asm/remnants/text", func_00234E78);

extern char vu1_gs_regs_tex_flush[];

/* VU1_texFlush(void) */
void VU1_texFlush(void) {
    g_render_packet_cursor[0] = 0x30000003;
    g_render_packet_cursor[1] = (int)vu1_gs_regs_tex_flush;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000003;
    g_render_packet_cursor += 4;
}

extern char gs_regs_normal_packet[];

/* VU1_gsRegsNormal(void) */
void VU1_gsRegsNormal(void) {
    g_render_packet_cursor[0] = 0x30000003;
    g_render_packet_cursor[1] = (int)gs_regs_normal_packet;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000003;
    g_render_packet_cursor += 4;
}

extern char gs_regs_alt_packet[];

/* Append a DMA ref tag that sends the alt GS register packet (3 qwords) via DIRECT. */
void vu1_gs_regs_alt(void) {
    g_render_packet_cursor[0] = 0x30000003;
    g_render_packet_cursor[1] = (int)gs_regs_alt_packet;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000003;
    g_render_packet_cursor += 4;
}

LINKER_REMNANT("asm/remnants/text", func_00234FA0);

extern char draw_environment[];

/* Append a DMA ref tag that sends the draw environment (11 qwords) via DIRECT. */
void vu1_gs_regs_font(void) {
    g_render_packet_cursor[0] = 0x3000000B;
    g_render_packet_cursor[1] = (int)draw_environment;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x5000000B;
    g_render_packet_cursor += 4;
}

LINKER_REMNANT("asm/remnants/text", func_00235008);

extern int vif1_dma_handler_id MACRO_ADDR;
extern int vu1_halt_handler_id MACRO_ADDR;

extern int add_dmac_handler(int, void *, void *);
extern void enable_dmac_atomic(int);
extern void vif1_dma_interrupt_handler(void);
extern void print_register_values_and_halt(void);

/*
 * DMAC_VIF1_Enable(void): unless the handlers are installed, set CIM1 in
 * D_STAT and install func_00235118 (channel 1) and func_00235218 (0xF),
 * then enable channel 1. The D_STAT read is volatile but the store is
 * plain: a volatile store stops reorg's try_merge_delay_insns at it and
 * leaves a second copy of the handler's lui on the fall-through path.
 */
void DMAC_VIF1_Enable(void) {
    if (vif1_dma_handler_id == 0 && vu1_halt_handler_id == 0) {
        if ((*(volatile int *)0x1000E010 & 0x20000) == 0) {
            *(int *)0x1000E010 = 0x20000;
        }
        vif1_dma_handler_id = add_dmac_handler(1, (void *)vif1_dma_interrupt_handler, (void *)0);
        vu1_halt_handler_id = add_dmac_handler(0xF, (void *)print_register_values_and_halt, (void *)0);
        enable_dmac_atomic(1);
    }
}

/*
 * DMAC_VIF1_Disable(void). The two handler ids are MACRO_ADDR: the copy
 * scheduled into the branch delay slot is $gp-relative, the copy on the
 * taken path is the lui/%lo expansion of the same macro. The volatile
 * D_STAT write is what forces the reload on that path.
 */
extern int remove_dmac_handler(int, int);
extern int disable_dmac_atomic(int);

/* Stop VIF1 DMA, remove the VIF1 DMA and VU1 halt interrupt handlers. */
void DMAC_VIF1_Disable(void) {
    if ((*(volatile int *)0x1000E010 & 0x20000) != 0) {
        *(volatile int *)0x1000E010 = 0x20000;
    }
    remove_dmac_handler(1, vif1_dma_handler_id);
    remove_dmac_handler(0xF, vu1_halt_handler_id);
    disable_dmac_atomic(1);
    vif1_dma_handler_id = 0;
    vu1_halt_handler_id = 0;
}

ASM_FUNC("asm/handwritten/text", func_00235118);

extern char vu1_halt_msg_header[];
extern char vu1_halt_register_fmt[];
extern char vu1_halt_msg_footer[];

/* Print the VIF1 DMA register values and hang (VU1 timeout handler). */
void print_register_values_and_halt(void) {
    int a = *(volatile int *)0x10009000;
    int b = *(volatile int *)0x10009030;

    STUB_printf(vu1_halt_msg_header);
    STUB_printf(vu1_halt_register_fmt, a, b);
    STUB_printf(vu1_halt_msg_footer);
    while (1) {
        ;
    }
}

/*
 * Append one quadword to the packet at D_00161000. This one wants the
 * pointer typed as a quadword: stepping an `int *` by 4 folds the copy
 * retail keeps (`daddu $5,$2,$0`) into the increment and comes out one
 * instruction short, so the same symbol is declared a second time under
 * a quadword type.
 */
typedef struct {
    int w[4];
} Qword;
extern Qword *g_render_packet_cursor_q __asm__("D_00161000") MACRO_ADDR;

/* Append an interrupting DMA tag with qword count arg0 to the second packet list. */
void append_dma_tag(int arg0) {
    Qword *p = g_render_packet_cursor_q++;
    p->w[0] = arg0 + 0x90000000;
    p->w[1] = 0;
    p->w[2] = 0;
    p->w[3] = 0;
}

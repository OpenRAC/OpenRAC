/* Carried by port/tools/versionmap/carry_functions.py from the decompilation of Ratchet & Clank (PAL), src/overlays/shared/hero_animation_slots.c (games/rac1): func_L00_0020E3B8, matched C there.
 * Kind `same` in the version map (../../README.md): the two versions have the same instructions for it.
 * Names and addresses are this version's. Functional C, not matched C. */
/* carry, not from a `same` function or an `exact` data address (read them):
 *   D_L00_0015F780 -> 0015F6A0: between
 */

typedef signed char s8_c00_20dd20;
typedef short s16_c00_20dd20;
typedef int s32_c00_20dd20;
typedef unsigned char u8_c00_20dd20;
typedef unsigned short u16_c00_20dd20;
typedef unsigned int u32_c00_20dd20;
typedef unsigned long u64_c00_20dd20;
typedef float f32_c00_20dd20;
typedef struct {
    f32_c00_20dd20 x;
    f32_c00_20dd20 y;
    f32_c00_20dd20 z;
    f32_c00_20dd20 w;
} Vec4f_c00_20dd20;
typedef u32_c00_20dd20 VecQuad_c00_20dd20 __attribute__((mode(TI), aligned(16)));
typedef union {
    VecQuad_c00_20dd20 q;
    f32_c00_20dd20 f[4];
    s32_c00_20dd20 i[4];
} Vec4_c00_20dd20;
struct Moby_c00_20dd20;
struct MobyTrail_c00_20dd20 {
    Vec4_c00_20dd20 pos[8];
    Vec4_c00_20dd20 rot[8];
    s32_c00_20dd20 copy_fade[4];
    s32_c00_20dd20 delay[4];
    struct Moby_c00_20dd20 *copies[4];
    s16_c00_20dd20 head;
    s16_c00_20dd20 count;
    struct Moby_c00_20dd20 *source;
    s32_c00_20dd20 copy_count;
    s32_c00_20dd20 active;
};
struct HeroEase_c00_20dd20 {
    u8_c00_20dd20 joint;
    u8_c00_20dd20 attached;
    u8_c00_20dd20 pad_2[0xE];
    Vec4_c00_20dd20 quat;
    Vec4_c00_20dd20 scale3;
    Vec4_c00_20dd20 translation;
    Vec4_c00_20dd20 rot;
    Vec4_c00_20dd20 rot_vel;
    Vec4_c00_20dd20 rot_target;
    Vec4_c00_20dd20 pos;
    Vec4_c00_20dd20 pos_vel;
    Vec4_c00_20dd20 pos_target;
    s16_c00_20dd20 attach_joint;
    s16_c00_20dd20 kind;
    f32_c00_20dd20 accel;
    f32_c00_20dd20 max_speed;
    f32_c00_20dd20 scale;
};
struct HeroManipulator_c00_20dd20 {
    u8_c00_20dd20 anim_joint;
    u8_c00_20dd20 attached;
    u8_c00_20dd20 unk2;
    u8_c00_20dd20 absolute;
    u8_c00_20dd20 pad_4[0x8];
    f32_c00_20dd20 blend;
    Vec4_c00_20dd20 quat;
    Vec4_c00_20dd20 scale3;
    Vec4_c00_20dd20 translation;
};
struct HeroItemSlot_c00_20dd20 {
    struct Moby_c00_20dd20 *moby;
    struct Moby_c00_20dd20 *moby2;
    u8_c00_20dd20 pad_08[0x4];
    u8_c00_20dd20 *unk0C;
    s32_c00_20dd20 button_mask;
    s32_c00_20dd20 respawn_frame;
    s16_c00_20dd20 timer;
    u8_c00_20dd20 thrown;
    u8_c00_20dd20 released_frames;
    u8_c00_20dd20 switch_lock;
    u8_c00_20dd20 timer_reload;
    u8_c00_20dd20 pad_1E[0x2];
    s32_c00_20dd20 ready_frames;
    s32_c00_20dd20 state;
    s32_c00_20dd20 item_id;
    f32_c00_20dd20 glow_phase;
    f32_c00_20dd20 unk30;
    f32_c00_20dd20 unk34;
    s32_c00_20dd20 unk38;
    s16_c00_20dd20 unk3C;
    u16_c00_20dd20 unk3E;
    s32_c00_20dd20 locked_on;
    u8_c00_20dd20 pad_44[0x4];
    struct Moby_c00_20dd20 *lock_target;
    f32_c00_20dd20 lock_yaw;
};
struct HeroVelocityKey_c00_20dd20 {
    f32_c00_20dd20 value;
    f32_c00_20dd20 delta;
    s32_c00_20dd20 frames;
};
struct HeroMotion_c00_20dd20 {
    Vec4_c00_20dd20 pos;
    Vec4_c00_20dd20 rot;
    Vec4_c00_20dd20 rot_spring_vel;
    Vec4_c00_20dd20 gravity_point;
    Vec4_c00_20dd20 near_ground_point;
    Vec4_c00_20dd20 body_center;
    Vec4_c00_20dd20 velocity;
    Vec4_c00_20dd20 platform_delta;
    Vec4_c00_20dd20 displacement;
    Vec4_c00_20dd20 resolved_velocity;
    Vec4_c00_20dd20 resolved_velocity_z;
    Vec4_c00_20dd20 resolved_velocity_xy;
    Vec4_c00_20dd20 platform_carry;
    Vec4_c00_20dd20 momentum;
    f32_c00_20dd20 speed_xy;
    f32_c00_20dd20 flat_speed;
    f32_c00_20dd20 forward_speed;
    f32_c00_20dd20 move_slope;
    Vec4_c00_20dd20 move_dir;
    f32_c00_20dd20 move_yaw;
    f32_c00_20dd20 yaw_rate;
    f32_c00_20dd20 yaw_error;
    u8_c00_20dd20 pad_18C[0x4];
};
struct HeroRandTimer_c00_20dd20 {
    s32_c00_20dd20 fired;
    s32_c00_20dd20 deadline;
    s32_c00_20dd20 range;
};
struct HeroState_c00_20dd20 {
    s32_c00_20dd20 current;
    s32_c00_20dd20 step;
    s32_c00_20dd20 control_mode;
    s32_c00_20dd20 prev;
    s32_c00_20dd20 prev_control_mode;
    s32_c00_20dd20 prev_timer;
    s32_c00_20dd20 prev2;
    s32_c00_20dd20 prev2_control_mode;
};
struct HeroHealth_c00_20dd20 {
    s32_c00_20dd20 hp;
    s32_c00_20dd20 vehicle_mode_hp;
    s16_c00_20dd20 unk22B0;
    s16_c00_20dd20 unk22B2;
};
struct Hero_c00_20dd20 {
    f32_c00_20dd20 rot_matrix[16];
    f32_c00_20dd20 inv_rot_matrix[16];
    struct HeroMotion_c00_20dd20 motion;
    f32_c00_20dd20 target_speed;
    f32_c00_20dd20 ground_speed;
    s32_c00_20dd20 state_timer;
    s32_c00_20dd20 control_mode_timer;
    s32_c00_20dd20 step_frames;
    s32_c00_20dd20 anim_seq_frames;
    s32_c00_20dd20 stick_held_frames;
    s32_c00_20dd20 stick_idle_frames;
    s16_c00_20dd20 ledge_probe_cooldown;
    s16_c00_20dd20 unk1B2;
    s32_c00_20dd20 wall_jump_window;
    s32_c00_20dd20 long_jump_lockout;
    s32_c00_20dd20 item_anim_frames;
    s32_c00_20dd20 invulnerable_frames;
    s32_c00_20dd20 move_lock_timer;
    s16_c00_20dd20 unk1C8;
    s16_c00_20dd20 rail_catch_grace;
    s32_c00_20dd20 collision_off_frames;
    s32_c00_20dd20 unk1D0;
    s32_c00_20dd20 jump_buffer_lockout;
    s16_c00_20dd20 jump_lockout_timer;
    s16_c00_20dd20 unk1DA;
    s16_c00_20dd20 unk1DC;
    s16_c00_20dd20 unk1DE;
    s16_c00_20dd20 unk1E0;
    s16_c00_20dd20 still_frames;
    s16_c00_20dd20 rail_grab_cooldown;
    s16_c00_20dd20 grapple_lockout_timer;
    s16_c00_20dd20 fidget_cooldown;
    s16_c00_20dd20 unk1EA;
    s16_c00_20dd20 unk1EC;
    s16_c00_20dd20 unk1EE;
    s16_c00_20dd20 unk1F0;
    s16_c00_20dd20 jump_lock_timer;
    s16_c00_20dd20 unk1F4;
    s16_c00_20dd20 unk1F6;
    s16_c00_20dd20 unk1F8;
    u8_c00_20dd20 pad_1FA[0x6];
    Vec4_c00_20dd20 wall_contact_normal;
    Vec4_c00_20dd20 wall_contact_point;
    f32_c00_20dd20 capsule_top;
    f32_c00_20dd20 capsule_bottom;
    f32_c00_20dd20 capsule_top_target;
    f32_c00_20dd20 capsule_bottom_target;
    f32_c00_20dd20 capsule_radius_target;
    f32_c00_20dd20 capsule_radius;
    f32_c00_20dd20 unk238;
    u8_c00_20dd20 *coll_hit_moby;
    s32_c00_20dd20 unk240;
    union { f32_c00_20dd20 f; s32_c00_20dd20 i; } unk244;
    f32_c00_20dd20 wall_probe_distance;
    u8_c00_20dd20 pad_24C[0x4];
    f32_c00_20dd20 wall_normal_pitch;
    u8_c00_20dd20 wall_probe_moby_in_class_range;
    u8_c00_20dd20 wall_probe_hit_moby;
    u8_c00_20dd20 pad_256[0x1];
    u8_c00_20dd20 wall_hit;
    f32_c00_20dd20 edge_drop_depth;
    f32_c00_20dd20 edge_probe_offset;
    s32_c00_20dd20 edge_ahead;
    u8_c00_20dd20 pad_264[0xC];
    Vec4_c00_20dd20 ground_normal;
    Vec4_c00_20dd20 water_surface_normal;
    Vec4_c00_20dd20 gravity_dir;
    Vec4_c00_20dd20 ground_point;
    Vec4_c00_20dd20 last_ground_point;
    union { f32_c00_20dd20 f[2]; s32_c00_20dd20 i[2]; } unk2C0;
    union { f32_c00_20dd20 f[2]; s32_c00_20dd20 i[2]; } unk2C8;
    union { f32_c00_20dd20 f[2]; s32_c00_20dd20 i[2]; } unk2D0;
    union { f32_c00_20dd20 f; s32_c00_20dd20 i; } ground_z;
    f32_c00_20dd20 ground_distance;
    union { f32_c00_20dd20 f; s32_c00_20dd20 i; } ground_slope;
    union { f32_c00_20dd20 f; s32_c00_20dd20 i; } ground_pitch;
    union { f32_c00_20dd20 f; s32_c00_20dd20 i; } unk2E8;
    f32_c00_20dd20 downhill_yaw;
    f32_c00_20dd20 water_surface_z;
    f32_c00_20dd20 sink_surface_z;
    s32_c00_20dd20 in_water_frames;
    struct Moby_c00_20dd20 *ground_moby;
    s32_c00_20dd20 grounded_frames;
    f32_c00_20dd20 ground_spring_velocity;
    s16_c00_20dd20 unk308;
    s16_c00_20dd20 on_flagged_ground_moby;
    union { s16_c00_20dd20 s; u16_c00_20dd20 u; } frames_off_ground;
    union { s16_c00_20dd20 s; u16_c00_20dd20 u; } air_frames;
    Vec4_c00_20dd20 platform_local_pos;
    Vec4_c00_20dd20 attach_anchor_rot;
    Vec4_c00_20dd20 attach_anchor_alt_pos;
    u8_c00_20dd20 pad_340[0x10];
    Vec4_c00_20dd20 attached_moby_move;
    void *platform_moby;
    s32_c00_20dd20 attach_flags;
    u8_c00_20dd20 pad_368[0x48];
    f32_c00_20dd20 unk3B0;
    union { f32_c00_20dd20 f; s32_c00_20dd20 i; } unk3B4;
    s16_c00_20dd20 unk3B8;
    u8_c00_20dd20 pad_3BA[0x2];
    s16_c00_20dd20 run_start_blending;
    s16_c00_20dd20 unk3BE;
    u8_c00_20dd20 pad_3C0[0x10];
    s32_c00_20dd20 unk3D0;
    s32_c00_20dd20 velocity_keys_end_frame;
    f32_c00_20dd20 keyed_move_target_frame;
    f32_c00_20dd20 unk3DC;
    s32_c00_20dd20 keyed_anim_target_frame;
    struct HeroVelocityKey_c00_20dd20 *unk3E4;
    s32_c00_20dd20 unk3E8;
    s32_c00_20dd20 unk3EC;
    f32_c00_20dd20 unk3F0;
    f32_c00_20dd20 long_jump_speed;
    f32_c00_20dd20 fall_gravity;
    u8_c00_20dd20 pad_3FC[0x4];
    Vec4_c00_20dd20 takeoff_pos;
    f32_c00_20dd20 smoothed_ground_z;
    f32_c00_20dd20 air_turn_rate;
    s32_c00_20dd20 rail_land_frames;
    s16_c00_20dd20 velocity_keys_active;
    s16_c00_20dd20 past_jump_apex;
    s32_c00_20dd20 takeoff_frame;
    f32_c00_20dd20 air_stick_speed;
    f32_c00_20dd20 unk428;
    f32_c00_20dd20 unk42C;
    f32_c00_20dd20 unk430;
    f32_c00_20dd20 unk434;
    f32_c00_20dd20 high_jump_drift_yaw;
    f32_c00_20dd20 takeoff_yaw;
    f32_c00_20dd20 apex_anim_frame;
    f32_c00_20dd20 target_anim_frame;
    f32_c00_20dd20 jump_land_anim_frame;
    s32_c00_20dd20 frames_to_land;
    s32_c00_20dd20 unk450;
    f32_c00_20dd20 air_target_speed;
    f32_c00_20dd20 high_jump_drift_speed;
    f32_c00_20dd20 ledge_jump_speed;
    Vec4_c00_20dd20 unk460;
    Vec4_c00_20dd20 unk470;
    f32_c00_20dd20 air_speed_step_up;
    f32_c00_20dd20 jump_decel;
    f32_c00_20dd20 jump_height_start;
    f32_c00_20dd20 unk48C;
    f32_c00_20dd20 fall_start_distance;
    f32_c00_20dd20 jump_anim_retime_scale;
    s16_c00_20dd20 unk498;
    s16_c00_20dd20 jump_max_frames;
    s16_c00_20dd20 unk49C;
    s16_c00_20dd20 unk49E;
    f32_c00_20dd20 jump_gravity;
    s16_c00_20dd20 jump_blocked_ahead;
    s16_c00_20dd20 unk4A6;
    s16_c00_20dd20 unk4A8;
    s16_c00_20dd20 fall_delay_frames;
    s16_c00_20dd20 unk4AC;
    u8_c00_20dd20 can_chain_high_jump;
    u8_c00_20dd20 can_double_jump;
    f32_c00_20dd20 rise_fall_gravity;
    u8_c00_20dd20 pad_4B4[0x4];
    f32_c00_20dd20 past_apex;
    s32_c00_20dd20 predicted_land_frames;
    f32_c00_20dd20 rise_fall_target_frame;
    s32_c00_20dd20 rise_fall_land_frames;
    u8_c00_20dd20 pad_4C8[0x8];
    Vec4_c00_20dd20 ledge_hang_pos;
    f32_c00_20dd20 ledge_top_z;
    f32_c00_20dd20 ledge_normal_yaw;
    s32_c00_20dd20 ledge_found;
    f32_c00_20dd20 unk4EC;
    f32_c00_20dd20 unk4F0;
    u32_c00_20dd20 ledge_flags;
    s32_c00_20dd20 ledge_moby;
    u8_c00_20dd20 pad_4FC[0x4];
    Vec4_c00_20dd20 rail_point;
    Vec4_c00_20dd20 joint9_world_pos;
    Vec4_c00_20dd20 joint14_world_pos;
    Vec4_c00_20dd20 root_joint_pos;
    Vec4_c00_20dd20 prev_rail_point;
    Vec4_c00_20dd20 jump_target_pos;
    s32_c00_20dd20 rail_spline;
    s32_c00_20dd20 rail_segment;
    f32_c00_20dd20 rail_seg_frac;
    s32_c00_20dd20 grind_rail;
    s32_c00_20dd20 rail_off_frames;
    f32_c00_20dd20 unk574;
    s32_c00_20dd20 grind_direction;
    f32_c00_20dd20 rail_dir_yaw;
    f32_c00_20dd20 rail_pitch;
    s32_c00_20dd20 rail_spline_loops;
    f32_c00_20dd20 grind_yaw_offset;
    s32_c00_20dd20 grind_anim_variant;
    s32_c00_20dd20 rail_switching;
    s32_c00_20dd20 transfer_from_spline;
    s32_c00_20dd20 prev_rail_segment;
    f32_c00_20dd20 prev_rail_seg_frac;
    s32_c00_20dd20 unk5A0;
    s32_c00_20dd20 rail_transfer_frames;
    s32_c00_20dd20 grind_pose;
    union { f32_c00_20dd20 f; s32_c00_20dd20 i; } unk5AC;
    f32_c00_20dd20 grind_base_speed;
    s32_c00_20dd20 unk5B4;
    s32_c00_20dd20 unk5B8;
    s16_c00_20dd20 unk5BC;
    s16_c00_20dd20 unk5BE;
    s32_c00_20dd20 rail_jump_queued;
    s32_c00_20dd20 rail_gap_jump;
    f32_c00_20dd20 rail_jump_gravity;
    f32_c00_20dd20 unk5CC;
    f32_c00_20dd20 rail_lean;
    f32_c00_20dd20 grind_lean_vel;
    struct Moby_c00_20dd20 *grind_look_moby;
    f32_c00_20dd20 rail_snap_dist;
    Vec4_c00_20dd20 grab_rail_point;
    s32_c00_20dd20 *follow_path_spline;
    s32_c00_20dd20 follow_path_segment;
    f32_c00_20dd20 rail_candidate_frac;
    s32_c00_20dd20 grab_rail_found;
    s32_c00_20dd20 follow_path_end_frames;
    f32_c00_20dd20 follow_path_speed;
    u8_c00_20dd20 pad_608[0x4];
    s32_c00_20dd20 rail_candidate_kind;
    f32_c00_20dd20 grab_rail_yaw;
    f32_c00_20dd20 follow_path_pitch;
    union { f32_c00_20dd20 f; s32_c00_20dd20 i; } unk618;
    u8_c00_20dd20 pad_61C[0x24];
    Vec4_c00_20dd20 basis[3];
    f32_c00_20dd20 facing_x;
    f32_c00_20dd20 facing_y;
    u8_c00_20dd20 pad_678[0xC];
    f32_c00_20dd20 basis_pitch;
    f32_c00_20dd20 held_pose_yaw;
    u8_c00_20dd20 pad_68C[0x4];
    f32_c00_20dd20 unk690;
    f32_c00_20dd20 unk694;
    s32_c00_20dd20 unk698;
    f32_c00_20dd20 unk69C;
    s32_c00_20dd20 unk6A0;
    struct Moby_c00_20dd20 *sink_effect_moby;
    f32_c00_20dd20 unk6A8;
    f32_c00_20dd20 unk6AC;
    f32_c00_20dd20 unk6B0;
    f32_c00_20dd20 unk6B4;
    u8_c00_20dd20 pad_6B8[0x8];
    Vec4_c00_20dd20 smoothed_spin_stick;
    f32_c00_20dd20 pivot_rot_x;
    f32_c00_20dd20 trick_rot_y;
    u8_c00_20dd20 pad_6D8[0x8];
    s32_c00_20dd20 tricks_landed[6];
    s32_c00_20dd20 unk6F8;
    u8_c00_20dd20 pad_6FC[0x4];
    s32_c00_20dd20 unk700[3];
    s32_c00_20dd20 unk70C;
    u8_c00_20dd20 pad_710[0x10];
    Vec4_c00_20dd20 smoothed_steer_stick;
    u8_c00_20dd20 pad_730[0x10];
    Vec4_c00_20dd20 unk740;
    Vec4_c00_20dd20 landing_displacement;
    Vec4_c00_20dd20 board_ground_pos;
    Vec4_c00_20dd20 unk770;
    Vec4_c00_20dd20 tilt_up_dir;
    Vec4_c00_20dd20 ground_move_dir;
    s16_c00_20dd20 racer_laps[8];
    s16_c00_20dd20 racer_path_point[8];
    s16_c00_20dd20 race_rank[8];
    f32_c00_20dd20 ground_distance_history[0x20];
    u8_c00_20dd20 pad_850[0x4];
    f32_c00_20dd20 board_vertical_speed;
    f32_c00_20dd20 unk858;
    f32_c00_20dd20 hoverboard_speed;
    f32_c00_20dd20 board_gravity;
    s32_c00_20dd20 boost_frames;
    f32_c00_20dd20 boost_speed;
    u8_c00_20dd20 *hoverboard_moby;
    s32_c00_20dd20 unk870;
    f32_c00_20dd20 board_air_yaw;
    f32_c00_20dd20 air_yaw_spring_vel;
    f32_c00_20dd20 unk87C;
    s32_c00_20dd20 trick_hud_handle;
    s16_c00_20dd20 board_boost_active;
    s16_c00_20dd20 landing_turn_delay;
    s16_c00_20dd20 board_jump_land_frames;
    s16_c00_20dd20 prev_air_frames;
    u8_c00_20dd20 steep_jump;
    u8_c00_20dd20 trick_input;
    u8_c00_20dd20 vert_ramp_launch;
    u8_c00_20dd20 air_pose_index;
    u8_c00_20dd20 *unk890;
    s32_c00_20dd20 race_time_frames;
    s16_c00_20dd20 race_path_node;
    s16_c00_20dd20 race_lap;
    s16_c00_20dd20 board_jump_frames;
    s16_c00_20dd20 board_jump_cooldown;
    f32_c00_20dd20 board_floor_z;
    f32_c00_20dd20 unk8A4;
    s32_c00_20dd20 race_score;
    s16_c00_20dd20 high_jump_window;
    u8_c00_20dd20 off_course_frames;
    u8_c00_20dd20 off_course_reset_pending;
    s16_c00_20dd20 height_ring_count;
    s16_c00_20dd20 ground_distance_history_index;
    void *race_path;
    union { f32_c00_20dd20 f; s32_c00_20dd20 i; } unk8B8;
    s16_c00_20dd20 hoverboard_airborne;
    s16_c00_20dd20 board_respawn_delay;
    s32_c00_20dd20 race_position;
    s32_c00_20dd20 boost_pad_timer;
    s16_c00_20dd20 wrong_way_path_point;
    s16_c00_20dd20 help_hint_frames;
    s16_c00_20dd20 boost_min_frames;
    u8_c00_20dd20 race_ammo;
    u8_c00_20dd20 shot_cooldown;
    Vec4_c00_20dd20 unk8D0;
    s32_c00_20dd20 unk8E0;
    f32_c00_20dd20 swim_lean_vel;
    f32_c00_20dd20 swim_roll_velocity;
    f32_c00_20dd20 unk8EC;
    s32_c00_20dd20 puff_timer;
    s32_c00_20dd20 unk8F4;
    s16_c00_20dd20 pack_particle_timer;
    u8_c00_20dd20 pad_8FA[0x2];
    f32_c00_20dd20 drown_sink_speed;
    s16_c00_20dd20 cw_turn_frames;
    s16_c00_20dd20 swim_square_hold_frames;
    s16_c00_20dd20 unk904;
    s16_c00_20dd20 unk906;
    s16_c00_20dd20 unk908;
    s16_c00_20dd20 splash_effect_timer;
    s16_c00_20dd20 swim_hard_press_frames;
    s16_c00_20dd20 jump_button_frames;
    s32_c00_20dd20 swim_cross_tap_count;
    s32_c00_20dd20 last_cross_press_frame;
    u8_c00_20dd20 pad_918[0x8];
    Vec4_c00_20dd20 move_impulse;
    f32_c00_20dd20 unk930;
    f32_c00_20dd20 unk934;
    f32_c00_20dd20 unk938;
    u8_c00_20dd20 pad_93C[0x4];
    f32_c00_20dd20 fall_state_gravity;
    f32_c00_20dd20 stickless_decel;
    f32_c00_20dd20 fatal_fall_roll_rate;
    f32_c00_20dd20 fixed_pivot_rot_y;
    f32_c00_20dd20 pivot_rot_x_target;
    f32_c00_20dd20 fatal_fall_tumble_target_y;
    u8_c00_20dd20 pad_958[0x8];
    f32_c00_20dd20 grapple_pull_speed;
    struct Moby_c00_20dd20 *grapple_target;
    s32_c00_20dd20 grapple_target_in_sight;
    f32_c00_20dd20 target_arrival_frames;
    f32_c00_20dd20 unk970;
    s32_c00_20dd20 grapple_pull_ending;
    s32_c00_20dd20 unk978;
    f32_c00_20dd20 tether_radius;
    f32_c00_20dd20 grapple_target_radius;
    f32_c00_20dd20 unk984;
    s32_c00_20dd20 unk988;
    u8_c00_20dd20 pad_98C[0x4];
    struct Moby_c00_20dd20 *swing_target;
    struct Moby_c00_20dd20 *swing_target_moby;
    f32_c00_20dd20 swing_target_dist;
    s16_c00_20dd20 unk99C;
    s16_c00_20dd20 swing_target_in_sight;
    f32_c00_20dd20 unk9A0;
    f32_c00_20dd20 unk9A4;
    f32_c00_20dd20 swing_length_vel;
    f32_c00_20dd20 swing_yaw;
    f32_c00_20dd20 swing_gravity;
    f32_c00_20dd20 swing_push_speed;
    f32_c00_20dd20 swing_yaw_vel;
    s16_c00_20dd20 swing_anim_dir;
    s16_c00_20dd20 swing_entry_push;
    u8_c00_20dd20 pad_9C0[0x4];
    f32_c00_20dd20 swing_rope_spring_k;
    f32_c00_20dd20 unk9C8;
    f32_c00_20dd20 swing_rope_max_speed;
    f32_c00_20dd20 rail_yaw;
    f32_c00_20dd20 rail_ride_pitch;
    u8_c00_20dd20 pad_9D8[0x4];
    f32_c00_20dd20 unk9DC;
    f32_c00_20dd20 rail_pull_step;
    f32_c00_20dd20 roll_spring_velocity;
    f32_c00_20dd20 rail_ride_pitch_vel;
    u8_c00_20dd20 pad_9EC[0x64];
    u8_c00_20dd20 *throw_target;
    struct Moby_c00_20dd20 *melee_target;
    s32_c00_20dd20 heading_locked;
    f32_c00_20dd20 attack_target_yaw;
    s32_c00_20dd20 attack_record;
    s32_c00_20dd20 unkA64;
    f32_c00_20dd20 pushback_yaw;
    f32_c00_20dd20 speed_scale;
    f32_c00_20dd20 slam_descending;
    f32_c00_20dd20 aim_yaw;
    f32_c00_20dd20 aim_pitch;
    s32_c00_20dd20 unkA7C;
    s32_c00_20dd20 last_landed_moby;
    u8_c00_20dd20 *unkA84;
    u8_c00_20dd20 *unkA88;
    void *vehicle_mode3_moby;
    f32_c00_20dd20 anim_speed;
    f32_c00_20dd20 anim_frame_rate;
    s32_c00_20dd20 anim_flags;
    s32_c00_20dd20 anim_changing;
    s32_c00_20dd20 anim_blend_curve;
    s32_c00_20dd20 anim_blend_step;
    f32_c00_20dd20 anim_frame;
    f32_c00_20dd20 anim_frame_delta;
    s32_c00_20dd20 anim_loop_frame;
    s32_c00_20dd20 queued_anim_frame;
    s32_c00_20dd20 queued_frame_request;
    u8_c00_20dd20 pad_ABC[0x244];
    u8_c00_20dd20*unkD00;
    u8_c00_20dd20*unkD04;
    u8_c00_20dd20 *held_item_anim_blend[2];
    u8_c00_20dd20 *unkD10;
    s32_c00_20dd20 unkD14;
    u8_c00_20dd20 pad_D18[0x8];
    f32_c00_20dd20 unkD20;
    u8_c00_20dd20 pad_D24[0xC];
    struct HeroManipulator_c00_20dd20 manipulators[7];
    u8_c00_20dd20 pad_EF0[0x100];
    struct HeroRandTimer_c00_20dd20 rand_timer;
    u8_c00_20dd20 pad_FFC[0x8];
    s32_c00_20dd20 unk1004;
    s32_c00_20dd20 unk1008;
    u8_c00_20dd20 pad_100C[0x4];
    s32_c00_20dd20 unk1010;
    f32_c00_20dd20 idle_look_ease_rate;
    f32_c00_20dd20 unk1018;
    u8_c00_20dd20 pad_101C[0x8];
    s32_c00_20dd20 unk1024;
    s32_c00_20dd20 unk1028;
    u8_c00_20dd20 pad_102C[0x8];
    s32_c00_20dd20 unk1034;
    s32_c00_20dd20 unk1038;
    u8_c00_20dd20 pad_103C[0x8];
    s32_c00_20dd20 unk1044;
    s32_c00_20dd20 unk1048;
    u8_c00_20dd20 pad_104C[0x8];
    s32_c00_20dd20 unk1054;
    s32_c00_20dd20 unk1058;
    u8_c00_20dd20 pad_105C[0x34];
    struct HeroItemSlot_c00_20dd20 items[7];
    u8_c00_20dd20 pad_12C0[0x20];
    s16_c00_20dd20 ground_type;
    u8_c00_20dd20 on_ice_ground;
    u8_c00_20dd20 selector_4;
    u8_c00_20dd20 base_condition;
    u8_c00_20dd20 selector_1;
    u8_c00_20dd20 selector_3;
    u8_c00_20dd20 on_magnet_ground;
    u8_c00_20dd20 selector_5;
    u8_c00_20dd20 unk12E9;
    u8_c00_20dd20 on_slip_ground;
    u8_c00_20dd20 selector_11;
    u8_c00_20dd20 selector_13;
    u8_c00_20dd20 ground_sound_class;
    u8_c00_20dd20 unk12EE;
    u8_c00_20dd20 pad_12EF[0x301];
    struct Moby_c00_20dd20 *ship_moby;
    s16_c00_20dd20 ship_oclass;
    u8_c00_20dd20 ship_ammo;
    u8_c00_20dd20 ship_ammo_max;
    u8_c00_20dd20 unk15F8;
    u8_c00_20dd20 unk15F9;
    u8_c00_20dd20 unk15FA;
    u8_c00_20dd20 unk15FB;
    f32_c00_20dd20 ship_hp;
    f32_c00_20dd20 ship_hp_max;
    f32_c00_20dd20 unk1604;
    s32_c00_20dd20 ship_hp_percent;
    s16_c00_20dd20 unk160C;
    u8_c00_20dd20 unk160E;
    u8_c00_20dd20 ship_flags;
    u8_c00_20dd20 pad_1610[0x4];
    s32_c00_20dd20 unk1614;
    u8_c00_20dd20 pad_1618[0x8];
    void *unk1620;
    void *unk1624;
    f32_c00_20dd20 glow_phase;
    f32_c00_20dd20 unk162C;
    s32_c00_20dd20 vehicle_hp;
    s16_c00_20dd20 unk1634;
    s16_c00_20dd20 vehicle_triangle_cooldown;
    u8_c00_20dd20 pad_1638[0x8];
    Vec4_c00_20dd20 target_pose;
    s32_c00_20dd20 unk1650;
    u8_c00_20dd20 pad_1654[0xC];
    s32_c00_20dd20 sink_jump_count;
    u8_c00_20dd20 pad_1664[0xC];
    struct MobyTrail_c00_20dd20 trail;
    u8_c00_20dd20 unk17B0[0x140];
    struct HeroEase_c00_20dd20 item_eases[3];
    Vec4_c00_20dd20 pos_history[32];
    u8_c00_20dd20 saved_pos_rot[0x20];
    f32_c00_20dd20 stick_x;
    f32_c00_20dd20 stick_y;
    u8_c00_20dd20 pad_1D28[0x28];
    Vec4_c00_20dd20 unk1D50[4];
    struct HeroManipulator_c00_20dd20 unk1D90;
    Vec4_c00_20dd20 draw_queue_vec[8];
    s32_c00_20dd20 draw_queue_val[8];
    f32_c00_20dd20 draw_queue_f0[8];
    f32_c00_20dd20 draw_queue_f1[8];
    union {
        s16_c00_20dd20 draw_queue_count;
        s16_c00_20dd20 unk1EB0;
    };
    s16_c00_20dd20 unk1EB2;
    u8_c00_20dd20 pad_1EB4[0x1C];
    f32_c00_20dd20 unk1ED0;
    f32_c00_20dd20 unk1ED4;
    f32_c00_20dd20 unk1ED8;
    u8_c00_20dd20 pad_1EDC[0x8];
    f32_c00_20dd20 unk1EE4;
    u8_c00_20dd20 pad_1EE8[0x20];
    s32_c00_20dd20 unk1F08;
    u8_c00_20dd20 pad_1F0C[0x4];
    Vec4_c00_20dd20 unk1F10;
    f32_c00_20dd20 unk1F20;
    f32_c00_20dd20 unk1F24;
    f32_c00_20dd20 unk1F28;
    u8_c00_20dd20 pad_1F2C[0x13];
    s8_c00_20dd20 unk1F3F;
    f32_c00_20dd20 unk1F40;
    u8_c00_20dd20 pad_1F44[0x1C];
    u8_c00_20dd20 unk1F60[0x40];
    u8_c00_20dd20 unk1FA0[0x40];
    u8_c00_20dd20 *unk1FE0;
    u8_c00_20dd20 pad_1FE4[0x11];
    u8_c00_20dd20 unk1FF5;
    u8_c00_20dd20 ammo_used;
    u8_c00_20dd20 ammo_capacity;
    u8_c00_20dd20 pad_1FF8[0x28];
    u8_c00_20dd20 *unk2020[6];
    u8_c00_20dd20 pad_2038[0x8];
    struct Moby_c00_20dd20 *metal_detector_target;
    f32_c00_20dd20 detector_target_dist;
    s32_c00_20dd20 nearby_moby_close;
    u8_c00_20dd20 pad_204C[0x4];
    struct Moby_c00_20dd20 *unk2050[10];
    s32_c00_20dd20 sucked_moby_count;
    s32_c00_20dd20 suck_cannon_incoming;
    struct Moby_c00_20dd20 *moby;
    struct HeroState_c00_20dd20 state;
    u8_c00_20dd20 vehicle_mode;
    u8_c00_20dd20 unk20A5;
    u8_c00_20dd20 force_slot0_item8;
    u8_c00_20dd20 item_select_blocked;
    u8_c00_20dd20 item_anim_active;
    u8_c00_20dd20 wading;
    u8_c00_20dd20 unk20AA;
    u8_c00_20dd20 unk20AB;
    u8_c00_20dd20 item_use_locked;
    u8_c00_20dd20 tilt_disabled;
    u8_c00_20dd20 unk20AE;
    u8_c00_20dd20 unk20AF;
    u8_c00_20dd20 hydro_tank_full;
    u8_c00_20dd20 fall_fade_pending;
    u8_c00_20dd20 unk20B2;
    u8_c00_20dd20 gravity_mode;
    u8_c00_20dd20 unk20B4;
    u8_c00_20dd20 wrench_combo_step;
    u8_c00_20dd20 pad_20B6[0x2];
    s32_c00_20dd20 pending_item[7];
    s32_c00_20dd20 selected_item[7];
    s32_c00_20dd20 saved_item[7];
    s32_c00_20dd20 restore_item[7];
    f32_c00_20dd20 yaw_history[32];
    s32_c00_20dd20 yaw_history_index;
    s32_c00_20dd20 yaw_ring_count;
    s32_c00_20dd20 pos_history_index;
    s32_c00_20dd20 pos_history_count;
    u8_c00_20dd20 pad_21B8[0x20];
    s16_c00_20dd20 unk21D8[8][4];
    union {
        struct {
            s32_c00_20dd20 grind_voice;
            s32_c00_20dd20 unk221C;
            s32_c00_20dd20 voice_slot2;
            s32_c00_20dd20 unk2224;
            s32_c00_20dd20 voice_slot4;
            s32_c00_20dd20 pack_swim_voice;
            s32_c00_20dd20 voice_slot6;
            s32_c00_20dd20 board_boost_voice;
        };
        s32_c00_20dd20 sound_slots[8];
    };
    union {
        struct Moby_c00_20dd20 *unk2238[8];
        struct Moby_c00_20dd20 *sound_slot_mobys[8];
    };
    u8_c00_20dd20 pad_2258[0x18];
    s32_c00_20dd20 fidget_index;
    s32_c00_20dd20 unk2274;
    void *unk2278;
    s32_c00_20dd20 unk227C;
    struct Moby_c00_20dd20 *damage_source_moby;
    s32_c00_20dd20 unk2284;
    f32_c00_20dd20 reach_distance;
    f32_c00_20dd20 reach_height;
    f32_c00_20dd20 unk2290;
    s32_c00_20dd20 item_hold_anim;
    s32_c00_20dd20 item_anim_item_id;
    f32_c00_20dd20 stick_magnitude;
    s32_c00_20dd20 underwater_air;
    f32_c00_20dd20 water_depth;
    struct HeroHealth_c00_20dd20 health;
    s32_c00_20dd20 unk22B4;
    f32_c00_20dd20 current_path_dist;
    u8_c00_20dd20 pad_22BC[4];
    void *unk22C0;
    s32_c00_20dd20 slot3_anim_timer;
    s16_c00_20dd20 unk22C8;
    u8_c00_20dd20 unk22CA;
    u8_c00_20dd20 vent_updraft;
    s16_c00_20dd20 unk22CC;
    s16_c00_20dd20 unk22CE;
    u8_c00_20dd20 pad_22D0[0x2];
    s16_c00_20dd20 item_selector_timer;
    s16_c00_20dd20 quick_swap_tap_timer;
    s16_c00_20dd20 quick_swap_tap_window;
    s16_c00_20dd20 pack_disabled;
    s16_c00_20dd20 path_constrain_pending;
    s16_c00_20dd20 unk22DC;
    s16_c00_20dd20 vehicle_toggle_timer;
    s16_c00_20dd20 glow_pulse_level;
    s16_c00_20dd20 pad22E2;
    s32_c00_20dd20 unk22E4;
    f32_c00_20dd20 unk22E8;
    f32_c00_20dd20 unk22EC;
    f32_c00_20dd20 unk22F0;
    s32_c00_20dd20 color_fade_active;
    s32_c00_20dd20 tint_color;
    s16_c00_20dd20 unk22FC;
    s16_c00_20dd20 unk22FE;
    s32_c00_20dd20 unk2300;
};
typedef struct Hero_c00_20dd20 Hero_c00_20dd20;
typedef struct HeroState_c00_20dd20 HeroState_c00_20dd20;
typedef struct HeroMotion_c00_20dd20 HeroMotion_c00_20dd20;
typedef struct HeroItemSlot_c00_20dd20 HeroItemSlot_c00_20dd20;
typedef struct HeroManipulator_c00_20dd20 HeroManipulator_c00_20dd20;
typedef struct HeroRandTimer_c00_20dd20 HeroRandTimer_c00_20dd20;
typedef struct HeroHealth_c00_20dd20 HeroHealth_c00_20dd20;
typedef struct HeroEase_c00_20dd20 HeroEase_c00_20dd20;
typedef struct HeroVelocityKey_c00_20dd20 HeroVelocityKey_c00_20dd20;
typedef struct MobyTrail_c00_20dd20 MobyTrail_c00_20dd20;
struct Moby_c00_20dd20;
extern char D_0013E533[];

int func_L00_0020D5B8(char *a, int b);

int func_L00_0020D5D0(int id, float *out);
extern float D_0015ED60 ;
extern void *func_L00_0024F028(void *, int);
extern float func_00213ED8(float *, float, float);
extern void func_L00_0024F0E8(void *, void *);
extern void func_L00_0024F190(void *, unsigned char *);
extern int func_001F9688(int);
extern short D_L00_0015F6A0;
extern float D_0015ED64 ;
extern unsigned char D_L00_00197BC0[];
extern char *D_L00_00197300[];
extern struct HeroEase_c00_20dd20 D_L00_0017A680[];
extern char D_L00_0017CA80[];
extern int func_L00_002050B8(int);
struct AnimBlendSlot_c00_20dd20 {
    u8_c00_20dd20 unk_00[6];
    u8_c00_20dd20 has_pose[2];
    f32_c00_20dd20 weight;
    u8_c00_20dd20 pad_0C[0xC];
    void *class_header;
    u8_c00_20dd20 pad_1C[4];
    u8_c00_20dd20 frame;
    u8_c00_20dd20 next_frame;
    u8_c00_20dd20 seq;
    u8_c00_20dd20 next_seq;
    f32_c00_20dd20 blend;
    f32_c00_20dd20 unk_28;
    f32_c00_20dd20 blend_rate;
    s16_c00_20dd20 seq_changed;
    u8_c00_20dd20 blending;
    u8_c00_20dd20 blend_started;
    s32_c00_20dd20 releasing;
    char *pose_buffers[2];
};
void FUN_L00_0020dd20(void) {
    int i;
    for (i = 0; i < 2; i++) {
        Hero_c00_20dd20 *g = (Hero_c00_20dd20 *)(D_0013E533 + 0xE1D);
        struct AnimBlendSlot_c00_20dd20 *o;
        int diff;
        struct { short v; } w;
        int r;
        int k;
        int j;
        float out;
        struct AnimBlendSlot_c00_20dd20 *t0 = (struct AnimBlendSlot_c00_20dd20 *)(((char **)(D_0013E533 + 0x1B1D))[i]);
        if (t0 != 0 && t0->releasing != 0) {
            if (func_00213ED8(&t0->weight, 0.0f, D_0015ED60 * 0.1f) == 0.0f) {
                func_L00_0024F0E8(*(char **)&g->moby, &(((char **)(D_0013E533 + 0x1B1D))[i]));
            }
        } else {
            int c = g->unk20AB;
            int want = 0;
            if (c != 0 && (i == 0 || c == 2)) {
                want = 1;
            }
            {
                Hero_c00_20dd20 *h = (Hero_c00_20dd20 *)(D_0013E533 + 0xE1D);
                if (h->unk20AF != 0) {
                    struct AnimBlendSlot_c00_20dd20 *t1;
                    want = 0;
                    t1 = (struct AnimBlendSlot_c00_20dd20 *)(((char **)(D_0013E533 + 0x1B1D))[i]);
                    if (t1 != 0) {
                        t1->releasing = 1;
                    }
                }
            }
            if (want) {
                Hero_c00_20dd20 *h = (Hero_c00_20dd20 *)(D_0013E533 + 0xE1D);
                if ((((char **)(D_0013E533 + 0x1B1D))[i]) == 0) {
                    char *n = (char *)func_L00_0024F028(*(char **)&h->moby, ((int *)&D_L00_0015F6A0)[i]);
                    (((char **)(D_0013E533 + 0x1B1D))[i]) = n;
                    ((struct AnimBlendSlot_c00_20dd20 *)n)->weight = 0.0f;
                    *(char **)((((char **)(D_0013E533 + 0x1B1D))[i]) + 0x18) = D_L00_00197300[D_L00_00197BC0[i + 1]];
                }
            } else {
                Hero_c00_20dd20 *h = (Hero_c00_20dd20 *)(D_0013E533 + 0xE1D);
                struct AnimBlendSlot_c00_20dd20 *t2 = (struct AnimBlendSlot_c00_20dd20 *)(((char **)(D_0013E533 + 0x1B1D))[i]);
                if (t2 == 0) {
                    continue;
                }
                if (func_00213ED8(&t2->weight, 0.0f, D_0015ED60 * 0.1f) == 0.0f) {
                    func_L00_0024F0E8(*(char **)&h->moby, &(((char **)(D_0013E533 + 0x1B1D))[i]));
                }
            }
        }
        {
            Hero_c00_20dd20 *h = (Hero_c00_20dd20 *)(D_0013E533 + 0xE1D);
            char *hr;
            o = (struct AnimBlendSlot_c00_20dd20 *)(((char **)(D_0013E533 + 0x1B1D))[i]);
            if (o == 0) {
                continue;
            }
            hr = *(char **)&h->moby;
            diff = *(unsigned char *)(hr + 0x52) != *(unsigned char *)(hr + 0x53);
            if (*(int *)&h->held_item_anim_blend[0] != 0 && h->unkD14 == 0) {
                func_00213ED8(&o->weight, 0.0f, D_0015ED60 * 0.1f);
            } else if (o->releasing == 0) {
                func_00213ED8(&o->weight, 1.0f, D_0015ED60 * 0.1f);
            }
        }
        w.v = 0;
        if (diff) {
            Hero_c00_20dd20 *h = (Hero_c00_20dd20 *)(D_0013E533 + 0xE1D);
            if (h->anim_changing != 0) {
                int a = func_L00_002050B8(o->next_seq);
                if (a != func_L00_002050B8(*(unsigned char *)(*(char **)&h->moby + 0x53))) {
                    w.v = o->seq != o->next_seq;
                }
            }
        }
        o->seq_changed = w.v;
        r = func_L00_0020D5D0(*(unsigned char *)(*(char **)&g->moby + 0x53), &out);
        if (r != 0 || o->blending != 0) {
            if (out != 0.0f) {
                float x = D_0015ED64;
                char *tb = (char *)D_L00_0017A680;
                *(float *)(tb + 0x694) = out;
                *(float *)(tb + 0x5E4) = out;
                *(float *)(tb + 0x6D4) = x * 0.05f;
                *(float *)(tb + 0x6D8) = x * 0.3f;
                *(float *)(tb + 0x624) = x * 0.05f;
                *(float *)(tb + 0x628) = x * 0.3f;
            }
            if (r != 0 && (o->blending == 0 || o->next_seq != 0)) {
                {
                    if (o->seq == o->next_seq ||
                        0.5f < o->blend) {
                        o->seq = *(char *)&o->next_seq;
                        o->frame = *(char *)&o->next_frame;
                    }
                    o->blend = 0.0f;
                    o->next_seq = 0;
                    o->next_frame = 0;
                    o->unk_28 = 1.0f;
                    o->blend_rate = 1.0f / (float)func_001F9688(0xF);
                    o->blending = 1;
                    o->blend_started = 0;
                }
            }
            if (r == 0) {
                if (o->blend_started == 0) {
                    char *hr = *(char **)&g->moby;
                    o->next_seq = func_L00_0020D5B8(hr, *(unsigned char *)(hr + 0x53));
                    o->next_frame = 0;
                    o->blend_started = 1;
                    o->blend = 0.0f;
                    o->unk_28 = 0.0f;
                } else {
                    float f = o->blend + D_0015ED60 * 0.1f;
                    o->blend = f;
                    if (1.0f < f) {
                        o->blend = 1.0f;
                    }
                    if (o->blend == 1.0f) {
                        o->blending = 0;
                    }
                }
            } else {
                o->unk_28 = 1.0f;
            }
            for (j = 0; j < 2; j++) {
                unsigned char *id = &o->seq;
                char *e = (char *)o->has_pose;
                if (id[j] < 0x17) {
                    e[j] = 1;
                    *(char **)((char *)o + 0x38 + j * 4) = D_L00_0017CA80 + (i * 0x800 + j * 0x400);
                } else {
                    e[j] = 0;
                }
            }
            func_L00_0024F190(*(char **)&g->moby, (unsigned char *)o);
        } else {
            if (o->seq_changed == 0) {
                Hero_c00_20dd20 *h = (Hero_c00_20dd20 *)(D_0013E533 + 0xE1D);
                for (k = 0; k < 2; k++) {
                    if (!diff || k == 1) {
                        char *hr = *(char **)&h->moby;
                        unsigned char *p = &o->seq;
                        p[k] = func_L00_0020D5B8(hr, *(unsigned char *)(hr + k + 0x52));
                    }
                }
            }
            if (o->seq_changed != 0) {
                float f = o->blend + D_0015ED60 * 0.15f;
                o->blend = f;
                if (1.0f <= f) {
                    o->seq = *(char *)&o->next_seq;
                    o->frame = *(char *)&o->next_frame;
                    o->seq_changed = 0;
                }
            } else {
                if (!diff) {
                    o->frame = *(unsigned char *)(*(char **)&g->moby + 0x50);
                }
                if (o) {
                    o->next_frame = *(unsigned char *)(*(char **)&g->moby + 0x51);
                    o->blend = *(float *)(*(char **)&g->moby + 0x54);
                } else {
                    o->next_frame = *(unsigned char *)(*(char **)&g->moby + 0x51);
                    o->blend = *(float *)(*(char **)&g->moby + 0x54);
                }
            }
            for (j = 0; j < 2; j++) {
                unsigned char *id = &o->seq;
                char *e = (char *)o->has_pose;
                if (id[j] < 0x17) {
                    e[j] = 1;
                    *(char **)((char *)o + 0x38 + j * 4) = D_L00_0017CA80 + (i * 0x800 + j * 0x400);
                } else {
                    e[j] = 0;
                }
            }
            func_L00_0024F190(*(char **)&g->moby, (unsigned char *)o);
        }
    }
}
extern char D_0013E533[];
extern short D_L00_0015F6A0;
extern void *func_L00_0024F028(void *, int);
extern int func_001F9688(int);
extern s32_c00_20dd20 func_001F9688(s32_c00_20dd20);
struct Manip_c00_20dd20;
struct GifEntry_c00_20dd20;
struct AnimSeq_c00_20dd20;
struct MobyClass_c00_20dd20 {
    u8_c00_20dd20 pad_0[0xC];
    u8_c00_20dd20 seq_count;
    u8_c00_20dd20 pad_D[3];
    u32_c00_20dd20 collision;
    u8_c00_20dd20 pad_14[0x8];
    void *retarget_table;
    struct GifEntry_c00_20dd20 *gifs;
    f32_c00_20dd20 scale;
    s32_c00_20dd20 sound_defs;
    void **callbacks;
    u8_c00_20dd20 pad_30[0x14];
    u16_c00_20dd20 flags;
    s16_c00_20dd20 type;
    struct AnimSeq_c00_20dd20 *seqs[1];
};
struct Moby_c00_20dd20 {
    Vec4f_c00_20dd20 bsphere;
    Vec4f_c00_20dd20 pos;
    u8_c00_20dd20 state;
    u8_c00_20dd20 group;
    u8_c00_20dd20 class_slot;
    u8_c00_20dd20 alpha;
    struct MobyClass_c00_20dd20 *pclass;
    struct Moby_c00_20dd20 *next;
    f32_c00_20dd20 scale;
    u8_c00_20dd20 update_dist;
    u8_c00_20dd20 visible;
    s16_c00_20dd20 draw_dist;
    u16_c00_20dd20 flags;
    u16_c00_20dd20 occlusion;
    u64_c00_20dd20 spawn_frame;
    Vec4f_c00_20dd20 rot;
    u8_c00_20dd20 frame;
    u8_c00_20dd20 prev_frame;
    u8_c00_20dd20 seq;
    u8_c00_20dd20 prev_seq;
    f32_c00_20dd20 anim_time;
    f32_c00_20dd20 anim_speed;
    u8_c00_20dd20 pad5C[8];
    struct Manip_c00_20dd20 *manips;
    void *cur_frame_data;
    void *prev_frame_data;
    u8_c00_20dd20 anim_flags;
    u8_c00_20dd20 unk71;
    u8_c00_20dd20 unk72;
    u8_c00_20dd20 unk73;
    void (*update)(struct Moby_c00_20dd20 *moby);
    u8_c00_20dd20 *pvars;
    u8_c00_20dd20 unk7C;
    u8_c00_20dd20 sound_slot;
    u8_c00_20dd20 anim_trigger_count;
    u8_c00_20dd20 shadow_range;
    s32_c00_20dd20 unk80;
    union {
        struct {
            u8_c00_20dd20 pad84[2];
            s16_c00_20dd20 unk86;
            u8_c00_20dd20 pad88[8];
        };
        struct {
            f32_c00_20dd20 height_min;
            f32_c00_20dd20 height_max;
            u8_c00_20dd20 pad90_88[4];
        };
    };
    s32_c00_20dd20 glow_color;
    u32_c00_20dd20 collision;
    s32_c00_20dd20 collision_disable;
    u8_c00_20dd20 pad9C[8];
    u8_c00_20dd20 hit_slot;
    u8_c00_20dd20 padA5;
    s16_c00_20dd20 oclass;
    u8_c00_20dd20 padA8[8];
    u8_c00_20dd20 mission;
    u8_c00_20dd20 unkB1;
    u16_c00_20dd20 save_id;
    s16_c00_20dd20 unkB4;
    s16_c00_20dd20 unkB6;
    void *parent;
    u8_c00_20dd20 command;
    u8_c00_20dd20 unkBD;
    u8_c00_20dd20 padBE[2];
    Vec4f_c00_20dd20 rot_row0;
    Vec4f_c00_20dd20 rot_row1;
    Vec4f_c00_20dd20 rot_row2;
    u8_c00_20dd20 padF0[0x10];
};
typedef struct Moby_c00_20dd20 Moby_c00_20dd20;
typedef struct MobyClass_c00_20dd20 MobyClass_c00_20dd20;
extern struct HeroEase_c00_20dd20 D_L00_0017A680[];
extern char D_0013E533[];
extern int func_001F9688(int);
extern float D_0015ED64 ;
extern char D_0013E533[];
extern char D_0013E533[] ;
extern char D_0013E533[];

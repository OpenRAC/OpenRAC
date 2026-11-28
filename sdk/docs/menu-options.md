# Menu options

A mod can turn an option of the port's menu on or off with `menu_option_set` (C) or
`mod.menu_option_set` (C++).

- `OPENRAC_MENU_OPTION_ON`: the option can be used, even where the port greys it out.
- `OPENRAC_MENU_OPTION_OFF`: the option is greyed out, with your mod's name beside it. It is shown,
  but cannot be changed.
- `OPENRAC_MENU_OPTION_DEFAULT`: the port decides again.

The last call for an option wins. When a mod unloads, its calls go with it, and what another mod
set comes back.

An option that is off also ignores its key (F2 frame counter, F4 Enhanced, F9 and F10 volume, F11
full screen, and the developer keys) and its row on the game's own Video page. The launcher and the
settings file can still change it. Script mods use `openrac.menu_option_set(name, "on" | "off" | "default")`.

Example: a mod that sets the frame rate itself greys out the menu's Frame rate:

```cpp
mod.menu_option_set("frame_rate", OPENRAC_MENU_OPTION_OFF);
```

```lua
openrac.menu_option_set("frame_rate", "off")
```

A mod that vibrates the pad with `set_motors` does not need this: the port shows the Vibration
settings again by itself.

## Names

Names never change. A port that does not know a name ignores it. Panels, notes and Quit are not
listed: they cannot be turned off.

The player's essentials are never turned off, only on: `volume`, `full_screen`, `display_mode`,
`monitor`, `menu_size` and every Controls option.

- Graphics: `full_screen`, `display_mode`, `monitor`, `menu_size`, `picture`, `gameplay_fov`, `hud`, `render_scale`, `msaa`, `anisotropy`, `draw_distance`, `active_distance`, `frame_rate`, `frame_cap`, `vsync`, `frame_counter`, `hdr`, `hdr_paper_white`, `hdr_peak`, `hdr_highlights`, `hdr_sdr_hue`, `hdr_sdr_chroma`, `hdr_blowout`, `low_latency`, `responsive_controls`, `gpu_breadcrumbs`
- Enhanced: `enhanced_graphics`, `enhanced_low`, `enhanced_medium`, `enhanced_high`, `shadow_quality`, `shadow_filter`, `occlusion_quality`, `texture_upscaling`, `effects_detail`, `early_lighting_quality`, `upscaling`, `sharpening`, `motion_blur`, `motion_blur_quality`, `color_correction`, `brightness`, `enhanced_reset`, `occlusion_hi_z`, `enhanced_advanced`, `enhanced_materials`
- Game: `volume`, `mute_in_background`, `skip_cutscenes`
- Controls: `controls_file`, `controls_player`, `controls_player_reset`, `trigger_point`, `face_buttons`, `pad_background`, `rumble_strength`, `rumble_style`, `rumble_triggers`, `rumble_phone`, `players_reset`, `touch_action_buttons`, `touch_button_size`, `touch_camera_speed`, `touch_move_controls`, `controls_reset`
- Developer: `debug_overlay`, `moby_picker`, `object_viewer`, `meat_proxy_review`, `level_select`, `keep_paused`, `step_tick`, `speed`, `screenshot`, `record_gameplay`, `record_length`, `record_sweep`, `slow_cpu`, `freecam`, `freecam_mode`, `freecam_inspect`, `freecam_speed`, `freecam_reset`, `freecam_exit`

# Going Commando's armour vendor and armour models (research for NG++ 0.6.0, 2026-11-22)

Addresses: v1.01 NTSC-U first, then PAL and v2.00 (NTSC-U Greatest Hits). Function names as the
OpenRAC port's build of each disc names them (libraries.tsv / sdk/bindings/rac2.json), so `func_L00_`
of PAL and v2.00 is that release's level-0 address. Sources: rac2-decomp (OpenRAC games/rac2/decomp
at 4ea93cea), the discs' executables and level programs disassembled (~/RACSwarm/ngpp-armour/tools/
dis.py, ldis.py), OpenRAC develop 1a18038fe, the port's export of the levels. The model side is in
more detail in armour-models.md beside this file.

## 1. The vendor is the weapon vendor's machine in mode 3

There is no armour shop moby of its own. The armour vendor is the **Megacorp Vendor, class 11**
(MOBY_CLASS_MEGACORP_VENDOR), the same class as the weapon vendor; its pvars choose the menu:

- `OpenVendorMenu` (func_002F8178; PAL and v2.00 carry the same C, src/pal/game/002F90E8-open-g8.c,
  src/us2/game/002F8FF8-open-g8.c), for a class-11 moby, reads `*(int *)(pvars + 0x94)`:
  0 weapons (mode 0), 1 the channel list (mode 1), 4 the equipment (mode 2), **2 armour (mode 3)**,
  3 weapons with field744 set. A caller can also open it with a mode 0-3 in place of the moby
  (game mode 5's `OpenVendorMenu(g_pending_mode_arg_a)`, func_002B3868): it then makes a vendor
  moby in front of the camera. `g_shop_saved_menu_mode` (0x1AD1C4), when not -1, forces the mode.
- Placed class-11 vendors (the export's placements): levels 1, 2, 3, 4, 6, 7, 8, 9, 11, 12, 13, 14,
  16, 17, 18, 19 and 20 have one to four each. Their pvars +0x94, read in game (NG++ test runs on
  NTSC-U v1.01, 2026-11-23): **the armour vendors (2) are on Dobbo (level 9, at 470, 266), Boldan
  (13, at 314, 310), Snivelak (16, at 400, 491) and Grelbin (19, at 212, 278)**; Barlow's second
  vendor (364, 233) is 3 (weapons with field744 set); every other one is 0 (the weapon vendor). The
  game's prompt at it reads "Activate Armor Vendor".
- State: the vendor's state block **g_vendor_menu_state** (the weapon vendor's g_vendorUi), 0x760
  bytes at 0x0021F240 (PAL 0x0021F240, v2.00 0x0021F1C0). In a level the level's copy of the code
  uses the level's copy of the block, which the port maps back onto the executable's address
  (port/game/rac2-ntsc/tables/derived_names.json: D_L00_0022F7C0 -> 0x0021f240).
  - +0x28 the vendor moby, +0x2C the preview objects (0x100-byte mobys, not in the moby array),
    +0x34 their count, +0x50 mode, +0x80 the cursor, +0x84 the purchase dialog's state, +0x140 the
    list, +0x740 its count.
  - In mode 3 the list is **12-byte rows** (not the weapon list's 0x18): `{s32 text, s32 price,
    s32 armour}` (`PreviewArmor {id, resource, value}` in src/game/002F9BC0-preview-g8.c), up to 128.

## 2. The list

`shop_rebuild_unlocked_slot_list`, **func_002F71B8** (PAL func_002F8128, v2.00 func_002F8038), C in
src/game/002F71B8-g8.c:

- Clears the count and `g_shop_bolts_level_override` (gp -0x1E38, 0x1AD1B8).
- For each armour i from (worn armour + 1) to 4: offered when bit i of the flag byte
  `g_game_flag_table[0x8D]` (0x001395B8 + 0x8D) is set (the armour was unlocked on its planet), or
  when `flag_is_byte_set` (func_00288D58: `g_extras_unlocked_flag`, 0x1A7A0A, the extras of a
  finished game) says so.
- The row: text `0xEA92 + i` (the name), price `g_shop_slot_value_table[i]` (0x001AD268:
  0, 25 000, 100 000, 250 000, 1 000 000 bolts), armour i.
- So **with Carbonox (4) worn the list is empty**: the vendor has nothing to sell.
- The worn armour it starts from is `g_shop_saved_bolts_level` (0x1AD1C0), which OpenVendorMenu
  copied from the armour word as the menu opened.

## 3. Price and purchase

- No function prices an armour: the row carries its price, and the dialog reads it (+0x144 of the
  block, the row's +4). The weapon vendor's `Vendor_Price` (func_002F6510) is not used in mode 3.
- `shop_dispatch_purchase_confirm` (func_L00_00374708, every level) calls the dialog of the mode;
  mode 3's is **func_L00_00373D40** (UpdateSelectedItemPurchaseDialog; PAL func_L00_00376810,
  v2.00 func_L00_00375068), `void(s32 *answer)`, answer the level's word at gp +0x35A0. Its
  states (+0x84):
  - 1: the bolts below the price: the refusal sound (PlayMobySound 2), state 2.
  - 2: waits for the pad: Triangle (0x10) answers 0, Cross (0x40) answers 1; state 3.
  - 3: decides. Answer 1, the bolts at least the price and the worn armour (the level copy of
    g_shop_saved_bolts_level) not 4: `g_shop_bolts_level_override` = 1, `g_shop_bolts_level_alt`
    (gp -0x1E34, 0x1AD1BC) = the row's armour, the purchase sound (7), bolts -= price, the message
    0x2DCB, then func_L00_003744F8 closes the menu. Otherwise the message 0x2DCC (the worn armour
    is the best, or too few bolts) or, answer 0, nothing; state 0 and func_L00_003794D0.
- The armour level is written when the menu closes: `shop_close_sub_menu` (func_002F9198; PAL
  func_002FA118, v2.00 func_002FA028), mode 3: the armour word = alt when the override is set,
  else the saved one; `tex_load_level_pack(armour)`; with the override, a message from
  `g_shop_close_table` (0x1AD308) for the new armour.
- **The armour in the save**: s16 `g_player_stats` +0x18 = the save block (g_boltCount) +0x20:
  0x001A7A18 (PAL 0x001A7AA0, v2.00 0x001A7A20). 0 none, 1 Tetrafiber, 2 Duraplate, 3 Electrosteel,
  4 Carbonox. `AdjustPlayerHealth` (func_L00_002A4EF8, rac2.player.take_damage) scales each hit by
  g_player_damage_scale's entry for it. It is part of the save block, so the game saves it.
- Because the dialog refuses everything while Carbonox is worn, no list entry can be bought through
  the game's own purchase past Carbonox: a mod's tier needs the dialog itself (the hook below).

## 4. The UI text

- The row's first word, 0xEA92 + armour, is an **icon** id (hud widgets: 0xEA96 Carbonox's), not a
  text. The name under the preview is the armour's item record's name: the table at 0x001A8B60
  {none 0xAB2, 0xAAD Tetrafiber, 0xAAE, 0xAAF, 0xAB0 Carbonox Armor} on v1.01; Carbonox's name is
  0x0AB0 on v1.01 and PAL and 0x331B on v2.00 (found by text search in game and in the item records
  at 0x0023CF7C PAL / 0x0023CF84 v1.01 / 0x0023CF04 v2.00, which hold {icon 0xEA96, description,
  name, name}).
- Descriptions: `g_shop_armor_text_ids` (0x001AD250; PAL 0x001AD350, v2.00 0x001AD2D0), by armour:
  v1.01 0x2FD2..0x2FD5, PAL 0xEF6..0xEF9, v2.00 0x2E68..0x2E6B, shown by the preview as the
  scrolling message (shop_set_message_text); a row of armour 0 shows 0x3188.
- Purchase: 0x2DCB bought, 0x2DCC refused; on closing, `g_shop_close_table[armour]`.
- The game mode word is 5 in a vendor: 0x001A8BB0 (PAL 0x001A8C60, v2.00 0x001A8BB0).

## 5. What it shows: the preview

`PrepareVendorSelectionPreview`, **func_002FB1A0** (PAL func_002FC1D0, v2.00 func_002FC008), on
each move of the cursor, mode 3:

- When the row's armour is not the loaded pack (0x001A7290, PAL 0x001A7310, v2.00 0x001A7290) it
  loads that armour's pack over Ratchet's class 0 (`tex_load_level_pack`, func_002923C0, section 7).
- It makes the preview objects from `g_preview_classes` (0x1AD500 in the executable):
  `{0, 1290, 2675, 2675, -1}`: class 0, Ratchet himself in the loaded armour, turned toward the
  camera (`rotZ` = facing) and posed; 1290 drawn only for pack 4 (Carbonox) and 2675 twice for
  packs 3 and up (extra armour pieces on the manipulator joints 22-24). Scale 3.5 at (-9, 4.5, -1).
- The description of the row's armour as the message (section 4).
- On closing the menu the worn armour's pack is loaded back (section 3).
- So **the preview is Ratchet's own model in the selected armour**: what the player buys is what
  he will wear. In the port the same: the preview's class-0 mobys draw with the exported armour
  class the loaded pack names (port/game/rac2/profile.cpp `hero_class`, 6000 + pack).

## 6. How an armour is structured, swapped and drawn (summary of armour-models.md)

- RC2.HDR +0x4F08: nine packs; **the pack is the armour**: 0 Ratchet's suit, 1 Tetrafiber,
  2 Duraplate, 3 Electrosteel, 4 Carbonox; 5-8 the unlockable skins (func_00292078 swaps the
  requested pack for 6/8/7/5 when 0x1A7A49/42/4B/4A is set). Each pack is a class-0 mesh (113
  joints, one skin, one primitive per texture plus a chrome `_metal` mesh) and its own textures
  (8-bit indexed with a 256-colour CLUT): packs 0-3 a 256x256 body atlas, two 128x128 and a small
  one, in **one UV layout where Ratchet's fur, face and hands are the same texels in every pack**
  (pack 1 differs from pack 0 in 33% of its texels, 2 in 60%, 3 in 59%): armour plates and fur
  share textures and primitives. **Pack 4, Carbonox, is the whole-body suit**: its own 5,769-vertex
  mesh and one 256x256 texture, all of it armour (a helmet with red eyes).
- `tex_load_level_pack` (SetArmor, func_002923C0; PAL 0x00292CD0, v2.00 0x00292C70): textures to
  VRAM (func_00292078), draw packets (func_00292328), class header over class 0 (func_002934B8),
  then the loaded pack word. Writing the armour word alone changes nothing until one of its
  callers runs: the vendor's close (mode 3), its preview, the extras menu's skins
  (func_002CF5B0), a level's load.
- In the port: the export has class 0 = pack 0 and classes 6001-6008 = packs 1-8 (textures
  1000 + 16 x pack + slot); `hero_class` (profile.cpp) draws a class-0 moby with 6000 + the loaded
  pack (the skins' bytes first). A mod's colour on a moby is whole-instance (`moby_material`:
  tint, platinum, obsidian, energy; frame.cpp `apply_mod_material`, mesh.frag).

## 7. What this means for NG++'s armour tiers

- Sell them **at the armour vendor** (mode 3): a mod's rows appended to the mode-3 list after the
  game builds it (a hook on func_002F71B8). With Carbonox worn the game's list is empty, so the
  tiers are the whole list there, exactly like the game's next armour.
- A tier's row carries **armour 4**: the preview then loads and shows Carbonox, the model the tiers
  build on, and the game's list code, preview and close never see an armour past 4 (a row of 5+
  would index the pack table, the description table and the close table out of range).
- The purchase must be the mod's (the game refuses everything while Carbonox is worn): a hook on the
  mode-3 dialog (func_L00_00373D40) that, for a mod row at state 3 with the answer yes, pays and
  steps the tier, then lets the game close the dialog.
- The look: **Carbonox recoloured**. Carbonox is the only armour whose texture is all armour, so a
  whole-instance material recolours the armour and nothing of Ratchet; the lower armours share
  their texels with his fur and would need a per-texel mask from the export (later work, noted in
  the design). The recolour is a native material (a moby_material kind), so the hero in play and
  the vendor's preview objects (class 0 in the loaded pack 4) take the same look.

## 8. What was built (2026-11-23)

- OpenRAC (interface 1.34): hook points `rac2.armor_vendor.build_list` (func_002F71B8 / PAL
  func_002F8128 / v2.00 func_002F8038), `rac2.armor_vendor.buy` (func_L00_00373D40 / 00376810 /
  00375068, `void(u32 answer, u32 unused)`: the dispatcher passes two words), `rac2.vendor.preview`
  (func_002FB1A0 / 002FC1D0 / 002FC008); `OPENRAC_MOBY_MATERIAL_PAINT` (5, glow level bits 8-11)
  and `moby_material_class(mod, class, scope, material, rgb)`.
- NG++ 0.6.0: a row {0xEA96, tier price, armour 4} after the game's (empty with Carbonox worn);
  the name (0xAB0 / 0x331B) and description (0x2FD5 / 0xEF9 / 0x2E6B) retold while it is under
  the cursor; the purchase at the dialog's state 3 (pay, step, answer 0 so the game closes the
  dialog); class 0 painted in the world (tier worn) and the menu scope (tier under the cursor).
- Seen in hidden runs on Dobbo, all three discs: the vendor lists Alpha Armor at 900,000 under
  Carbonox's icon and name "Alpha Armor", the preview shows painted Carbonox (Gamma jade clearly),
  two purchases (2,500,000 bolts to 400,000), Gamma refused for bolts, and Ratchet in Beta's azure
  outside the vendor.

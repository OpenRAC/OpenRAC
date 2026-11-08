-- Generated from sdk/bindings/rac2.json by sdk/tools/gen_binding.py. Do not edit.
-- SPDX-License-Identifier: MIT
--
-- The `openrac` global as a script mod for rac2 sees it (docs/port/MODS.md, section 11): put this
-- file where your editor's Luau language server reads definitions (luau-lsp: --definitions).
-- A hook point is named by its permanent id (rac2.player.take_damage) or one of its aliases.

type HookPoint =
      "rac2.player.take_damage"
    | "rac2.combat.get_hit"
    | "rac2.weapon.take_ammo"
    | "rac2.moby.record_death"
    | "rac2.level.load"
    | "rac2.player.approach_speed"
    | "rac2.player.nanotech_steps"
    | "Hero_ApproachSpeed"
    | "Hero_NanotechSteps"
    | "Hero_TakeDamage"
    | "Level_Load"
    | "Moby_GetHitMessage"
    | "Moby_RecordDeath"
    | "Weapon_TakeAmmo"
type MemoryType = "u8" | "i8" | "u16" | "i16" | "u32" | "i32" | "f32"

type Fields = {
}

-- rac2.player.take_damage (Hero_TakeDamage) void(i32): Takes a hit's points from the hero's health (the hero at 0x189E20 on NTSC-U, health at +0x24CC), after the armour's scale; the hero's state machine calls it for each hit.
type Hero_TakeDamageFn = (amount: number) -> ()
-- rac2.combat.get_hit (Moby_GetHitMessage) u32(u32,u32,i32): The pending hit for a moby from the hit ring (the damage, f32 at +0x2C; its source at +0x38), or 0: what an enemy's damage path reads.
type Moby_GetHitMessageFn = (moby: number, mask: number, keep: number) -> number
-- rac2.weapon.take_ammo (Weapon_TakeAmmo) i32(i32,i32): Takes rounds of a weapon's ammo (item -1: the one in hand); a weapon with no capacity has endless ammo. Rac1's TakeAmmo.
type Weapon_TakeAmmoFn = (item: number, amount: number) -> number
-- rac2.moby.record_death (Moby_RecordDeath) void(u32,i32,i32): A moby's death recorded: its experience, the bestiary and its bolt drop (the moby's s16 at +0xB4); crates call it too.
type Moby_RecordDeathFn = (moby: number, flags: number, arg: number) -> ()
-- rac2.level.load (Level_Load) i32(i32,i32): Loads a level (new, or resumed after a death); it calls the gameplay file's loader.
type Level_LoadFn = (load: number, resume: number) -> number
-- rac2.player.approach_speed (Hero_ApproachSpeed) void(f32,f32): Eases the hero's speed (+0x1B4) toward his target speed (+0x1B0) each tick, at up when speeding up and down when slowing; rac1's approach_speed.
type Hero_ApproachSpeedFn = (up: number, down: number) -> ()
-- rac2.player.nanotech_steps (Hero_NanotechSteps) i32(u32,u32,u32): Adds the nanotech levels' steps of health to *count; the recount of the most health each tick adds the boosts after it (the HUD's experience bar calls it too, ignoring count).
type Hero_NanotechStepsFn = (a0: number, a1: number, count: number) -> number

declare openrac: {
    game: string,
    version: "ntsc" | "pal" | "ntsc2",
    platform: string,
    api: string,
    log: (...any) -> (),
    warn: (...any) -> (),
    setting: (key: string) -> any,
    hook: (name: HookPoint, fn: (original: (...number) -> ...number, ...number) -> ...number) -> (),
    before: (name: HookPoint, fn: (...number) -> ()) -> (),
    after: (name: HookPoint, fn: (...number) -> ()) -> (),
    on_frame: (fn: () -> ()) -> (),
    on_pad: (fn: (port: number, report: buffer) -> ()) -> (),
    read: (kind: MemoryType, address: number) -> number,
    write: (kind: MemoryType, address: number, value: number) -> (),
    fields: Fields,
    -- interface 1.2: the keyboard and mouse; a bind is a name of openrac/keys.h ("KeyW", "MouseLeft", "WheelUp")
    key: (bind: string) -> boolean,
    pressed: (bind: string) -> boolean,
    mouse: () -> (number, number, number, number),
    window: () -> (boolean, boolean, boolean),
    -- interface 1.3: the cursor over the game's picture, 0..1 each way (x * 512, y * 416 the game's 2D pixel), and whether it is over it
    pointer: () -> (number, number, boolean),
    take_input: (...("keyboard" | "mouse")) -> (),
    capture_mouse: (capture: boolean) -> (),
    -- interface 1.13: the on-screen pad's action buttons (MODS.md, "Touch controls")
    touch_define: (json: string) -> boolean,
    touch_icon: (name: string, svg: string) -> boolean,
    touch_push: (context: string?) -> (),
    touch_hint: (hint: string, on: boolean) -> (),
}

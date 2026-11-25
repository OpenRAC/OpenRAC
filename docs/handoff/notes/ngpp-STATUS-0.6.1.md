# NG++ 0.6.1 (2026-11-23; briefed as 0.5.4, but the armour agent released 0.6.0 first): RYNO homing, Zodiac reach, off-screen picks

## Landed
- openrac-mods master: `fix(ng-plus-plus): steer the RYNO II's homing rockets onto live targets so they never hover`
  + 4cb79bd + `chore(ng-plus-plus): release 0.6.1` (1055b26), tag `ng-plus-plus-v0.6.1`; installed on the Mac (backup ~/RACSwarm/backups/ng-plus-plus-before-0.6.1-20261123-004827).
- OpenRAC develop: PR #767 (squash c5709e01) `feat(rac2): auto-targeting weapons pick enemies off screen for mods`
  (weapon_culling.c table: items 43, 28, 31 drones, 24 seekers, 41 turrets, 45 shield/Tesla Barrier, 18 Tesla Claw;
  drawn bypass + cone opened for the drones, LOS via CollLine flags 18; OPENRAC_PICK_TRACE=1 or item list).

## RYNO cause (homing_l00_shot_update_3, rac2 decomp src/levels/shared/L00_0040A4A8.c)
1. The game turns a rocket only by its target's motion (lead): a still/shuffling target never turns it or snaps it
   back and forth over the aim point -> hovering when the point can't be hit.
2. NG++ picked split targets that are kind-5 enemies but hidden/asleep at spawn points (Grelbin Yetis 4304,
   Leviathans 3927), dying, or with no health: unhittable.
3. The game's spawn gives a rocket made with no target a 4-10 deg/tick pitch wobble (s->yaw): untargeted splits
   looped in place until the 300-tick timer.
4. When a target went, the game reverts to pvars+0x30 (first heading): "projects off elsewhere".

## Fix (ng-plus-plus/src/homing.{h,cpp}, shots.cpp steer_rockets, moby_scan facts)
- Targets: state<0x80, kind 5, health record >0, (mode+0x34 & 0x81)==0, alpha+0x23>=8, ran last tick (moby_run), LOS
  (level_line). Perks::nearest uses targetable() too (all perks' targets).
- Each tick per homing RYNO rocket: turn = max(base, 3*speed/dist) (can't orbit), heading written to moby rows and
  pvars+0x30, offset zeroed; strike (blast, else posted hit) after 3 ticks within reach unhit; drop after 2 walled sight
  checks or 150 ticks; retarget at once to next valid enemy in sight; split wobble zeroed.
- Zodiac (perks.cpp zodiac_reach): at pulse start hits enemies the game's pick left out (not run, >40, cone): reach
  50 + 4/tier (max 90), LOS required, kills <=100 health else 20*30*tier damage, via Perks::post (needs a prior weapon
  hit snapshot); wakes asleep ones (+0x30 = 255 for 90 ticks).

## Tests
- Unit: tests/homing_test.cpp (orbit vs fixed turn, target behind, circling, PAL, dead/hidden targets, chase verdicts).
- Hidden muted runs on box1 (csweep port 84612d4e7, NGPP_TEST_SETUP build, 4000 frames): Grelbin RYNO tier 5: 130 homing rockets, longest 113 ticks, 0 over 2 s; Grelbin tier 10: 801 homing, longest 122, 1 over 2 s; Maktar tier 8: 150 homing, longest 112, 0 over 2 s. Before the wobble fix 26 of 116 and 286 of 1156 splits ran their full 300-tick timer. Yetis (4304) at spawn points were let go as hidden.

## Open / risks
- Zodiac reach and #767's drones/seekers/turrets/claw not seen in a game run (only Tesla Barrier was, by #767's agent).
- Zodiac reach posts nothing without a prior weapon-hit snapshot (Zodiac-only play).
- Waking asleep enemies writes their update distance; restored after 90 ticks.
- #767 hand-added rac2_names.tsv rows for ntsc2/pal: the next release_carry.py run drops them unless pairs facts get them.
- How to test: NGPP_TEST="44:5;hold=44;near=1" on Grelbin (--level 19), grep "test: ryno rocket" (test builds only).

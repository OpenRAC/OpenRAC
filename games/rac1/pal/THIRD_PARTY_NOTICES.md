# Third-party notices

## Lombyte

The following functions adapt source from
[Lombyte](https://github.com/mateuszklysz/Lombyte) for the PAL executable:

- `src/core/00119328.c`: `func_001194C8` (`topThread`)
- `src/core/00119D88.c`: `func_0011C208` (`sceClose`)
- `src/core/00119868.c`: `func_00119CC8` (`sceTtyInit`)
- `src/game/hud.c`: `func_00201190` (HUD sprite with explicit UV corners)
- `src/game/vendor.c`: `func_00239A00` (vendor item carousel)
- `src/game/lights.c`: `func_002027C0` (detach point light)
- `src/core/0011D0D0.c`: `func_0011D248` (`sceSifRebootIop`)
- `src/game/menu.c`: `func_00208338` (read sector, track size)
- `src/game/loaders.c`: `func_00203038` (unpack point records)
- `src/core/00112380.c`: `func_001123A8` (`_calloc_r`)
- `src/core/00119D88.c`: `func_0011B2F8` (`sceSifBindRpc`)
- `src/core/00119D88.c`: `func_0011AE20` (`sceSifInitRpc`)
- `src/core/00121750.c`: `func_00121B78` (`sceGsResetGraph`)
- `src/core/00123168.c`: `func_001233E8` (`sceDmaPutEnv`)
- `src/core/00125630.c`: `func_001273A0` (`_waitBdecOut`)
- `src/core/00125630.c`: `func_001286E8` (`_ipuVdec`)
- `src/core/00125630.c`: `func_001299E8` (`_cpr8`)
- `src/core/0012AC80.c`: `func_0012C0A0` (`_decodeOrSkipField`)
- `src/core/00113B70.c`: `func_00113B70` (`_free_r`)
- `src/game/memcard.c`: `func_0020BBC8` (memcard_PrepData)
- `src/core/0011CCE0.c`: `func_0011CE70` (`_sceSifLoadModuleBuffer`)
- `src/game/loaders.c`: `func_00205220` (ParseSpaceSceneChunk)
- `src/game/draw.c`: `func_001F5368` (screen stripe fill)
- `src/core/00114060.c`: `func_00114060` (`__sfvwrite`)
- `src/game/memcard.c`: `func_0020BD70` (memcard_RestoreData)
- `src/core/0012AC80.c`: `func_0012C990` (`_setDefaultQM`)
- `src/core/00125630.c`: `func_00128F90` (`_pictureCodingExtension`)
- `src/core/989snd.c`: `func_0012DDC0` (snd_FlushSoundCommands)
- `src/game/loaders.c`: `func_00204FC0` (update_world_object_animation)
- `src/core/989snd.c`: `func_0012E688` (snd_SendIOPCommandAndWait)
- `src/core/00125630.c`: `func_00128860` (`_peepBit`)
- `src/core/0011D0D0.c`: `func_0011D0D0` (`sceSifResetIop`)
- `src/core/0012AC80.c`: `func_0012BF40` (`_decodeOrSkipFrame`)
- `src/core/001236F0.c`: `func_00123D48` (sceMcWrite)
- `src/core/boot.c`: `func_0012DA38` (parse_bin)
- `src/game/pause.c`: `func_002224A8` (draws an options menu: each item's label at left, its current value's name at right, selected one highlighted)
- `src/game/framebuf.c`: `func_001FB908` (clears the screen through a GIF packet appended to D_00161000: a fixed header then n = w / 32 pairs of sprite corner registers stepping 0x200 per column across a w x h area centred on 0x8000)
- `src/game/space.c`: `func_002308C8` (spawns six particles around a moby: random velocity and a position taken from the flare corner table, transformed by the moby's matrices)
- `src/core/0012CC90.c`: `func_0012D068` (sceIpuInit)
- `src/game/loaders.c`: `func_002032D0` (load_hud_banks)
- `src/game/pause.c`: `func_0021D4C0` (pause slot-select tick)
- `src/game/draw.c`: `func_001F2608` (builds the view matrices in the camera block from the rotation set up in D_00187390 or the three rotation angles, then copies the result to D_0018D080)
- `src/game/lights.c`: `func_002023E0` (create_point_light)
- `src/game/initonce.c`: `func_00201E88` (init_once)
- `src/game/draw.c`: `func_001F2BC8` (build_occlusion_visibility)
- `src/game/pause.c`: `func_0021E4B0` (draws a pause-menu list (rows of text with optional subtext, cursor highlight, scrolling))
- `src/core/00125630.c`: `func_00127D40` (_decMB0)
- `src/game/vendor.c`: `func_00239CF8` (fun_002389e0)
- `src/core/0012AC80.c`: `func_0012B2C0` (_PES_packet)
- `src/game/camera.c`: `func_001ED080` (camera update toward a target orientation: blends yaw/pitch/distance and rebuilds the camera basis)
- `src/game/space.c`: `func_00233308` (do_space_transition)
- `src/game/help.c`: `func_001FE6C0` (update_help_state)
- `src/overlays/shared/stream_002670E0.c`: `func_L00_002676A0` (stores b in the d field of the slot func_L00_00267618 finds, if any)
- `src/overlays/shared/tieproc_00299108.c`: `func_L00_0029A868` (plays the movie of table entry i (set chosen by a flag))
- `src/overlays/shared/partupd_00272158.c`: `func_L00_00276F18` (FUN_L00_00276078)
- `src/overlays/shared/hud_00235960.c`: `func_L00_00238478` (FUN_L00_00237ae8)
- `src/overlays/shared/help_0020CDF0.c`: `func_L00_00212D70` (builds the two camera-relative vectors a and b from offsets fa and fb, rotating about the up axis when mode is set)
- `src/overlays/shared/vendor_002D9438.c`: `func_L00_002DD2D0` (frees a moby's state slot and returns 1 when it is in state 4)
- `src/overlays/shared/vendor_002BA7C8.c`: `func_L00_002BFF88` (probe below a point and return the ground height offset, or zero when nothing is hit)
- `src/overlays/shared/vendor_002D9438.c`: `func_L00_002DCFD0` (place a moby at the given position under its owner, reset its owner's state to 4 and start the matching animation)
- `src/overlays/shared/vendor_002E1660.c`: `func_L00_002E5630` (computes two adjusted points from a moby's lookup entry and its direction vector; returns 1 on success)
- `src/overlays/shared/help_0020CDF0.c`: `func_L00_0020DF90` (finds the listed object nearest a point)
- `src/overlays/shared/partupd_0026A130.c`: `func_L00_0026A7F8` (spawn a particle of type 4 with position, velocity, color, lifetime and flag)
- `src/overlays/shared/vendor_002AB910.c`: `func_L00_002ADD78` (updates a decaying moby: spins down its position fields and fades it out)
- `src/overlays/shared/vendor_002C12B0.c`: `func_L00_002C6608` (spawns a moby of a class and initialises its data block)
- `src/overlays/shared/mobyutil_00261B00.c`: `func_L00_00264570` (spawns impact sparks at a point on the ground near the camera)
- `src/overlays/shared/mobyutil_00258BC8.c`: `func_L00_0025B040` (records a shadow-like entry for the moby at its ground height, scaled by arg)
- `src/overlays/l01_novalis/vendor_002FABE8.c`: `func_L01_0030D248` (spawn a debris moby on src, scaled, moving along dir with random spin and lifetime)
- `src/overlays/shared/hud_00235960.c`: `func_L00_0023B750` (shows or hides the weapon HUD element depending on the current weapon)
- `src/overlays/shared/mobyutil_00258BC8.c`: `func_L00_0025AAC0` (records a timed effect entry for a moby unless a fresher one of its own is still live)
- `src/overlays/shared/vendor_002BA7C8.c`: `func_L00_002C0BB8` (updates a homing state block o from target m: turn rate, heading, and a speed clamped to seven times a global)
- `src/overlays/l01_novalis/vendor_002FABE8.c`: `func_L01_002FB440` (spawns a beam moby (0x2D2) from owner between two points, colored, with random rotation and two glow sprites)
- `src/overlays/shared/mobyutil_00258BC8.c`: `func_L00_0025BA50` (sends a message to every moby in a list except one, each carrying the angle from pos to it and a velocity along that angle)
- `src/overlays/shared/vendor_002C96D0.c`: `func_L00_002C96D0` (places a moby at a spot: copies the vector in, transforms it, and if the move is blocked pushes it back out along the stored normal)
- `src/overlays/shared/space_0028FB78.c`: `func_L00_00290030` (start a ship-travel transition to the given destination, setting up the flight path and timing)
- `src/overlays/shared/vendor_002B33E8.c`: `func_L00_002B6E90` (steers the held object's pose: eases a field toward 0.45, spins it, and rebuilds its matrix and position from the parent)
- `src/overlays/shared/mobyutil_00258BC8.c`: `func_L00_0025CCF0` (spring a wrapped angle *P (rate *V) toward target X; returns the remaining angle difference)
- `src/overlays/shared/vendor_002AB910.c`: `func_L00_002AE110` (spawns a type-0x79 moby with random spin axes and speeds, owned by the given object, at pos)
- `src/overlays/shared/partupd_0026A130.c`: `func_L00_0026D3F8` (particle type 18 update: bounces its alpha ramp and fades, killing when spent)
- `src/overlays/shared/partupd_00272158.c`: `func_L00_002767B0` (spawns a type 78 particle: position, colour, lifetime and mode, with a trailing record of speeds, id and target)
- `src/overlays/l01_novalis/vendor_002BA898.c`: `func_L01_002F7C78` (spawns a beam moby from an owner, position, direction and colour vectors)
- `src/overlays/l01_novalis/vendor_002FABE8.c`: `func_L01_002FAFC8` (oscillating spinner: swings its angle between -1 and 1 and turns the moby to match)
- `src/overlays/shared/update_0029B6A0.c`: `func_L00_0029BED8` (leaves the vendor screen: releases two handles, resets state, repositions the hero and restores the game state)
- `src/overlays/shared/vendor_002AB910.c`: `func_L00_002B0B98` (spawns a particle moby of the given type owned by owner, with random spin and size)
- `src/overlays/shared/partupd_00272158.c`: `func_L00_00272158` (spawns a type 0x2C particle with position, velocity, colour and scale parameters)
- `src/overlays/shared/partupd_00272158.c`: `func_L00_002722F0` (per-frame update of a type 0x2C particle: fade colour, move and rotate its velocity)
- `src/overlays/shared/partupd_0026A130.c`: `func_L00_0026FA28` (part type 31 update: eases two trailing points toward its parent's position and dies when they converge)
- `src/overlays/shared/partupd_00272158.c`: `func_L00_002732D8` (part type 54 update: advances a particle, fades its color near the end of life and spawns a light effect at its position)
- `src/overlays/shared/vendor_002D9438.c`: `func_L00_002D9438` (update a trailing moby: advance its timer, delete it when done or its owner died, else follow the owner)
- `src/overlays/l01_novalis/vendor_002FABE8.c`: `func_L01_00300F00` (smoke emitter: keeps its sound alive and spawns two puffs of particles per frame)
- `src/overlays/shared/partupd_00272158.c`: `func_L00_00273F80` (spawns an effect record at a position inside the level bounds;)
- `src/overlays/shared/pause_00277208.c`: `func_L00_00286128` (seeds the pause-menu snapshot: copies the header, marks valid entries in the bitmasks, fills the state record)
- `src/overlays/shared/shrubproc_0028A198.c`: `func_L00_0028A3E0` (steps each shrub sparkle entry: type 0 spins and scales a direction, type 1 randomises a colour)
- `src/overlays/shared/camera_001EB508.c`: `func_L00_001EDA28` (moves the camera position a toward a target in steps, stopping each step on collision)
- `src/overlays/shared/mobyutil_00261B00.c`: `func_L00_00264BE8` (bursts two rings of sparks (and then three more) from two points with the given velocity)
- `src/overlays/shared/help_00203E98.c`: `func_L00_00209EC0` (every lim calls of the counter, sprays a dust particle at the player's position with a random velocity and resets the counter)
- `src/overlays/shared/vendor_002E1660.c`: `func_L00_002E1C38` (scrolls the four hue values of a moby's effect and spawns a coloured streak for each)
- `src/overlays/shared/help_00203E98.c`: `func_L00_00209CB0` (emits a pair of sparks from the player's position each time the counter passes its limit)
- `src/overlays/shared/partupd_0026A130.c`: `func_L00_00271F40` (updates a fading colour particle: interpolates its packed colour and alpha over its life and drifts it upward)
- `src/overlays/shared/vendor_002D1168.c`: `func_L00_002D6978` (walks a moby toward a target point: turn to face it, ease its speed, step it along its heading; returns the distance (or 37.0 when a flag is set))
- `src/overlays/shared/vendor_002E1660.c`: `func_L00_002E4F38` (path-following moby update: start a run along its path in state 0, then step along the path each frame and pick an animation when the flag is set)
- `src/overlays/shared/partupd_0026A130.c`: `func_L00_0026DB50` (per-frame update of a particle that drifts by its velocity and fades; sparks off a child particle at random)
- `src/overlays/shared/vendor_002BA7C8.c`: `func_L00_002BFB98` (draws a textured quad twice (first tinted, then white) from a moby's orientation)
- `src/overlays/shared/vendor_002C12B0.c`: `func_L00_002C8680` (aims a point near p toward q (at most range away), then fills out with a heading vector of length 8.5 * D_0015EE6C and a clamped height)
- `src/overlays/shared/mobyutil_00261B00.c`: `func_L00_00263950` (eases a moby's per-axis offsets toward zero and applies them and its scale to the held object, resetting when settled)
- `src/overlays/shared/mobyutil_00261B00.c`: `func_L00_00263680` (pushes the source object's transforms into an 8-entry ring and replays them into the trailing objects, ending them when all faded)
- `src/overlays/shared/shrubproc_0028A198.c`: `func_L00_0028A198` (fills the sky particle list: entries from a up are dim specks, the first a are coloured sparks with random direction and speed)
- `src/overlays/l01_novalis/vendor_002BA898.c`: `func_L01_002FA998` (elevator update: waits, then moves the lift between its two stops and carries whatever rides it)
- `src/overlays/shared/vendor_002D1168.c`: `func_L00_002D8180` (draws four drifting, colour-cycling sprites around a moby, stepping each one's phase and fade timer)
- `src/overlays/shared/help_00221A98.c`: `func_L00_00222668` (picks the nearest eligible moby of class 0x323 near the player and stores it as the target)
- `src/overlays/l01_novalis/vendor_002BA898.c`: `func_L01_002E78F0` (spawns three bursts of sparks around a moby with random velocity, colours and size)
- `src/overlays/shared/vendor_0029FD68.c`: `func_L00_002A0F60` (draws a screen-space rectangle from five joints of a moby: builds a box from its joint vectors, projects it and draws the rectangle)
- `src/overlays/shared/vendor_002C12B0.c`: `func_L00_002C53A8` (steers a moby toward the player's follow distance, easing its height)
- `src/overlays/shared/mobyutil_00258BC8.c`: `func_L00_0025ED30` (nearest point on the segment a-b to p (clamped), returned with the distance result)
- `src/overlays/shared/vendor_002A5138.c`: `func_L00_002AB2A8` (initialises a spawned moby: random spin vector, nearby-count cull of the oldest same-class moby, then a bounce off the floor probe)
- `src/overlays/shared/help_002297E0.c`: `func_L00_00229E30` (tries the crouch-jump style moves in priority order and starts the first whose conditions hold)
- `src/overlays/shared/vendor_002BA7C8.c`: `func_L00_002BC3B8` (builds and draws two layered glow quads around a moby)
- `src/overlays/shared/help_0020CDF0.c`: `func_L00_0020E100` (picks the hero's current animation id and speed from its state and starts or updates the animation slot)
- `src/overlays/l01_novalis/vendor_002FABE8.c`: `func_L01_00300140` (sliding door update: opens when the player is near, closes when far, and offsets the door along its heading by the open amount)
- `src/overlays/shared/help_00214D60.c`: `func_L00_00216880` (builds the camera rotation matrix m that looks along v, with pitch smoothing)
- `src/overlays/shared/partupd_00272158.c`: `func_L00_002742E8` (updates a fading, pulsing particle: animates its phase, colour and scale, then orients it)
- `src/overlays/shared/partupd_00272158.c`: `func_L00_00274D80` (particle type 65 update: fall under gravity, bounce off the world, fade the colour and die when spent)
- `src/overlays/shared/help_0020CDF0.c`: `func_L00_00211A38` (updates the hero camera's follow-speed targets by mode, then eases them)
- `src/overlays/shared/vendor_002AB910.c`: `func_L00_002B08A0` (per-frame update of a falling spark moby: emits a burst particle, moves, fades and dies when out of bounds)
- `src/overlays/shared/vendor_002B33E8.c`: `func_L00_002B5998` (finds the best-aimed target moby in the global list for a given yaw and pitch, refining them)
- `src/overlays/shared/vendor_002C96D0.c`: `func_L00_002CE390` (spawns a type-0x1C9 projectile moby at a position, aimed at an optional target; returns it, or 0 if it was launched at once)
- `src/overlays/shared/help_00203E98.c`: `func_L00_0020A540` (per-frame spark effect: emits a burst of particles around a saved position while a global timer runs down)
- `src/overlays/shared/help_0020CDF0.c`: `func_L00_00211F80` (turns the stick vector into a target heading and move direction for the player (mode 1 snaps axes, 2 and 3 snap to 4 or 8 directions, 4 uses the player's own heading))
- `src/overlays/l00_veldin1/initonce_0023E300.c`: `func_L00_0023EC00` (per-frame Veldin hook: queues callbacks and spawns particles by game state)
- `src/overlays/shared/vendor_002E1660.c`: `func_L00_002EA068` (steers the hero heading and pitch towards a stick input)
- `src/overlays/shared/help_00214D60.c`: `func_L00_002162B8` (sets the camera lean/offset terms from the pad input for the glide, spin and hover states)
- `src/overlays/shared/vendor_002C12B0.c`: `func_L00_002C7128` (draws a streak of quads along the vector from the moby's joint to its target, swelling in the middle)
- `src/overlays/shared/mobyutil_00258BC8.c`: `func_L00_002594C8` (moves pos toward to while keeping it over the ground and out of walls; returns 0 if it was snapped back)
- `src/overlays/shared/partupd_0026A130.c`: `func_L00_0026E5A0` (per-frame update of a fading, bouncing spark particle: moves it, bounces off geometry, dims its colour and kills it when spent)
- `src/overlays/shared/help_0020CDF0.c`: `func_L00_002129D0` (hero ground probe: measures the support under the feet, and slides the velocity off a steep floor)
- `src/overlays/shared/partupd_00272158.c`: `func_L00_00273A60` (part type 58 update: fade in, hold spawning sparks, then fall and fade out)
- `src/overlays/l01_novalis/vendor_002BA898.c`: `func_L01_002FA030` (emitter update: ticks timers, occasionally bursts a smoke puff and spawns two drifting smoke particles per frame)
- `src/overlays/shared/shrubproc_0028A198.c`: `func_L00_0028A878` (initialises and animates the star field of the sky)
- `src/overlays/l00_veldin1/vendor_002DB278.c`: `func_L00_002E2F58` (spawns a spark: steps the moby's position and emits one fast particle and three slow tails)
- `src/overlays/l01_novalis/vendor_002BA898.c`: `func_L01_002FA458` (collapsing platform: sparkles while idle, then drops when the player stands near, and records it as collected)
- `src/overlays/l01_novalis/vendor_002FABE8.c`: `func_L01_002FB898` (per-frame update of a falling drop: integrates its velocity, tests it against the world and spawns its impact effect)
- `src/overlays/shared/help_002297E0.c`: `func_L00_00229B78` (menu/hint update: picks an action for the player from the game block's timers and flags)
- `src/overlays/shared/tieproc_00299108.c`: `func_L00_0029A8D0` (looks up table entry i, from one of two sets chosen by a flag, and passes it on)
- `src/overlays/shared/vendor_002C12B0.c`: `func_L00_002C6F48` (per-frame step of a counter moby: advances, plays sound and finishes)
- `src/overlays/shared/vendor_002C96D0.c`: `func_L00_002CD3B8` (spawns a debris moby with randomized spin and velocity)
- `src/overlays/shared/mobyutil_00261B00.c`: `func_L00_00264EA8` (spawns a burst of effects on a moby: a fixed set at its position, then randomly offset ones around it)
- `src/overlays/shared/help_0020CDF0.c`: `func_L00_0020DD48` (scores how far a moby is from the camera/hero reference point and flags it when out of range)
- `src/overlays/shared/vendor_002AB910.c`: `func_L00_002B0F58` (spawns an effect moby tied to an owner and a target and aims its starting state)
- `src/overlays/shared/vendor_002EB0D8.c`: `func_L00_002EB3A0` (clears a moby's state block and calls its reset helper)
- `src/overlays/shared/vendor_002EB0D8.c`: `func_L00_002ECAF8` (steps a follow-camera object toward a target moby, easing its angle and position)
- `src/overlays/l00_veldin1/vendor_002DB278.c`: `func_L00_002E2038` (Veldin vendor update: walks a path, turns toward the player, attacks and self-destructs)
- `src/overlays/l01_novalis/vendor_002FABE8.c`: `func_L01_002FED78` (breakable pot: waits for its hit flag, then shatters into four sparks)
- `src/overlays/l01_novalis/vendor_002FABE8.c`: `func_L01_002FCC80` (hinged bridge: swings with a sound while triggered and poses the bridge from its pivot)
- `src/overlays/shared/mobyutil_00258BC8.c`: `func_L00_0025A868` (tests whether a moby's linked object has a non-zero field at +8)
- `src/overlays/shared/vendor_002E1660.c`: `func_L00_002E99E4` (stores a float into a moby field at +0xB4)
- `src/overlays/shared/vendor_002D9438.c`: `func_L00_002DCD40` (calls the callback a moby's class defines at +0x10, unless the moby is deleted)
- `src/overlays/shared/mobyutil_00258BC8.c`: `func_L00_00261848` (marks a planet as discovered and appends it to the galaxy list)
- `src/overlays/shared/mobyutil_00258BC8.c`: `func_L00_0025BBA0` (splits a vector into an angle and two scales)
- `src/overlays/shared/vendor_002EB0D8.c`: `func_L00_002EC860` (sets a moby's damping and scale constants, then re-runs its setup)
- `src/overlays/shared/help_00203E98.c`: `func_L00_0020A8B8` (tests a segment against the world and returns the hit distance)

Data taken from Lombyte:

- `tools/extract/moby_classes.tsv`: the moby class names the level editor
  shows, from its `config/overlays/us/names/level-NN.json` ("moby-class-record"
  evidence), which joins each level's class dispatch table with the class
  names of [Wrench](https://github.com/chaoticgd/wrench)'s moby class unpack.

MIT License

Copyright (c) 2026 Mateusz Kłysz

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## ReRAC

[ReRAC](https://github.com/re-rac/rerac), a native PC port of the US build,
documents the game's formats and systems. The following adapt its format
code and notes, or quote it:

- `tools/extract/mobys.py`: the moby instance record and what the level
  loader does with each field, from `crates/rc-formats/src/gameplay.rs` and
  `docs/plan/moby_render_notes.md`; described in `docs/ASSETS.md` ("Mobys").
- `tools/extract/moby_class.py`: the moby class header and model format
  (packets, vertex cache, skinning slots, normals, untextured faces), from
  `docs/formats/moby_rac1.md` §1-§2 and `crates/rc-formats/src/moby.rs`;
  described in `docs/ASSETS.md` ("Moby classes").
- `tools/extract/moby_anim.py`: moby skeletons and animation sequences,
  from `docs/formats/moby_rac1.md` §3-§4, `docs/plan/moby_animation.md` and
  `crates/rc-formats/src/moby_anim.rs`.
- `tools/extract/collision.py`: the collision block (cell tree, packed
  vertices, faces and surface bytes), from `docs/formats/collision_rac1.md`,
  `docs/plan/collision_queries.md` and `crates/rc-formats/src/collision.rs`;
  described in `docs/ASSETS.md` ("Collision").
- `config/overlays/rerac_notes.tsv`: names and notes quoted from its
  `tools/ghidra/names/doc_names.csv` (the commit is in the file's header),
  shown in worker packets.

ISC License

Copyright (c) 2026 ReRAC contributors

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

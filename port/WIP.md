# Work in progress: ReRAC converted to C++

Paused on 2026-10-09. These parts of the port are being converted from
[ReRAC](https://github.com/re-rac/rerac) (ISC License, Copyright (c) 2026
ReRAC contributors) into OpenRAC's C++, re-organised for all four games. They
were written by six parallel agents that were stopped part-way: **nothing
here is reviewed, built or tested yet**, and the build leaves it out unless
`-DOPENRAC_PORT_WIP=ON`. [WIP_BRIEF.md](WIP_BRIEF.md) is the brief every
agent worked to (rules, layout, legal constraints).

| Part | Directory | Converted from | Where it stopped |
|---|---|---|---|
| Disc and extractor | `assets/disc/`, `tests/disc/`, `extract/` (empty) | rc-formats iso9660, toc, disc, wad, level, overlays, volumes, sha1, strings, scenes, saves; rc-extract; rc-data | writing the disc library; the C++ extractor not started |
| Geometry | `assets/geometry/` | rc-formats texture, vif, tfrag, tie, shrub, sky, moby, moby_anim, lighting | starting the VIF parser |
| World data | `assets/world/`, `tests/assets_world/` | rc-formats collision, occlusion, cameras, gameplay, moby_spawn, water, sea, font, hud, pif | at volumes |
| Sound, movies, audio | `assets/sound/`, `media/`, `audio/`, `tests/sound/`, `tests/media/` | rc-formats sound_bank, vag, pss; rc-video (MPEG-2); rc-game audio (SPU-style mixer, 989snd player, reverb) | writing assets/sound |
| World renderers | `renderer/world/`, `renderer/shaders/world/` | rc-engine renderers and its 23 WGSL shaders, to GLSL 4.1 | writing the draw-data definitions |
| Engine docs | `../docs/engine/formats/`, `../docs/engine/systems/` | ReRAC docs/formats and docs/plan | part-way; docs/engine/README.md not updated |

To continue: finish each part against WIP_BRIEF.md, build it with
`-DOPENRAC_PORT_WIP=ON`, review it (legal: no game bytes, no personal names),
test it, then remove it from the guard. After that: bridge assets/geometry to
the renderers' draw data, drive the viewer from the disc, wire
game/common/lib/snd.c to openrac_audio, switch the launcher to the C++
extractor, credit ReRAC in THIRD_PARTY_NOTICES.md for each finished part.

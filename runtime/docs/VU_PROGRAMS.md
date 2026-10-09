# The games' vector unit programs, by name

How the runtime can tell which of a game's vector unit programs is loaded,
and which programs the games share. It matters for the renderer: a program
that is recognised can be drawn by host code written for it instead of being
interpreted ([DESIGN.md](DESIGN.md), section 4.4).

The convention was first written down for the US disc of the first game by
[ReRAC](https://github.com/re-rac/rerac) (`docs/formats/vu_microprograms.md`,
ISC) and measured for Going Commando by rac2-decomp
(`games/rac2/ntsc/docs/VU-MICROPROGRAMS.md`). The numbers below were read
again from our own disc images on 2026-10-08, with a section reader of a few
lines; no program bytes are here.

## Where the programs are

The boot executable carries its vector unit code in one section, `.vutext`,
at the start of the program (EE address 0x00100080). A table section,
`.DVP.ovlytab`, has one 12-byte record for each 0x800-byte chunk of a
program:

    { u32 name_offset; u32 ee_address; u32 vu_address; }

`name_offset` points into `.DVP.ovlystrtab` at a name of the form

    .DVP.overlay..<vu_address>.<program_id>.<line>.<chunk>

so the chunks of one program share a `program_id`, and a program is loaded by
copying its chunks to consecutive addresses of a unit's program memory. The
section table is present in the retail executables of all three discs below.

## The programs

| Program | Unit | Role (ReRAC, from the first game's code) | RAC1 PAL and US | Going Commando (US) |
|---:|---|---|---|---|
| 55907 | VU1 | terrain (tfrag), main | 8 chunks | 8, all the same as RAC1 |
| 903379 | VU1 | terrain, second strip list | 4 | 4, 3 the same |
| 56467 | VU1 | shrubs, first list | 2 | 2, both the same |
| 912339 | VU1 | shrubs, second list | 3 | 3, all the same |
| 13507 | VU1 | ties | 4 | 5, none the same |
| 224979 | VU1 | ties, second program | 7 | 8, none the same |
| 13859 | VU1 | mobys | 6 | 6, none the same |
| 57843 | VU1 | textured sprites | 3 | 3, 2 the same |
| 221571 | VU1 | particles | 1 | 1, not the same |
| 104691 | VU0 | helper for mobys | 2 | 2, 1 the same |
| 436083 | VU0 | end of frame, transitions | 1 | 1, not the same |
| 28259 | VU0 | patch loaded once (sine and others) | 2 | 2, both the same |
| 56883 | VU1 | unknown | not there | 5 |

The PAL and US discs of the first game have the same 12 programs, chunk for
chunk. Going Commando has all 12 and one more; 21 of its 49 comparable chunks
are the first game's unchanged. rac2-decomp found its vector unit code
identical from the earliest prototype disc to the shipped game.

The roles are ReRAC's reading of the first game's code. They are a lead: each
is to be confirmed here by which draw path a program is running under before
host code is written for it.

## What follows for the runtime

1. **Naming a loaded program.** The runtime already keeps one compiled image
   for each distinct content of a unit's program memory. With the table above
   read at boot, an image can be given its `program_id` by comparing its
   chunks with the executable's, with no table per game. Reports and
   profiles can then say "terrain" instead of a hash.
2. **Host renderers start with terrain and shrubs.** Their programs are the
   same in both games, so one host renderer for each serves both. Ties and
   mobys were edited between the games and need a look at what changed.
3. **The `<line>` field** is the line of the program's source it started at:
   the same in both games for unchanged programs, shifted for the edited
   ones. It tells which programs to compare first when a later game is
   added.
4. Level programs may carry vector unit code of their own in their data; this
   has not been checked for any game.

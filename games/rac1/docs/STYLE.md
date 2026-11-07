# Code style

Short rules for C in `src/` and `include/`. If a rule would change the compiled bytes, the bytes win. Leave a
short comment that says why.

## Names

| What | Style | Example |
|---|---|---|
| Function | `snake_case`, system first | `hero_set_move_state` |
| Copy of a function in one level | system, then level | `hero_l17_set_move_state` |
| Global | `g_` + `snake_case` | `g_class_slot` |
| File-local data | `s_` + `snake_case` | `s_fade_timer` |
| Type | `PascalCase` | `HeroMotion` |
| Moby private data | class name + `Vars` | `ScreamerVars` |
| Enum value, macro | `UPPER_CASE`, with a shared prefix | `MOBY_CLASS_SCREAMER` |
| Unknown field | offset | `unk_24` |
| Unknown function, data | address | `func_001F4E08`, `D_0013F450` |

- Readable names are macros over address names. Add them to `config/names_local.tsv`, not to `include/names.h`
  ([NAMES.md](NAMES.md)).
- Never rename original names: `recovered` names in `config/names.tsv`, and library names (`memcpy`, `sceGsSync`).
- A name says what the thing is. No numbers from the disc: not `Moby388`, `Class908Vars_FD578`.

## Files

Keep this order in a `.c` file:

1. First comment: what the file holds. Overlay files also give their start address.
2. Includes
3. Local types and macros
4. Data
5. Static functions, when the link order allows it
6. Functions, in link order

Link order comes first, because retail fixed it. Do not reorder functions to make a file look nicer.

## Headers

- Put a type or prototype in `include/`, not in a `.c` file, when two files use it.
- Guards are `#ifndef NAME_H`.
- Keep the include order. `include/names.h` macros depend on it.

## Numbers

- Use an enum or a named macro when the meaning is known: `MOBY_CLASS_SCREAMER`, not `0x3EF`.
- Use a struct field, not `*(int *)(p + 0x24)`.
- If the type is still unknown, use `STRUCT_OFFSET(p, 0x24, int)`. It is easy to find and replace later.

## Switches

- Mark a case that runs into the next one with `/* fall through */`.
- Use one number base for the cases of a switch.
- Keep the order of the cases: it gives retail's jump table.
- Keep empty cases, such as `case 2: break;`. They record the values the game knows about, and removing them can change the
  jump table. Tidy code is not worth losing what the binary shows.

## Comments

Every function gets one or two short sentences: what it does, and why the C has this shape if that is not clear.

```c
/* Spawns a Screamer at the hero's position. */
```

Do:

- Use names: "Spawns a Screamer".
- Say what a number means when it has no name yet: `/* 0x1C: frames left */`.

Do not:

- Use IDs when a name is known: `/* Spawns Moby_564 */`, `/* class 0x3EF */`.
- Use `func_`, `D_` or `+0x` when a name or field exists.
- Leave worker notes: "matches now", "regalloc", "functional: high".
- Repeat what the code says.

**Provenance notes are licence credit.** Never delete or reword them, even in a tidy pass. They look like
`Adapted from Lombyte's project (GPL-3.0) for PAL: <file>, <name>.`

## Markers

| Marker | Meaning |
|---|---|
| `/* FAKEMATCH: <reason> */` | The code matches, but it is not what the authors wrote. Tracked in `config/fakematch.tsv`. |
| `/* KEPT: <reason> */` | Looks like a hack, but someone read it and has a reason it stays. |
| `INCLUDE_ASM` | Not matched in C yet. |
| `ASM_FUNC` | Original assembly. It stays assembly. |
| `LINKER_REMNANT` | Linker leftovers. They stay assembly. |
| `PREBUILT_ASM` | Code from Sony's prebuilt libraries, not built from source. It stays assembly. |
| `STRUCT_OFFSET` | A raw offset whose field is not known yet. |

## Format

4 spaces, K&R braces, 120 columns. No tabs and no trailing spaces.

## Allowed hacks

Pure C only. These exceptions are the only ones:

- `qcopy` / `qzero`, `Cpu*` helpers, VU0 and handwritten assembly
- `SDATA()`
- `do { } while (0)`, no-op expressions and dead branches, each with a `FAKEMATCH` comment

No new `__asm__` aliases, dead initialisers or `register` pins.

More detail: [kb/conventions.md](kb/conventions.md).

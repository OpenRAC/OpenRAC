# Fourth RAC2 C lot - 2026-10-02

Sixty further bodies are integrated in the boot, taking the integrated C from
**1076 to 1752 bytes** (26 to 86 functions), and they are placed in the level overlays as
well. `check_candidates.py` reports **86/86 matched, zero different bytes** against the
pinned boot.

The lot was pushed in two measured steps, and the tables below keep them apart: **49 bodies,
512 bytes** (`ee319a1`), then **11 bodies, 164 bytes** (`12a59e5`).

## Where they come from

Unlike the second and third lots, nothing here is ported from another game. These are
**RAC2's own bytes**: a boot function whose reviewed C compiles to the exact bytes that
*open* a function in the level overlays. Writing it once pays twice - once in the boot, and
once per overlay that carries the same opening - which is what the placement table counts.

The measurement that selects a target is the same one `docs/LEVEL-INTEGRATION-PLAN.md`
states, applied in the other direction: the body must be self-contained (no call, no
address materialisation, no `$gp` access), so that the reviewed C is admissible in the boot
catalogue and in every level placement at once.

## What the gate required, measured

- **The splat boundary equals the compiler extent for all sixty** (measured against the
  pinned disassembly: zero of sixty has a longer boundary). The third lot's four-byte
  trailing `nop` does not occur here: these bodies are eight to twenty-four bytes and do not
  end on a store in a return delay slot. Nothing had to be trimmed or padded.
- **None of the sixty makes a call or reads `$gp`** (zero `jal`/`jalr`, zero `($gp)`), so
  nothing had to be re-anchored to a RAC2 call or data target.
- **One body materialises an address**: `FUN_002B0D60`, twelve bytes, loads a `D_` symbol
  through `lui`. That single body is also the single one the level catalogue does not place,
  which is the level link's own rule rather than a choice: the level catalogue is built with
  an empty external map (`scripts/integration.py`), because the reviewed level bodies are
  self-contained. It counts once, in the boot.

## The forty-nine bodies of the first step

| Symbol (RAC2) | Reviewed bytes | Level placements |
| --- | ---: | ---: |
| `FUN_00282C48` | 8 | 26 |
| `FUN_00283CE0` | 16 | 26 |
| `FUN_002A7790` | 8 | 26 |
| `FUN_00335E10` | 8 | 26 |
| `FUN_00335E18` | 8 | 26 |
| `FUN_00335E20` | 8 | 26 |
| `FUN_00336318` | 8 | 26 |
| `FUN_003367B8` | 8 | 26 |
| `FUN_00336950` | 24 | 26 |
| `FUN_00336CC0` | 8 | 26 |
| `FUN_00339790` | 8 | 26 |
| `FUN_0033A9C8` | 8 | 26 |
| `FUN_0033A9D0` | 16 | 26 |
| `FUN_0033A9E0` | 24 | 26 |
| `FUN_0033A9F8` | 8 | 26 |
| `FUN_0033AE18` | 8 | 26 |
| `FUN_0033AE20` | 8 | 26 |
| `FUN_0033B0A8` | 8 | 26 |
| `FUN_00341540` | 16 | 26 |
| `FUN_00341550` | 8 | 26 |
| `FUN_00341CD8` | 8 | 26 |
| `FUN_003423A8` | 8 | 26 |
| `FUN_003423B0` | 24 | 26 |
| `FUN_00343038` | 8 | 26 |
| `FUN_00343058` | 8 | 26 |
| `FUN_00343060` | 8 | 26 |
| `FUN_003435A0` | 8 | 26 |
| `FUN_003435A8` | 8 | 26 |
| `FUN_00348118` | 8 | 26 |
| `FUN_00348120` | 8 | 26 |
| `FUN_00348128` | 8 | 26 |
| `FUN_00348130` | 8 | 26 |
| `FUN_00348570` | 8 | 26 |
| `FUN_00348578` | 8 | 26 |
| `FUN_00348630` | 16 | 26 |
| `FUN_00349490` | 8 | 26 |
| `FUN_00349590` | 8 | 26 |
| `FUN_003495C0` | 8 | 26 |
| `FUN_00349678` | 8 | 26 |
| `FUN_00349A60` | 16 | 26 |
| `FUN_00349AA8` | 16 | 26 |
| `FUN_00349B18` | 8 | 26 |
| `FUN_0034A4A8` | 8 | 26 |
| `FUN_0034CE98` | 8 | 26 |
| `FUN_0034FAE8` | 16 | 26 |
| `FUN_00350698` | 16 | 26 |
| `FUN_00351888` | 8 | 26 |
| `FUN_003518D8` | 8 | 26 |
| `FUN_00351E48` | 16 | 26 |

## The eleven bodies of the second step

| Symbol (RAC2) | Reviewed bytes | Level placements |
| --- | ---: | ---: |
| `FUN_002B0D60` | 12 | 0 |
| `FUN_003363C0` | 12 | 26 |
| `FUN_00336498` | 20 | 26 |
| `FUN_003364B0` | 20 | 26 |
| `FUN_00343028` | 12 | 26 |
| `FUN_003435B0` | 20 | 26 |
| `FUN_003480D8` | 12 | 26 |
| `FUN_003495C8` | 12 | 26 |
| `FUN_0034AFE8` | 20 | 26 |
| `FUN_003518E0` | 12 | 26 |
| `FUN_00351F28` | 12 | 26 |

## Levels

- **1534 new placements**, taking the level catalogue from 401 to **1935 placements,
  41,500 bytes**;
- fifty-nine of the sixty are placed in all **26 non-tutorial overlays**; the tutorial keeps
  its fifteen earlier bodies and is built apart, as `docs/LEVEL-INTEGRATION-PLAN.md` states;
- **the placed bytes are the boot's bytes**: for every placed symbol, the level proof
  records the same reference hash as the boot proof (measured over all placed symbols, zero
  differences). The placement is a proof of identity, not a similarity.

## Refusals and open ground, stated plainly

- The **VU/MMI reservoir stays closed** to plain C: the candidate gate refuses `asm`,
  `__asm__` and byte/word directives, and ordinary C never emits the VU macros. It is not
  needed for the current goal and it is not attempted here.
- **`$gp`-relative code is out of reach** under the required `-G0`: the retail image places
  its small globals near `$gp`, this link does not.
- Bodies that cite a global are admitted in the boot only; the level catalogue's empty
  external map excludes them by construction, as `FUN_002B0D60` shows.

## How the proofs were regenerated

The proofs in this commit were rebuilt in one pass with a fixed compile invocation: the SN
compiler writes the source spelling it is given into `.file`, so the candidate checker and
the integration used to hash two different objects for one source, and no third party could
reproduce either hash. Both compile sites now run the compiler from the source's own
directory under its bare name, so every proof in this repository carries one object hash.
No reviewed byte changed: the bodies, their sizes and the gates are identical to the ones
measured on 2026-10-02.

## Scope

1752 boot bytes plus 41,500 level placement bytes are **43,252 counted C bytes** of
48,788,176 executable bytes, about **0.0887 %**.

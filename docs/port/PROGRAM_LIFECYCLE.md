# Rac3 program-entry identity

The boot executable and frontbin are different programs even when a release gives their front-end
entries the same address. Native overlay selection must use frontbin's program-record entry, not
the boot front-end function's address mapping.

| Release | Boot front-end function | Frontbin record entry |
|---|---|---|
| PAL | `0x00381250` | `0x00381250` |
| NTSC-U | `0x00381280` | `0x0037D200` |

The entries are measured from the owned program records and agree with the pinned decomp's
per-program map. Each release's hand-written `host/rac3_version.h` supplies `RAC3_FRONTBIN_ENTRY`.
This is program identity, not an override of `RAC3_ADDR` or of the real boot function's pair.

## Handoff

`host/program.c:func_0013AFF8` keeps the existing ordering:

1. Configure SP relocation/range once and capture the caller's current level.
2. Keep a level's source records before the game's loader replaces memory.
3. Call the game's record loader, then increment the program-load count; keep its entry unchanged.
4. Select overlay 100 for this release's frontbin entry, otherwise the captured level or the
   existing executable fallback.

Hostgen registers frontbin under overlay 100. The runtime's function lookup consults the game's
overlay callback before looking up that table and then the executable table. Misclassifying NTSC-U
frontbin as the executable therefore selects the wrong function namespace; the two program
entries cannot be repaired by forcing the boot function's address pair to frontbin's address.

LoadExec, argument copying, executable function re-registration, MP's separate loader/classifier,
and warm-overlay restoration are unchanged. This correction does not reset pad state or rerun an
initializer; NTSC-U retains its boot-initialized core pads across SP overlays
([pad lifecycle](NATIVE_PADS.md)).

## Tests and limits

`rac3_program` and `rac3_program_ntsc` compile the real production wrapper with synthetic records
and a mocked loader/runtime boundary. They cover release entry distinctions, pre-load capture
ordering, retained core state, level/frontbin returns, counters, explicit context reset/restore,
and the existing unsupported-entry fallback. The NTSC fixture is carried with `release_host.py`.

These tests do not run translated game code, the actual runtime registry, LoadExec or the real
record loader. Whole-game generation/linking and coordinator-owned boot/frontbin/level/MP runs
remain required; the unit tests are not runtime-readiness evidence.

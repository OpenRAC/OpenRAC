# hostgen

hostgen writes a decompilation's C again, as C that runs natively in the
port. It is how the decompiled games become programs: their C is the C
that matched the console's executable, byte for byte, and hostgen keeps its
meaning while moving it to a 64-bit PC.

```sh
python3 port/tools/hostgen/hostgen.py --game port/game/rac1/hostgen.json \
    --source games/rac1/pal --out build/hostgen/rac1
python3 -m unittest discover -s port/tools/hostgen     # its tests (needs Clang)
```

The port's build runs it ([port/cmake/Games.cmake](../../cmake/Games.cmake));
running it by hand is for reading its report. The decompilation is never
changed: hostgen reads prepared copies under `--out/prep`.

## What it does

Game memory is a 4 GB reservation whose offsets are the console's addresses
([guest.h](../../runtime/include/openrac/guest.h)), the model OpenGOAL uses
for Jak's pointers. hostgen writes the C against it:

| In the decompilation | In hostgen's C |
|---|---|
| a pointer, of any type | a `gaddr`: the 32-bit game address it held on the console |
| `*p`, `p->f`, `p[i]` | an access to game memory: `GREF(T, addr)`, `((S *)G(p))->f` |
| pointer arithmetic | scaled by the pointee's size, which is the console's: every record keeps its layout, since its pointers are 4-byte `gaddr`s too |
| a global (`D_0013E650`, or a named one with its address) | the object at its retail address in game memory |
| a local whose address is taken, a local array, struct or union | a slot in a frame on the game stack (`GFRAME`), given back when the function returns |
| a string literal | copied into game memory once (`GSTR`) |
| a direct call | a call to the callee's **definition**, its arguments matched to the definition's parameters as the EE passes them (below) |
| a call through a pointer | `GFN`: the table of functions by code address, per level program |
| a function's address | its code address, as a number |
| `qcopy`, `qcopy_nc`, `qzero` (the decompilation's inline assembly) | the runtime's host versions |

Because addresses keep their meaning, the C's habits work unchanged:
absolute addresses, a global plus a large offset into its neighbours, the
uncached alias of RAM, the scratchpad at `0x70000000`, pointers held in
`int`s.

### Calls

The decompilation's files often declare a callee differently from its
definition (they were written to match machine code, one function at a
time). On the EE that is harmless where the registers line up, so hostgen
does what the EE did:

- Integers (and pointers) and floats travel in separate registers. The
  call site's arguments are split by kind and given to the definition's
  parameters by kind, in order, so `f(float, int)` reaches `f(int, float)`
  correctly.
- A parameter the call does not pass was whatever its register held: often
  the caller's own argument in the same register ("passed through"). hostgen
  passes the caller's parameter in that position, or 0, and counts these
  in its report.
- A result the caller reads but the callee does not return is 0.

### What it does not translate

A function with inline assembly other than `sync` and the quadword
helpers, or another construct hostgen does not write, becomes a stub that
logs that it ran; the report lists each one and why. Functions with no C at
all (still assembly in the decompilation, or libraries) are stubs too,
unless the game's `libraries.tsv` says the port writes them itself.

## Preparing the sources

[prep.py](prep.py) copies the files it reads and changes the copies without
moving a line: `long` becomes `long long` (the EE's long is 64-bit), and
file-scope `__asm__` (padding for the matching build) is blanked.
[clangast.py](clangast.py) has Clang read each file for a 32-bit MIPS
target, so every type has the console's size and layout. Where Clang
refuses what GCC 2.95 accepted, the copy is fixed from Clang's own
diagnostics: a call before the callee's declaration gets GCC's implicit
`int f()` (through an alias, so the later declaration stays as it is); a
call with too few arguments to a K&R function passes zeros.

## A game's configuration

`port/game/<game>/hostgen.json`:

| Key | |
|---|---|
| `sources`, `skip` | the C to translate (`src`, without `src/libgcc`: the host compiler has its own) |
| `includes` | the include directories, relative to the decompilation |
| `places` | where each level function sits in each level (`config/overlays/functions.tsv` of rac1/pal) |
| `libraries` | the table of library entry points ([port/game/rac1/libraries.tsv](../../game/rac1/libraries.tsv)) |
| `roots` | where the frontier report starts (the game's `main`, its title and level loops) |

`libraries.tsv` says, for each library function the game calls, whether the
decompilation's C runs as it is (`game`), the port writes its own (`host`,
with the signature callers are matched to), the C runs under a host
function of the same name (`wrap`: the C is kept as `<name>__game`), or a
host version is still to be written (`todo`).

## Output

| File | |
|---|---|
| `<unit>.c` | each file of the decompilation, written again |
| `game_protos.h` | every function of the program, with the signature calls are matched to |
| `stubs.c` | the functions with no C yet |
| `functions.c` | every function by code address, per level program (`openrac_game_register_functions`) |
| `report.md`, `report.json` | what was translated, what became a stub and why, and **the frontier**: the functions without C that the program reaches from its roots, nearest first. That list is where decompiling moves the port on. |

Names other than `func_...` get a `game_` prefix in the generated C, so
the game's own `memcpy` or `main` never meets the host's.

## Files

| File | |
|---|---|
| [hostgen.py](hostgen.py) | the command: prepare, index every unit, translate every unit, write the program files and the report |
| [prep.py](prep.py) | preparing the sources |
| [clangast.py](clangast.py) | running Clang, fixing what it refuses, following its JSON's locations |
| [ctype.py](ctype.py) | C types as Clang prints them, parsed and printed back for the host |
| [program.py](program.py) | what the whole program defines: signatures, addresses, typedefs |
| [lower.py](lower.py) | the translation of one unit |
| [test_hostgen.py](test_hostgen.py), [tests/harness.c](tests/harness.c) | small programs in the console's C, translated, built, run and checked |

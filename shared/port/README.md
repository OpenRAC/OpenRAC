# shared/port: what a version does not take

[tools/port.py](../../tools/port.py) carries matched functions between versions
([docs/engine/SHARED_CODE.md](../../docs/engine/SHARED_CODE.md), "Porting by
machine"). These lists say what it must leave alone.

| File | What it lists |
|---|---|
| [rac1-pal.never.txt](rac1-pal.never.txt) | The movie player and four libmpeg helpers: never ported, redone from the assembly alone ([sourcing policy](../../docs/policy/SOURCING.md)). The tool also excludes everything under rac1/pal's `src/game/movie/` by folder, whatever the list says. |
| [rac1-pal.undecided.txt](rac1-pal.undecided.txt) | Functions held back until someone has reviewed the source project's C. Empty. |

## Review of 2026-10-04: eleven library functions Lombyte has matched

Held back on 2026-10-02 because they are library code, where the sourcing
policy asks for a second look. Each was read in Lombyte's tree
(`games/rac1/ntsc/src`) for what the policy names: C that arrives complete
with SDK-style names, types or macros.

| rac1/pal | Lombyte's unit | What it is | Finding |
|---|---|---|---|
| `func_001126D8` | `sdk/library/_dtoa_r.c` | David M. Gay's dtoa, as in newlib | Open source; carries its AT&T notice, which must stay with it |
| `func_00114920` | `sdk/library/malloc_r.c` | newlib's `_malloc_r` (Doug Lea's allocator) | Open source, newlib's license |
| `func_00116168`, `func_001161B0` | `runtime/newlib/double_is_not_nan.c`, `math/classify_float_bits.c` | fdlibm's NaN and finite tests, as in newlib | Open source; the `do { } while (0)` is fdlibm's own `EXTRACT_WORDS` |
| `func_0011BF80`, `func_0011C388`, `func_0011C5C0`, `func_0011C820` | `sdk/library/sceopen.c`, `sce_lseek.c`, `sceRead.c`, `scewrite.c` | File I/O over SIF RPC | Written from the assembly: globals by address, the authors' own request structs and locals, helper names that are guesses. The semaphore struct follows the public ps2sdk's names. No SDK types or macros |
| `func_0011AA90` | `sdk/rpc/sce_sif_send_cmd.c` | `_sceSifSendCmd` | Written from the assembly (locals `mode44`, `addr0`). Its two small structs use short member names; rac1/pal's copy names those members by offset, so nothing rests on where the names came from |
| `func_0012E820`, `func_0012EB18` | `audio/rpc/snd_send_iop_command_no_wait.c`, `snd_send_current_batch.c` | 989snd command senders | Written from the assembly: globals by address, guessed helper names |

Released for porting. Function names such as `sceOpen` are the library's
public interface, as rac1/pal already uses them.

## What the port into rac1/pal still gets wrong (2026-10-09)

After Lombyte's pull requests 109 to 132, `tools/port.py` gave 206 candidates
for rac1/pal: 122 passed its check as written and 84 did not. Workers finished
48 of the first 53 they were given without changing any logic, most of them in
minutes, which makes these rules for the tool to learn (none is in it yet):

1. **The function itself declared otherwise in the target file** (38
   candidates, all of "the target's file already declares it with other
   types"). The file declares the function with another prototype at its
   caller or where it is passed as a callback, sometimes after the stub, so
   the compiler's "previous declaration" line can point at the candidate
   itself. Scan the whole file and define the function under an alias:
   `RET FUNC_r(ARGS) __asm__("FUNC");` then `RET FUNC_r(ARGS) { ... }`.
   rac1/pal's `tools/integrate.py` cannot place such a definition yet.
2. **Types the target file defines anywhere** (`struct Moby`, `Vec4f`,
   `Vec4`, `u128`, the Hero structs: earlier ports brought them in). Suffix
   every typedef and struct tag of the candidate with the function's address,
   and cut a large carried-over struct down to a private one with the fields
   used. No effect on the code.
3. **A `$gp` global read or written through a cast of `extern short`** next
   to a store through a struct pointer: rac1/pal's compiler schedules the cast
   access after the store. Emit the real type under a suffixed name with
   `SDATA(sym)` and use it plainly. This alone closed more than half of the
   candidates that were 11 to 30 bytes off.
4. **A table or scalar that the retail code reaches with `lui` and
   `addiu`/`lw` together on one register**: unsized with `MACRO_ADDR`. A one-
   or two-byte scalar reached with `lui` must be an unsized array alias, or it
   goes through `$gp`. A plain one- or two-byte declaration of the same symbol
   anywhere in the target file makes it small for the whole file: look for it
   before emitting a `MACRO_ADDR` alias.
5. **`volatile`** never travels: a second C name on the same assembler symbol
   for the later reads does what it did.
6. **Float stores through `((float *)d)[n]`** beside a typed global become
   struct member stores.
7. **Names the tool leaves unmapped**: a bare numeric address in the source
   (`*(u8 *)0x15EDB3` is a US address), and a named resident function called
   from level code, which came out as a data symbol on the US address.
8. **Real version differences**: one displacement or constant that differs
   between US and PAL. These can be patched from the diff of the two
   functions' instructions.

Two things are not the C at all: GNU as puts two `nop`s between an `mfc1` and
a branch that reads its register, where the retail assembler has none (two
functions, `func_L00_00269BE8` and `func_L00_002761C0`); and rac1/pal's check
resolves an unpaired `%hi` of a function that has two identical copies in a
level to the other copy (`func_L00_002C9820`).

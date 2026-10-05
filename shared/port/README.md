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

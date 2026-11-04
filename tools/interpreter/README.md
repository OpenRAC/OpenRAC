# Test-only interpreter

This is a test-only interpreter used solely by tooling (the backfill gate,
`tools/fcheck_gate.py`) to compare candidate C against retail on captured
calls. It is never built by the port's or the launcher's builds, never shipped,
never bundled, and never used to play the game. The native port does not
emulate.

It is a standalone CMake project:

```sh
cmake -S tools/interpreter -B build/interpreter -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/interpreter -j 12
ctest --test-dir build/interpreter
```

Programs: `openrac-boot` (headless run of a retail disc, `--capture` writes
call states) and `openrac-fcheck` (runs retail and a candidate from a state and
compares). Restored from acb726d5 (the fcheck side line); see
docs/port/BACKFILL.md.

`openrac-fcheck` delivers no interrupts, frames or disc, so it cannot check a function that waits on
them. `openrac-boot --swap ENTRY:FIRSTWORD:TARGET:FILE[:ADDRESS:FILE...]` can: it sends a whole run's
calls of ENTRY to code loaded elsewhere, while FIRSTWORD is at ENTRY. With `--dump-ram FIELD[,FIELD...]`,
compare main memory at those fields with a plain run under the same script and `OPENRAC_CLOCK`, leaving
out the candidate's code and the stack. `--digest FILE` hashes that memory every field, and `--cover ADDRESS:BYTES FILE`
writes which instructions of a function the run ran. `--returns ENTRY:FIRSTWORD FILE` logs each return of a function:
its result, kept registers and a memory hash. Compare memory, not just the random state and the hero's place:
a loader with its `resume` test inverted passed those, and memory caught it. It covers only what the
route ran.

The EE is charged 2 cycles an instruction. At that rate Going Commando's
level 0 misses a field now and then, so a `--trace` repeats or skips a game
tick and a `--press` lands on a different tick than the port's. For a
frame-by-frame comparison with the port, set `OPENRAC_BOOT_CPI=1`: the game
then runs one tick a field, and field f+1 of the trace is the port's tick f.

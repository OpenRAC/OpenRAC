# The host build of a game's decompiled C

This is how the port grows out of the interpreter
([DESIGN.md](../docs/DESIGN.md), section 3, route C). `port.py` compiles a
game's decompiled C for the host, as a library; the runtime loads it and runs
each decompiled function in place of the retail one, on the same memory
image. Whatever is not decompiled yet, or is left out below, the interpreter
keeps running. Nothing here changes how the decompilations build or match.

```sh
python3 runtime/port/port.py build rac1/pal          # about ten seconds
build/runtime/openrac-boot DISC.iso --hooks runtime/games/SCES_509.16.hooks --window \
    --native build/port/rac1-pal/libopenrac-native.dylib
```

It needs LLVM (clang with the wasm32 target, `wasm-ld`) and WABT (`wasm2c`);
with Homebrew, `brew install llvm lld wabt`. The library is written to
`build/port/<game>-<version>/` and holds code built from the repository's C
only. The sizes and checksums of the retail functions it stands in for are
read from your own disc's files, which `tools/openrac.py setup` and the
game's own set-up put in `games/<game>/<version>/baserom/`.

State on 2026-10-09, Ratchet & Clank (PAL): 299 of 316 source files are in
the library, 2,222 functions. From the boot to standing in the first level,
284 of them run (512,000 calls) and the level draws as it does interpreted;
checked both ways (below), none of them differs from the retail code.

## How it works

1. **32-bit pointers.** The C is written for the console: pointers are 4
   bytes and data sits at the retail program's addresses. Each source file is
   compiled to WebAssembly (wasm32), where both still hold, and `wasm2c`
   turns the result into C for the host. The module's memory is the guest's
   whole address space, mapped so that a guest address is an offset from one
   base (`src/ps2/memory.*`).
2. **One module a source file.** A call that leaves the file goes through the
   EE's registers, as the console's calling convention has them
   (`src/sys/native_abi.h`): integers in registers 4 to 11, floats in FPU
   registers 12 to 19, the rest on the stack. So it reaches retail code or
   another module alike, calls through function pointers work (a pointer in
   the game's memory is a guest address), and a function that two files
   declare with different prototypes does no harm.
3. **Retail globals and function addresses** are values the host supplies:
   the code is built position independent, and each symbol named after its
   address (`D_0013D9B4`, `func_L00_0025B4D0`) resolves to that address.
4. **Bound by checksum.** A host function runs at an address only while the
   guest's memory there holds the retail code it was written from; this is
   checked when the address is first reached and again after the program
   says it changed code (a level loading). A level's functions are therefore
   used in that level only, and nothing is used on another version's disc.
5. **A program for each level.** Ratchet & Clank loads a program of its own
   for each level, and each has its own copy of the engine at its own
   addresses: the function the boot program has at `001F9EE8` is at
   `001FF6A8` in level 0 and at `00221538` in level 1. A name therefore
   stands for an address in the program that is in memory. The library
   carries, for every name its code uses, the address in each level's
   program, from the decompilation's catalogue
   (`config/overlays/functions.tsv`); the runtime finds which level's program
   is loaded by a checksum of the start of its code and tells the library,
   whenever code changed. Where a level has a small function more than once
   under one name (several of the original objects each carried a copy), the
   copy a source file means is the one its functions' retail code calls.
6. **The console's arithmetic and devices.** Float operations go through the
   runtime's model of the console's FPU (no infinity, cut towards zero), so
   host code leaves what the retail code leaves. Loads and stores at the
   device addresses go to the runtime's register model. Host code's own
   stack and data are in the first megabyte of main memory, the kernel's on
   the console, because the game hands local buffers to the DMA controller
   like any other address.

`include/` holds the `common.h` and `include_asm.h` that stand in front of a
game's own in this build: the same types, the steering macros of the retail
compiler made empty, assembly never included, and the three quadword helpers
in plain C. `long` is 8 bytes on the console, so the build defines it as
`long long`.

## Checking a function against the retail code

```sh
build/runtime/openrac-boot DISC.iso --hooks ... --frames 1000 --no-card \
    --native LIBRARY --native-check 4
```

checks each function's first four calls: the retail function runs in the
interpreter, the machine is put back, the host function runs, and what the
two left in memory and in the result register is compared. A function that
differs is handed back to the interpreter and listed at the end with the
first difference. A call that reaches outside memory (a library function, a
device) cannot be run twice and is not compared; the count is printed. A
retail function that does not come back without a timed event (it waits for
an interrupt) is not compared either: all it did is taken back and the
interpreter makes the call. A host function that reaches outside memory
where the retail one did not counts as differing.

`--native-range FIRST:LAST` and `--native-skip FILE` use only part of the
library, for finding by halving which function changes a picture.

Native code takes no emulated time, so a load that took thirty fields
interpreted takes fewer: pictures of the same field number are not
comparable between the two ways of running. Compare what does not move (a
menu's text), or use the check above.

## Functions left to the interpreter

`leave/<game>-<version>.txt` lists decompiled functions that the build
leaves out, each with the reason. They match the retail bytes and still do
something else on a host. The fix is in the decompilation, checked against
the match, and the line goes when it is made. Two causes so far:

- **A 64-bit value passing through declarations typed as 32 bits**, which a
  register move on the console carries whole. `wide/<game>-<version>.txt`
  names the functions whose integer result is 64 bits wide whatever a file
  declares (one so far, the texture register value); the build reads every
  declaration of them as returning `long`, which is enough where the caller
  stores the result in a 64-bit place. A caller that keeps it in an `int` of
  its own is listed in `leave/`.
- **Locals laid out for a callee.** A function fills a structure that
  nothing reads and passes the address of the local next to it: the callee
  reads across both, which works only with the retail compiler's stack
  layout (the original source had one structure). `port.py` finds these
  itself, from the unoptimised compiler output: a function with an array or
  structure that is stored to and never read or handed on is left out, and
  the names are written to `left_by_stack_layout.txt` beside the library
  (11 today).

Source files that do not compile for the host are reported by `port.py` and
left out as a whole: today 14 of the first game's, each for a function
declared with two different prototypes in one file, which the retail
compiler accepted.

## What is not done

- A function that exists in several levels at different addresses is used
  only at the address its name gives: the engine's functions run as host
  code in the level they were decompiled from and interpreted in the others.
  Using them everywhere needs the address of each global they name in each
  level, which the retail code of the function's copies gives (the same
  instruction in each copy holds that level's address).
- The other games: `SOURCE_DIRS` in `port.py` lists the first game only.
- Speed: every float operation is a call into the model, and every load and
  store tests for a device address.
- The both-ways check does not cover functions that reach outside memory.

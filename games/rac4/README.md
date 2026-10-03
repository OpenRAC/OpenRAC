# Ratchet: Deadlocked (2005)

Released in Europe as *Ratchet: Gladiator*. **Not started.**

The NTSC-U disc, `SCUS_974.65` (disc v1.00), is identified in
[game.json](game.json) with its checksums and those of its boot executable,
so work can begin from it ([baserom/README.md](../../baserom/README.md)).

## Starting the decompilation

What the other games suggest, in order:

1. **Measure before choosing.** A cross-game function map (the method rac1/pal
   uses between the US and PAL builds, [OVERLAYS.md](../rac1/pal/docs/OVERLAYS.md))
   run between this boot executable and RAC3's shows how much code carries
   over, and which project's conventions fit best.
2. **Pick the toolchain from evidence.** Deadlocked followed Up Your Arsenal
   on the same engine; [docs/toolchains](../../docs/toolchains/README.md#choosing-a-toolchain-for-deadlocked)
   lists what to try first.
3. **Set up `games/rac4/ntsc/`** as a self-contained project
   ([games/README.md](../README.md#adding-a-version)), add its inputs to
   `game.json`, and commit the first step as `feat(rac4): ...`.

Everything follows the [sourcing policy](../../docs/policy/SOURCING.md): the
retail disc you own is the evidence, and nothing taken from leaked material is
used, whatever circulates about this game.

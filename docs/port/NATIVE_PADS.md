# Native SP pad lifecycle

Rac3 PAL repeats its pad startup in the boot program, frontbin and single-player levels. NTSC-U
does not: the boot program initializes the core records, which survive program replacement.
The native updates must change readers without resetting those records on NTSC-U overlay entry.
Multiplayer still uses its existing library path. This does not change cards or kernel services.

## Evidence

The owned NTSC-U disc and pinned rac3 catalogues establish these roles:

- Boot caller `0x003984CC` calls initializer `0x003A2E80`. Its 280 bytes differ from PAL's
  initializer only in the relocated clear call. It initializes eight records of `0x5C0` bytes.
- Core main `func_0013B330` runs the boot entry first, then calls entries returned by
  `func_0013AFF8`. That loader copies only each record's destination range; it has no pad reset.
  The pinned core C and the native program wrapper preserve that record-based handoff.
- The records occupy `[0x001CD0C0, 0x001CFEC0)`. The active and port-one pointers are at
  `0x001D52FC`, `0x001D5300`, `0x001D5304`; the readiness flag is at `0x001D4CF4`.
- Frontbin's records and all 34 SP level programs' records start at `0x001D5680` or above. No
  record overlaps those core fields. No SP overlay directly calls the pad/library initializer or
  port-open routine, or has a `$gp`-relative store to the two port-one core pointers.
- NTSC-U frontbin enters at `0x0037D200`. Its calls at `+0x400` and `+0x8D4` reach iterator
  `0x0039E8E0`, which passes each retained record to updater `0x0039E4B0`.
- Veldin enters at `0x003EE988`; its frame paths call iterator `0x0045E370`, which passes the
  same records to updater `0x0045DF40`. Both iterators are PAL's instructions except the updater
  call. The updates keep PAL's record-field offsets and core `$gp` references; only calls and the
  switch-table address at `+0x7C` differ.

The calling conventions remain the observed ones: no arguments to boot startup, one guest record
pointer to each update/clear, and record/report/byte-count to each reader. The SP programs all use
`$gp = 0x001DC8B0` (`config/gp.tsv`); core pointers and record offsets are not guessed from neighbours.

PAL's overlay startup and its frontbin clear are not entries in this NTSC-U lifecycle. They are
compiled only for PAL, not replaced with another program's address or a no-op. Clear dependencies
belong to the startup call, not to an update's reader table. NTSC-U keeps its real boot initializer
and the existing retail reader bodies, including its different flick-history lookback bounds.

## Tests and limits

- `rac3_pad_state`: real PAL host/shared pad C with synthetic RAM and game/input callbacks.
- `rac3_pad_ntsc`: the same fixture and host C carried by `release_host.py`, linked without mocks
  for any PAL-only startup or clear. It checks ready/disconnected/quiet paths and boot-to-overlay
  state retention with each program's own reader callback.
- `test_release_carry_rac3.py`: rejects unresolved `func_PAL_*` in the preprocessed NTSC pad graph,
  requires every native pad binding to have a definition, and permits only the proved clear/reader
  entries outside the host source. Clang is required for these compiler checks.

The tests need no disc or generated game headers. The fixture does not execute retail reader
instructions. Full game generation, linking, program switches and runtime controller/mod behavior
still need the coordinator's build and game checks; these unit tests do not claim P14 completion.

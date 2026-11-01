# Modding the game's sound

Ratchet & Clank in OpenRAC's native port can play your own music, speech and
effects in place of the game's. You export the sounds you want to change as WAV
files, edit or replace them in any audio editor, and import them back as a pack.
It works the same with either disc the game can be set up from.

Nothing here changes your disc image, and nothing you export may be shared: it
is your disc's data. A pack holds only the files you changed; share it only
with people who own the game, and only recordings you have the right to share.

## In the launcher

Everything is on the **Mods** page, under **Audio**:

1. **Export**: choose a level, or the whole game, and what to export: effects,
   music, speech or everything. The folder opens when it is ready, in `exports` in
   the game's set-up folder, named with the date and time you exported it:
   `audio-all-20261101T0900`, `audio-level_01-music-20261101T0900`. Exporting again
   writes a new folder (with `-2` within the same minute), never over your edits.
2. Edit or replace the WAV files, keeping each one's name and place. Delete any
   you do not need.
3. **Import your edits**: choose the export folder. The files you changed become a
   pack named after the folder, switched on, in My mods. Importing the same folder
   again updates that pack, or removes it if no file differs any more. A file that cannot be used is listed with what to do;
   the rest is still imported.
4. Turn on **Mods** under Play and play. Switch the pack off in My mods, or
   remove it, to hear the game's own sounds again.

| The whole game | About |
|---|---|
| Effects | 59 MB |
| Music | 475 MB |
| Speech, in all five languages | 1.6 GB |

A level's export is much smaller, and a level has only the sounds that play in
it: menu sounds and other sounds the whole game shares come with the whole game.

## The files

```
effects/Wrench 2.wav                       named after what plays them
effects/Novalis sound 3.wav                or the level, when the level itself does
effects/Ship flights/Flight 1 (left).wav   the ship's flights: each side is its own file
music/Novalis/Tobruk Crater (intro).wav
music/Novalis/Tobruk Crater (loop).wav
music/Novalis/Transition 1.wav
speech/English/Cutscenes/Novalis/Cutscene 1.wav
speech/French/Help/Veldin/Wrenchsmash.wav
speech/Not played/Vendor/Vendor 3.wav      on the disc, but the game never plays them
music/Not played/Enemy Loop.wav            the same
export.json                                which sound each file is: leave it there
```

The disc has no names for most effects, so they are numbered: listen to them to
find the one you want. An effect plays in every level that has it, and is exported
once.

A sound several levels share is one file: changing it changes it in all of them.
Exporting the whole game puts it in the first level that has it, so the music of
Rilgar's hoverboard race, which Kalebo III plays too, is in `music/Rilgar`.

The disc cuts speech's names to 16 characters, so two different lines can share one,
as `Hoverboardcon.wav` and `Hoverboardcon (2).wav` do. `Not played` holds a sixth
recording of some lines (Japanese where the PAL disc names them) and another encoding
of one piece of music: changing those does nothing.

An effect made of several tones exports only its first. Your file then replaces
the whole sound: where its tones play at once, your file plays once in place of
them; where the game picks one at random or plays them in turn, your file takes
the place of each. A tone that loops where the first does not, or the reverse,
goes silent: replace a door's opening clunk and its looping hum goes quiet, replace
a loop and the clunk that ends it does.

## Music

Most songs are two files. The game plays the intro once, then repeats the loop:

- Replace only the loop, and the game's intro still plays first.
- Replace both, and they need to join cleanly, and the loop's end to flow back into
  its own start.
- For a song with no intro, put it in the loop and make the intro a short silence.

The short `Transition` files play between songs. *Evacuate the Warship* is a single
file, as it does not repeat.

There is no official list of the songs' names that we could find. These come from a
[fan-made list](https://archive.org/details/ratchet-clank-2002-ost), chosen because it
agrees with the disc's own names (`L01_Cave`, `L01_Waterworks`) and the game's code,
and matched to the disc by each song's length and, where two have the same length, by
what in the game's code starts them. One of Kalebo III's songs is on no list: it is
`Untitled`.

## Your recordings

Any common WAV file works: 8, 16, 24 or 32-bit PCM, or 32 or 64-bit float, mono or
stereo, at 500 to 192000 Hz. Other formats (MP3, FLAC...) need saving as WAV first;
any audio editor can.

- **Music and speech** play as they are, at any length, in stereo if the file is.
  A track the game loops loops over your whole file, and the game's fades follow
  your file's length.
- **Effects** are mixed down to mono at 48 kHz, as the console's voices were, and
  keep the original's volume, pan and reverb. A sound that looped loops over your
  whole file.

## Both discs

A sound is known by what it is, not by where it is. The same effect is in many
levels under other numbers: one file changes it in all of them. The same goes for
the two discs:

| 2026-11-01 | PAL | NTSC-U | The same on both |
|---|---|---|---|
| Music | 109 | 109 | 109 |
| Speech | 1,498 | 1,492 | 1,477 |
| Effects | 1,048 | 1,049 | 1,047 |

The rest are recordings that differ, so a file made for one of them on one disc
does nothing on the other, and that sound stays the game's own.

## The pack format

Sound goes in a pack as any other mod does ([MODDING.md](MODDING.md#the-pack-format)),
and a pack may hold nothing else:

```
openrac-mod.json     {"format": 1, "game": "rac1", "version": "ntsc", "name": "..."}
audio/<key>.wav      a sound replaced: 16-bit PCM, mono or stereo, 500 to 192000 Hz
```

With several packs on, a later one wins where two replace the same sound.
`"version"` says only which disc the pack was made on.

A key is the first 16 hex digits of a SHA-256, in lower case:

- a **stream** (music, speech: a VAG file on the disc): of the file, header and
  all, up to its first 1 MiB;
- an **effect** (one sound of a 989snd bank): for each of its grains in order, its
  type and delay as little-endian 32-bit words, then for a tone the first 16 bytes
  of its parameters (not where its sample is) and the bytes of its sample up to and
  including the frame with the end flag, and for any other grain its 32 bytes of
  parameters.

`export.json` gives each exported file's key. `audio.py` works them out from the
disc and `port/audio/sound_replacements.cpp` from what the game loads;
`editor/test_audio.py` and `port/tests/audio/sound_replacements.cpp` hold the two to
the same answers.

## From a shell

```sh
python3 editor/audio.py export <disc.iso> build/my-sounds --what music --level 1
python3 editor/audio.py import build/my-sounds --into <data>/overrides
python3 editor/overrides.py stack --into <data>/overrides --out <data>/mods/<version> --levels <levels>
```

`--what` is `effects`, `music`, `speech` or `all`; without `--level`, the whole
game. The port logs what it did (`audio mods:` lines in the session log): how many
sounds of each bank were replaced, each stream played from a file, and any file it
could not use, in which case the sound stays the game's own.

## Limits

- Ratchet & Clank only: the other games' banks are not read yet.
- A movie's sound cannot be replaced.

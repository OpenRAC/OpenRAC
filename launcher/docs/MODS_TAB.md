# The Mods tab

What the Mods tab shows and where it lives in the code. All its text is in
[src/lib/modsText.ts](../src/lib/modsText.ts) (written `T.<key>` below). How
mods work: [docs/port/MODS.md](../../docs/port/MODS.md).

Rules for the text: written for players, one short line each, longer
explanations in a tooltip, and nothing for contributors unless developer tools
are on. `modsText.test.ts` keeps lines under 120 characters.

## Where things are

| Part                                       | Code                                                      | Shown when                                               |
| ------------------------------------------ | --------------------------------------------------------- | -------------------------------------------------------- |
| The tab                                    | `components/Header.svelte`                                | desktop only (Android has one Mods page)                 |
| Game list on the left                      | `pages/Mods.svelte` on `GamesFrame.svelte`                | always                                                   |
| Head: name, a line, Play with mods         | `pages/Mods.svelte`                                       | a game is picked                                         |
| Tabs: My mods, Level editor, Models, Audio | `pages/Mods.svelte`                                       | the level editor can open the game (Audio: rac1 only)    |
| Get mods (banner, first)                   | `components/GetMods.svelte`                               | My mods; hub buttons for Ratchet & Clank                 |
| Add a mod                                  | `components/AddMod.svelte`                                | My mods                                                  |
| Toolbar: search, kind, order               | `components/ModsToolbar.svelte`, `lib/modsView.svelte.ts` | six or more mods and packs                               |
| Gameplay mods                              | `components/PluginsPanel.svelte`                          | the game has a port that plays                           |
| A mod's own page (settings)                | `components/ModPage.svelte`, `ModSetting.svelte`          | its row was opened                                       |
| Mod updates                                | `components/ModUpdatesPanel.svelte`                       | an update waits, is held back, or just went in           |
| Graphics and sound packs                   | `components/PacksPanel.svelte`                            | the editor can open the game, or it has a texture pack   |
| Make a texture pack                        | `components/TexturePacksBlock.svelte`                     | Models tab; a fold with an arrow where there are no tabs |
| Level editor                               | `pages/Mods.svelte` (tab `editor`)                        | the editor can open the game                             |
| Models and textures                        | `components/AssetsPanel.svelte`                           | the editor can open the game                             |
| Audio                                      | `components/AudioPanel.svelte`                            | the disc can export audio                                |
| For developers                             | `pages/Mods.svelte`                                       | developer tools on                                       |

## The idea

**My mods** holds everything the game loads. The other tabs make things, and
what they make shows up in My mods. The launcher does not list mods itself:
the **Get mods** banner sends players to the hub (openrac.dev/mods) or tells
them a GitHub page, a `.zip` or a folder works too.

## The game list

One state per game (`T.GAME_LIST`): ready, no disc set up, working, early
access, gameplay mods only, or not moddable yet.

## The page per kind of game

| Game                                      | What shows                                                 |
| ----------------------------------------- | ---------------------------------------------------------- |
| Playable with the editor (rac1, checkout) | Play with mods and all tabs                                |
| Playable, no editor (installed launcher)  | Add a mod, Gameplay mods, packs; no tabs; the texture fold |
| Early access (rac2, rac3, rac4)           | the tabs, a dashed early-access line, add by file only     |
| Neither                                   | `T.PAGE.noneTitle`, `noneLine` and a link to openrac.dev   |

Play with mods is the one orange button. When it can't start, the line under
it says why (`T.PAGE.playEarly`, `playMissing`, `playBuild`).

## Add a mod

One panel for every kind. The player never picks the kind: `lib/addMods.ts`
sends gameplay mods to Gameplay mods, packs to the packs, and a texture pack
`.zip` into the player's own texture pack.

- **Link field:** a GitHub repository, branch, folder or release. A mod's page
  on openrac.dev (or an `openrac://` link) opens the install screen. Any other
  openrac.dev page says `T.ADD.notModPage`.
- **File, Folder,** or drop a file on the window (desktop).
- A link shows a preview first: each mod, whether it fits, and an Install,
  Reinstall or Update button. Nothing installs without a click.

## Mod codes

Friends match their mods (`T.MOD_CODES`; how it works: MODS.md, section 10, Mod codes). The field
of Add a mod takes a code as well as a link, and a code pasted anywhere on the page opens the same
review (`lib/modCodes.ts`, `looksLikeCode`). So does `openrac://modset/CODE`, from any page.

| Part             | Component                                  | Shown when                                                                                       |
| ---------------- | ------------------------------------------ | ------------------------------------------------------------------------------------------------ |
| Share these mods | a button in `PluginsPanel.svelte`'s head   | a gameplay mod is on                                                                             |
| The Share window | `ModSetShare.svelte` on `ModSetDialog`     | that button: what the code holds, suggestions to tick, Make code, then the code to copy          |
| The review       | `ModSetReview.svelte`, at the app's top    | a code was entered: every change grouped, Apply. Done: what came of it, and Put my old mods back |
| Earlier mods     | `ModSetSaved.svelte`, in the gameplay mods | a code was applied before: the last ten, each with Restore                                       |

| Review state                                       | Words                               | Apply          |
| -------------------------------------------------- | ----------------------------------- | -------------- |
| Everything matches                                 | `T.already`                         | none, Close    |
| A source neither OpenRAC's nor used before         | `T.trustTitle`, `trustLine`, a tick | after the tick |
| The game is running                                | `T.running`                         | when it closes |
| Some of it cannot be had (gone, changed, no build) | `T.partial`, each with its reason   | does the rest  |
| Another OpenRAC                                    | `T.otherOpenrac`                    | yes            |
| Another game, and that game is not set up          | `T.otherGame`                       | none, Close    |

The Share window with no server gives no short code: `T.offline`, and the full code to send.

## Gameplay mods

One row per mod: switch, name and one line, one badge (the most urgent),
Settings, and a menu (move, open folder, remove). Clicking the row opens the
mod's own page. Check for updates shows when a mod came from a link.

A mod's page has its version, source, actions (update, keep this version,
remove), presets, and its settings in groups. Settings save as they change.
Back or Escape returns to the list.

Purist mode (Settings) turns every mod off; the panel says so and offers to
turn it off.

No folder path is shown outside a folded section, since players share
screenshots. Messages show only the last part of a path (`lib/paths.ts`).

## Many mods: search, kinds, order

With six or more rows the toolbar shows:

- a search,
- kinds with counts: All, Gameplay, Graphics and sound, Tools (empty kinds
  are hidden),
- the order: Load order or A to Z.

In load order with no search, drag a row by its grip (`lib/dragSort.ts`,
mouse or finger), or use the menu's Move up, down, to the top or bottom (pad).
Sorted or searched, nothing moves and `T.VIEW.orderFrozen` says so. A search
that finds nothing says so once.

## Graphics and sound packs

Your texture pack comes first: its switch, a badge, Make your own, and a menu
(Share my pack, Replace with a pack, Open its folder). Then each pack with a
grip, a switch, what it replaces, up, down and remove.

## Level editor, Models and textures, Audio

- **Level editor:** open a level in Godot, then pack your changes. Each step
  says what is missing (disc, Godot) or how it went.
- **Models and textures:** export a level's assets, import your edits, and the
  texture pack steps.
- **Audio:** the same for sound ([editor/AUDIO_MODDING.md](../../editor/AUDIO_MODDING.md)).

What they make shows up under Graphics and sound packs.

## Installing from the hub

The hub's Install button opens the launcher with an `openrac://` link. The
install screen (`components/DeepLinkInstall.svelte`) shows what will install,
what it needs, warnings, and then the progress. It reads the signed mod list
(`core/src/index.rs`). The launcher reads that list only for a link, Share
and the update check, never to show it.

## Updates

- Small updates install by themselves: at start, every 6 hours and before
  Play, never while the game runs (`lib/modAutoUpdate.svelte.ts`).
- Big ones, and native mods with new hooks, wait for a click in Mod updates.
- Held-back ones say why ("Held back: A needs B 1.2.0").
- A version withdrawn from the hub is switched off by the update check, and
  Mod updates says so (`revokedOffText`).
- After a crash that follows an update, the crash notice offers a roll back.

Updates go to `plugins/.staging/` first; the old version is kept in
`plugins/.previous/`.

## Android

One page, no tabs: Get mods, the toolbar, Gameplay mods (Enhanced graphics
first), the packs, the texture pack steps (folded), then Add a mod. No level
editor, models or audio export (they need Godot and a desktop). Native mods
don't load; script mods do. A picked `.zip` is copied into the app first.

## Developer view

With developer tools on: a mod's id and hooks, the commit and files a link
reads, and a folded "For developers" panel with folders and the commands
behind the buttons. Players never see these.

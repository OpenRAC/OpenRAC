// Every word the Mods tab shows (Mods.svelte, AddMod.svelte, PluginsPanel.svelte, PacksPanel.svelte,
// AssetsPanel.svelte), with CrashNotice.svelte's mods line, the game page's Mods switch, lib/plugins.ts and
// lib/addMods.ts, in one place (launcher/docs/MODS_TAB.md lists where each shows). Written for
// players: short, plain, one idea per line; the longer "why" goes in a tooltip.
// Plain constants, maps from a state to its words, and small functions where a name or a number goes
// into a sentence: no decision is made here, only wording.

export type ModTone = "ok" | "off" | "warn" | "err";

/** A dot and a few words. */
export interface Label {
  text: string;
  tone: ModTone;
}

/** An installed mod's state on its row (lib/plugins.ts, pluginState, decides which). */
export const STATE_LABEL = {
  on: { text: "On", tone: "ok" },
  off: { text: "Off", tone: "off" },
  purist: { text: "Mods off at Play", tone: "off" },
  failed: { text: "Not loaded", tone: "err" },
  refused: { text: "Cannot load", tone: "err" },
} as const satisfies Record<string, Label>;

/** What a mod runs (lib/plugins.ts, modKind): code, scripts in the sandbox, or data only. */
export const KIND_LABEL = {
  native: { text: "Runs code", tone: "warn" },
  script: { text: "Sandboxed script", tone: "ok" },
  tool: { text: "Launcher page", tone: "ok" },
  pack: { text: "Textures and models", tone: "off" },
  builtIn: { text: "Experimental", tone: "warn" },
} as const satisfies Record<string, Label>;

/** The kinds' tooltips: what each means, for whoever wonders. */
export const KIND_HINT = {
  native: "A program: it runs on this computer with the game's rights. Install it only from people you trust.",
  script: "Runs in the game's sandbox: it reaches only what the game lets scripts change, never your files.",
  tool: "A page of its own in the launcher, kept apart from it: it reads the game's levels and saved textures, and saves only where you choose.",
  pack: "Data only: it replaces textures, models or levels, and runs nothing.",
  builtIn: "Part of the game, off unless you turn it on. Still being worked on.",
} as const satisfies Record<KindKey, string>;
type KindKey = keyof typeof KIND_LABEL;

/** Whether a mod from a link can be installed here, in its preview row. */
export const COMPATIBLE_LABEL = {
  yes: { text: "Works here", tone: "ok" },
  no: { text: "Cannot be installed", tone: "err" },
} as const satisfies Record<string, Label>;

/** The Mods tab's game list (Mods.svelte, modState). */
export const GAME_LIST = {
  gameplayMods: "Gameplay mods",
  notModdable: "Not moddable yet",
  early: "Early access",
  working: "Working…",
  setUp: "Set it up first",
  ready: "Ready to mod",
} as const;

/** The page around the panels (Mods.svelte). */
export const PAGE = {
  lead: "Add mods, switch them on, and play.",
  tabsLabel: "Mods",
  tabMine: "My mods",
  tabEditor: "Level editor",
  tabAssets: "Models and textures",
  tabAudio: "Audio",
  earlyTitle: "Early access",
  earlyLine:
    "Its levels open in Godot and you can make texture and model packs. Packing level edits and playing come later.",
  noneTitle: "No mods for this game yet",
  noneLine: "Mods arrive once the level editor can read its levels.",
  noneLink: "Follow the progress on openrac.dev",
  makeOwn: "Want to make your own?",
  makeOwnButton: "Edit a level in Godot",
  developerTitle: "For developers",
} as const;

/** My mods' first line (GetMods.svelte): where mods come from. The launcher lists none itself. */
export const GET_MODS = {
  title: "Get mods",
  lead: "Find mods on the official hub, openrac.dev, or anywhere else: a GitHub page, a .zip or a folder. Add them below.",
  leadNoHub: "Find mods anywhere: a GitHub page, a .zip or a folder. Add them below.",
  hub: "Open the mod hub",
  hubUrl: "https://openrac.dev/mods",
  publish: "Publish your mod",
  publishUrl: "https://openrac.dev/mods/submit",
} as const;

/** Adding a mod: a link, a file or a folder (AddMod.svelte). */
export const ADD = {
  title: "Add a mod",
  linkLabel: "A mod's link",
  linkPlaceholder: "Paste a mod's link, like openrac.dev/mods/lock-strafe, or a friend's mod code",
  notModPage: "That page isn't a mod. Open a mod's page on openrac.dev and copy its link.",
  go: "Find",
  file: "File…",
  folder: "Folder…",
  pickZip: "Choose a mod (.zip)",
  pickFolder: "Choose a mod's folder",
  copying: (name: string) => `Copying ${name} into the launcher…`,
  adding: (name: string) => `Adding ${name}…`,
  addingSeveral: (count: number) => `Adding ${count} mods…`,
  drop: "or drop a .zip or folder on this page",
  dropHere: "Drop to add it",
  close: "Close",
} as const;

/** The gameplay mods' list (PluginsPanel.svelte): a row each, its own page one click away. */
export const PANEL = {
  title: "Gameplay mods",
  checkUpdates: "Check for updates",
  updateAll: (count: number) => `Update all (${count})`,
  updateRow: "Update",
  empty: "No gameplay mods yet. Paste a mod's link above, or choose its file.",
  settings: "Settings",
  details: "Details",
  moveUp: "Move up",
  moveDown: "Move down",
  orderTitle: "Load order: a mod lower in the list goes after the ones above it.",
  keepVersion: "Keep this version",
  letUpdate: "Let it update",
  homepage: "Its page",
  openFolder: "Open its folder",
  remove: "Remove",
  hooks: "Hooks",
  license: "License",
  source: "Source",
  folder: "Folder",
  id: "Id",
  interface: "interface",
} as const;

/** My mods' toolbar when there are many (ModsToolbar.svelte, lib/modsView.svelte.ts). */
export const VIEW = {
  label: "Show my mods",
  search: "Find a mod",
  searchPlaceholder: "Find a mod by name or words",
  clear: "Clear",
  all: "All",
  gameplay: "Gameplay",
  looks: "Graphics and sound",
  tools: "Tools",
  sortLabel: "Order",
  sortOrder: "Load order",
  sortName: "A to Z",
  /** Under a list sorted by name or searched: it cannot be moved like that. */
  orderFrozen: "Sorted for viewing only: switch to Load order, with no search, to move mods.",
  none: (query: string) => `Nothing matches “${query}”.`,
  noneInGroup: "None of this kind yet.",
  grip: (name: string) => `Drag to move ${name}`,
  moveTop: "Move to the top",
  moveBottom: "Move to the bottom",
  toolsTitle: "Tools",
  builtInTitle: "Built in",
  toolsSub: "Pages of their own in the launcher, always on: they make things, the game does not load them.",
  /** A tool's switch: on, greyed, and why. */
  toolAlwaysOn: "Always on: a tool is a page in the launcher, so it can't be turned off.",
  toolAlwaysOnShort: "Always on",
} as const;

/** The badge on a mod's row besides its state and kind (lib/plugins.ts, modBadge). */
export const BADGE = {
  update: (version: string) => `Update ${version}`,
  updateHint: "A newer version is out: update it with its Update button.",
} as const;

/** The sections of a mod's page (lib/plugins.ts, settingGroups). */
export const GROUP_TITLE = {
  only: "Settings",
  general: "General",
  controls: (name: string) => `${name} controls`,
} as const;

/** The built-in Enhanced graphics entry (core/src/enhanced.rs; its page is ModPage.svelte's). */
export const ENHANCED = {
  id: "enhanced-graphics",
  vulkan: "Plays with the Vulkan renderer while on.",
  about:
    "Light that bounces off the world, sharp sun shadows and Ratchet's own, and soft shade in corners, drawn by the game's Vulkan renderer. The bounced light is baked on this computer from your own levels: once, and again after they are read from your disc again.",
  lightingTitle: "Lighting",
  baking: "Preparing the lighting",
  ready: (n: number) => `Ready for all ${n} levels.`,
  some: (ready: number, n: number) => `Ready for ${ready} of ${n} levels.`,
  failed: (n: number) => (n === 1 ? "1 level could not be prepared." : `${n} levels could not be prepared.`),
  fallback: "A level without it keeps its original lighting.",
  noLevels: "Get the levels ready on the game's page first.",
  noBaker: "This copy of the game has no lighting baker.",
  noBakerHere: "Intel Macs ship without the lighting baker: the enhanced graphics keep the original lighting.",
  partial: (n: number) =>
    n === 1
      ? "1 level was stopped part way: preparing goes on from there."
      : `${n} levels were stopped part way: preparing goes on from there.`,
  prepare: "Prepare lighting",
  goOn: "Go on preparing",
  again: "Prepare again",
  remove: "Delete the lighting",
  removeAsk: (levels: number) =>
    `Delete the prepared lighting of ${levels === 1 ? "this level" : `these ${levels} levels`}? The game keeps its original lighting until you prepare it again, which takes a while.`,
  removeYes: "Delete",
  removed: "The prepared lighting is deleted.",
  advanced: "Advanced",
} as const;

/** A mod's own page (ModPage.svelte). */
export const MOD_PAGE = {
  /** A mod with a page of its own (core/src/mod_tools.rs): the button that shows it. */
  openTool: "Open",
  openToolHint: (name: string) => `Open ${name}'s page`,
  back: "Back to My mods",
  crumbsLabel: "Where you are",
  crumbRoot: "My mods",
  on: "On",
  off: "Off",
  about: "About this mod",
  aboutKind: "What it runs",
  presets: "Presets",
  resetAll: "Reset to defaults",
  resetAllConfirm: (name: string) => `Put every setting of ${name} back to its default?`,
  resetAllDone: "Every setting is back to its default.",
  noSettings: "This mod has nothing to set: switch it on and play.",
  /** A mod with a page of its own keeps its settings there. */
  toolSettings: "Its settings are on its own page: Open.",
  offNote: "Off: its settings are kept for when you switch it on.",
  update: (version: string) => `Update to ${version}`,
  more: (name: string) => `More for ${name}`,
  details: "Details",
} as const;

/** Saving as you go, at the top of a mod's settings (lib/autosave.ts). */
export const SAVE = {
  idle: "Changes save as you make them",
  waiting: "Saving…",
  saving: "Saving…",
  saved: "Saved. Used from the game's next start",
  failed: "Not saved",
  retry: "Try again",
} as const;

/** One setting's control (ModSetting.svelte). */
export const SETTING = {
  reset: (name: string, value: string) => `Reset ${name} to ${value}`,
  resetShort: "Reset",
  more: (name: string) => `More about ${name}`,
  less: (name: string) => `Less about ${name}`,
  valueAria: (name: string) => `${name}, value`,
  saved: "Saved",
  defaultMark: (value: string) => `Default: ${value}`,
  yes: "On",
  no: "Off",
  empty: "Blank",
} as const;

/** A key-binding setting (KeyBindSetting.svelte) and a mod's presets of them. */
export const KEYBIND = {
  press: "Press a key…",
  pressHint: "A key, a mouse button or the wheel. Click elsewhere to cancel.",
  cancel: "Cancel",
  none: "Not bound",
  add: "Add",
  swapped: "That one was its other bind: the two swapped.",
  presets: "Control presets",
  defaults: "Default",
} as const;
export const bindAria = (setting: string, bind: string) => `${setting}: ${bind}. Change`;
export const addBindAria = (setting: string) => `${setting}: add a binding`;
export const clearBindAria = (setting: string, bind: string) => `Remove ${bind} from ${setting}`;
export const alsoBound = (names: string[]) =>
  `Also bound to ${names.length === 1 ? names[0] : `${names.slice(0, -1).join(", ")} and ${names[names.length - 1]}`}`;

/** The packs' list (PacksPanel.svelte). */
export const PACKS = {
  title: "Graphics and sound packs",
  sub: "Textures, models, levels and sound",
  empty: "None yet. Add one above, or make your own in the Level editor, Models and textures, or Audio.",
  orderHint: "Lower in the list wins",
  moveUpTitle: "Move up: the packs under it win where they change the same thing",
  moveDownTitle: "Move down: it wins over the packs above it",
  remove: "Remove",
} as const;

/** The preview of what a link holds. */
export const PREVIEW = {
  nativeWarning: "Mods that run code are programs. Install them only from people you trust.",
  install: "Install",
  reinstall: "Reinstall",
} as const;

/** The player's own texture pack: a row of the packs (PacksPanel.svelte) and the steps that make
 *  it (TexturePacksBlock.svelte; core/src/texture_packs.rs). */
export const TEXTURES = {
  name: "Your texture pack",
  badge: "Texture pack",
  badgeHint: "Pictures that replace the game's own by their name, at any size.",
  make: "Make your own",
  share: "Share my pack…",
  replace: "Replace with a pack…",
  replaceYes: "Replace my pack",
  openFolder: "Open its folder",
  pickTitle: "Choose a texture pack (.zip)",
  replaced: (n: string, kept: string | null) =>
    kept ? `Your pack is now ${n}. The old one is kept beside it, as “${kept}”.` : `Your pack is now ${n}.`,
  added: (n: string, replaced: number) =>
    replaced
      ? `Added ${n} to your texture pack, ${replaced} of them in place of yours.`
      : `Added ${n} to your texture pack.`,
  makeTitle: "Make a texture pack",
  makeLead: "Save the game's pictures, repaint or upscale them, and they become your texture pack.",
  whereItIs: "Your pack is in My mods, with the other packs, where you switch it on, share it or replace it.",
  goToMine: "Show it in My mods",
} as const;

/** Mods, the switch under Play on the game's page (GamePlayer.svelte): off,
 *  the default, the game starts with no mods at all; the Mods tab says so above its list. */
export const WITH_MODS = {
  label: "Mods",
  hint: "Play with your gameplay mods, packs, packed levels and Enhanced graphics from the Mods tab.",
  /** Beside the switch when it is on: what Play starts now, how many mods, a way to the Mods tab.
   *  It says the state first, so it never reads as what the switch would turn to. */
  count: (n: number | null) =>
    n === null ? "On: with mods" : n === 0 ? "On: no mods picked yet" : `On: ${n} ${n === 1 ? "mod" : "mods"}`,
  /** Beside the switch when it is off. */
  original: "Off: the original game",
  offNotice: "Play starts the game without mods. Turn on Mods under Play to use them.",
  turnOn: "Turn on",
} as const;

/** The level editor's tab (Mods.svelte). */
export const EDITOR = {
  lead: "Change a level in Godot, then pack your changes and play them.",
  leadEarly: "Look around a level in Godot.",
  openTitle: "Open a level in Godot",
  packTitle: "Pack your changes",
  levelLabel: "Level to edit",
  noDisc: "Set the game up from your disc first.",
  setUp: "Set up the disc",
  noGodot: "Levels open in Godot 4, a free editor: get it, then choose where it is.",
  getGodot: "Get Godot",
  chooseGodot: "Choose Godot…",
  pickGodot: "Godot 4",
  ready: "Move, turn or scale objects, then save in Godot.",
  readyEarly: "Look around a level's objects and textures.",
  firstTime: "The first time reads the levels from your disc: about 20 seconds.",
  firstTimeEarly: "The first time reads the levels from your disc: a few minutes.",
  open: "Open in Godot",
  prepare: "Prepare and open",
  opening: (level: string) => `Godot is opening ${level}.`,
  notGodot: (why: string) => `That is not a Godot 4 the editor can use: ${why}`,
  packLine: "Turns what you saved into level data the game reads.",
  packEarly: "Not yet for this game. Texture and model packs work.",
  packReading: "Reading your levels…",
  pack: "Pack",
  openFolder: "Open folder",
  packStopped: "Packing stopped. The activity log says why.",
  packed: "Packed.",
  packedSome: (edits: number, levels: number) =>
    `Packed ${edits} changed object${edits === 1 ? "" : "s"} in ${levels} level${levels === 1 ? "" : "s"}. Turn on Mods under Play to try them.`,
  packedNone: "Nothing to pack: your levels are as on the disc.",
  docs: "Editor documentation",
  projects: "Level projects",
  packedFolder: "Packed levels",
} as const;

// ---- Sentences with a name or a number in them ----

export const modsAria = (label: string) => `Mods in ${label}`;
export const useAria = (name: string) => `Use ${name}`;
export const settingsAria = (name: string) => `Open ${name}`;
export const moreAria = (name: string) => `More for ${name}`;
export const fromPublisher = (label: string, owner: string) =>
  owner ? { before: "In ", label, after: `, published by ${owner}` } : { before: "In ", label, after: "" };
export const installedAlready = (version: string) => `${version} installed`;
export const updateTo = (version: string) => `Update to ${version}`;
export const fromSource = (label: string) => `From ${label}`;
export const keptAt = (version: string) => `kept at ${version}`;
export const cannotSwitchOn = (why: string) => `Cannot be switched on: ${why}`;
export const removeConfirm = (name: string) => `Remove ${name}? The launcher's copy and its settings are deleted.`;
export const withName = (name: string, why: string) => `${name}: ${why}`;

/** The prompt when the launcher starts and the day's look found newer mods (modUpdates). */
export const START_UPDATES = {
  title: "Your mods have updates",
  lead: (title: string, count: number) =>
    `${count} ${count === 1 ? "mod" : "mods"} of ${title} ${count === 1 ? "has" : "have"} a newer version.`,
  kept: "Each mod keeps its place in the load order, its switch and its settings.",
  later: "Later",
  never: "Don't ask again",
  all: (count: number) => (count === 1 ? "Update it" : "Update all"),
  working: "Updating…",
  failed: (why: string) => `The update stopped: ${why}. Nothing was changed; the Mods tab can try again.`,
  neverHint: "The Mods tab's \u201cCheck now\u201d still looks whenever you ask.",
};

/** What the index's look found for the mods it lists (lib/modAutoUpdate's checkIndex). */
export interface IndexFound {
  /** Went in by themselves. */
  updated: number;
  /** Wait for a click under "Update available". */
  waiting: number;
  /** Cannot go in now (a newer launcher, another mod): under "Waiting". */
  held: number;
}

const plural = (n: number, one: string, many: string) => `${n} ${n === 1 ? one : many}`;

/**
 * The update check's answer: the updates of the mods from a link (`count`, with the sources it
 * could not read) and, when the index was looked at too, what it found for the mods it lists.
 */
export function updatesFound(count: number, errors: string[], index: IndexFound | null = null): string {
  const said: string[] = [];
  if (index?.updated) said.push(`Updated ${plural(index.updated, "mod", "mods")}; their settings are kept.`);
  if (index?.waiting) said.push(`${plural(index.waiting, "update is", "updates are")} available below.`);
  if (index?.held) said.push(`${plural(index.held, "update waits", "updates wait")}: see below for why.`);
  if (count > 0) said.push(`${plural(count, "update", "updates")} found.`);
  if (said.length === 0) said.push(index ? "Every mod is up to date." : "Every mod from a link is up to date.");
  return said.join(" ") + (errors.length ? ` Not checked: ${errors.join("; ")}` : "");
}

export function updatedAll(done: number, failed: number): string {
  const ok = `Updated ${done} ${done === 1 ? "mod" : "mods"}; their settings are kept.`;
  return failed ? `${ok} ${failed} could not be updated: open each for why.` : ok;
}
export const updated = (name: string, version: string) => `Updated ${name} to ${version}; its settings are kept.`;

/** A mod installed from a link: replaced, on, or off (its row says why). */
export const installedFrom = {
  replaced: (name: string, version: string, from: string) =>
    `Installed ${name} ${version} from ${from}; its settings are kept.`,
  on: (name: string, from: string) => `Installed ${name} from ${from}. It is on from the game's next start.`,
  off: (name: string, from: string) => `Installed ${name} from ${from}. It is off: its row says why.`,
};

/** A mod added from a .zip or a folder (lib/addMods.ts). */
export const added = {
  replaced: (name: string, version: string) => updated(name, version),
  on: (name: string) => `Added ${name}. It is on from the game's next start.`,
  off: (name: string) => `Added ${name}. It is off: its row says why.`,
  pack: (name: string) => `Added ${name}.`,
  several: (count: number) => `Added or updated ${count} mods; their settings are kept.`,
  packHereNot: (name: string) => `${name} is a texture and model pack, which cannot be added here yet.`,
  packFailed: (name: string, why: string) => `${name}: ${why.length > 0 ? why : "it could not be added"}`,
};

/** Two mods on one hook point (lib/plugins.ts, sharedWith). */
export function sharesWith(names: string[]): string {
  if (names.length === 0) return "";
  const list = names.length === 1 ? names[0] : `${names.slice(0, -1).join(", ")} and ${names.at(-1)}`;
  return `Changes the same part of the game as ${list}: the one lower in the list goes first.`;
}

/** A number setting's range on its line: "50 to 110". */
export function rangeText(min?: number, max?: number): string {
  if (min !== undefined && max !== undefined) return `${min} to ${max}`;
  if (min !== undefined) return `${min} or more`;
  if (max !== undefined) return `up to ${max}`;
  return "";
}

/** The crash notice, when gameplay mods were on at the start that crashed (CrashNotice.svelte). */
export const crashModsLine = (ids: string[]) =>
  `Gameplay mods were on (${ids.join(", ")}). If it crashes again, turn off Mods under Play, and tell us which mods you had.`;

/** Models and textures (AssetsPanel.svelte). */
export const ASSETS = {
  lead: "Export a level's models and textures, edit them in Blender or an image editor, then import them back as a pack.",
  howItWorks: "How it works",
  exportTitle: "Export",
  exportLine: "Opens a folder of glTF models and PNG textures.",
  exporting: "Writing the files…",
  readFirst: "Read the levels from your disc first: a few minutes, once.",
  readyFirst: "Get the levels ready first, on the game's page.",
  levelLabel: "Level to export",
  whatLabel: "What to export",
  read: "Read the levels",
  export: "Export",
  openFolder: "Open folder",
  importTitle: "Import your edits",
  importLine: "Choose the folder you edited: your changes become a pack in My mods.",
  importing: "Checking what you changed…",
  import: "Import…",
  pickImport: "Choose the export folder you edited",
  imported: "Imported: your edits are a pack in My mods, switched on.",
  importedRefused: (n: number) =>
    `Imported, but ${n} change${n === 1 ? " was" : "s were"} refused: the activity log says why and what to do.`,
  importStopped: "The import stopped. The activity log has what it said.",
  exportStopped: "The export stopped. The activity log has what it said.",
  readStopped: "Reading the levels stopped. The activity log has what it said.",
  exported: (level: string) => `Exported ${level}: the folder is opening.`,
} as const;

/** Mod codes (ModSetShare.svelte, ModSetReview.svelte, ModSetSaved.svelte, core/src/modset.rs): friends
 *  matching their mods. The words never name a disc's region, and "code" is always a mod code here. */
export const MOD_CODES = {
  share: "Share these mods",
  shareHint: "Make a code friends can enter to get the same mods",
  shareTitle: "Share your mods",
  shareLead: "Friends enter the code on their Mods page and get these mods, at these versions, with these settings.",
  nothing: "No mods are on, so there's nothing to share yet.",
  inCode: "In the code",
  suggest: "Also suggest",
  suggestHint: "Controls and looks are each player's own. Friends choose whether to take these.",
  byHand: "Friends need the file",
  byHandHint: "These aren't in a catalogue, so the code can only name them. Send the .zip yourself.",
  solo: "Can't be played in a party",
  soloHint: "A party refuses these. Turn them off before you host.",
  make: "Make code",
  making: "Making the code…",
  close: "Close",
  cancel: "Cancel",
  codeLabel: "Mod code",
  copyCode: "Copy code",
  copyFull: "Copy full code",
  copied: "Copied",
  copyRefused: "The system wouldn't let the launcher copy that. Select the text and copy it yourself.",
  codeLife: "It lasts 30 days from its last use. The full code never runs out and works offline.",
  offline: "The server can't be reached, so there's no short code this time.",
  offlineFull: "Send this instead. It holds everything and works without a server:",
  reading: "Reading the code…",
  madeOn: (date: string) => `Made ${date}`,
  codeAndDate: (code: string, date: string) => `Code ${code} · made ${date}`,
  whatChanges: "What changes",
  group: {
    install: "New",
    update: "Updated",
    downgrade: "Moved to an older version",
    reinstall: "Replaced with the catalogue's copy",
    enable: "Turned on",
    disable: "Turned off",
    settings: "Settings",
    optional: "Suggested, your choice",
    unavailable: "Can't be added",
    manual: "Get these by hand",
    packs: "Also needed",
  },
  packsHint: "Texture and model packs can't come through a code. Ask your friend for these.",
  manualHint: "These aren't in a catalogue. Ask your friend for the .zip.",
  nearest: (version: string) => `The newest there is: ${version}. Ask your friend to update.`,
  matches: (count: number) => (count === 1 ? "1 mod already matches." : `${count} mods already match.`),
  trustTitle: "This runs code on your computer",
  trustLine: (label: string) => `Mods from ${label} aren't made by OpenRAC, and you haven't used them before.`,
  trust: (label: string) => `I trust ${label}`,
  fresh: "Published in the last two days",
  otherGame: "This code is for another game.",
  noGame: "Set up a game first, then enter the code again.",
  queued: "Another code came in: it opens when this one is closed.",
  otherOpenrac: (version: string) =>
    `It was made with OpenRAC ${version}. A party needs the same OpenRAC on every side.`,
  partial: "Part of this can't be applied. The rest can, but the party may still refuse you.",
  running: "Close the game first: mods load when it starts.",
  saved: "Your mods now are saved first, so you can go back.",
  already: "You already match this code. There's nothing to change.",
  apply: "Apply",
  applying: "Applying…",
  doneTitle: "Your mods match",
  done: "Start the game and join the party.",
  doneSkipped: (count: number) =>
    count === 1
      ? "1 mod is still missing, so the party may refuse you."
      : `${count} mods are still missing, so the party may refuse you.`,
  undo: "Put my old mods back",
  playAndJoin: (party: string) => `Play and join ${party}`,
  undone: "Your old mods are back.",
  savedTitle: "Earlier mods",
  savedHint: "Saved each time a code was applied. The last ten are kept.",
  savedRow: (code: string, date: string) => (code ? `Before ${code}, ${date}` : `Before a full code, ${date}`),
  restore: "Restore",
  restoreConfirm: "Put these mods back? Mods added since stay installed, turned off.",
} as const;

/** Audio (AudioPanel.svelte). */
export const AUDIO = {
  lead: "Export the game's sounds as WAV files, edit or replace them, then import them back as a pack.",
  howItWorks: "How it works",
  exportTitle: "Export",
  exportLine: "Opens a folder of WAV files.",
  exporting: "Writing the files…",
  readyFirst: "Set the game up from your disc first, on its page.",
  levelLabel: "Level to export",
  allLevels: "The whole game",
  whatLabel: "What to export",
  what: { effects: "Effects", music: "Music", speech: "Speech", all: "Everything" },
  /** How much the whole game's takes, measured on both discs. */
  large: {
    music: "All the music is about 475 MB.",
    speech: "All the speech is about 1.6 GB.",
    all: "Everything is about 2.1 GB.",
  },
  export: "Export",
  openFolder: "Open folder",
  exportStopped: "The export stopped. The activity log has what it said.",
  exported: "Exported: the folder is opening.",
  importTitle: "Import your edits",
  importLine: "Choose the folder you edited: the files you changed become a pack in My mods.",
  importing: "Checking what you changed…",
  import: "Import…",
  pickImport: "Choose the audio export you edited",
  imported: "Imported: your sounds are a pack in My mods.",
  importedRefused: (n: number) =>
    `Imported: your sounds are a pack in My mods. ${n} file${n === 1 ? " was" : "s were"} refused.`,
  importRefused: (n: number) => `Nothing imported: ${n} file${n === 1 ? " was" : "s were"} refused.`,
  openActivityLog: "Open activity log",
  nothing: "No file in that folder differs from what was exported, so no pack is left from it.",
} as const;

// ---- The mod hub: the change set (DeepLinkInstall.svelte), opened by a link from openrac.dev ---
// Each message says what happened, why if known, and what to do, in two short sentences at most,
// with no code in it (core/src/mod_error.rs gives the kind; `detail` stays in "Copy details").

import type { ModError, PlanNote, Why } from "./api";

const list = (names: string[]) =>
  names.length <= 1 ? (names[0] ?? "") : `${names.slice(0, -1).join(", ")} and ${names[names.length - 1]}`;

export const CHANGE_SET = {
  title: "Install mods",
  loading: "Reading the mod list…",
  picked: "You picked",
  alsoInstalls: "Also installs",
  alreadyHave: "Already installed",
  recommended: "Recommended",
  suggested: "You might also like",
  cancel: "Cancel",
  backToMods: "Back to mods",
  play: "Play ▸",
  switchToDev: "Switch to the dev build",
  retry: "Try again",
  useCached: "Use the saved list",
  report: "Report a problem",
  copyDetails: "Copy details",
  working: "Installing…",
  closeAria: "Close",
  backedUp: "Your saves were backed up first.",
  blocked: "Fix what's above to install.",
} as const;

export const installCount = (n: number) => (n === 1 ? "Install 1 mod" : `Install ${n} mods`);
export const alsoCount = (n: number) => (n === 1 ? "and 1 it needs" : `and ${n} they need`);
export const updateFrom = (from: string, to: string) => `${from} → ${to}`;
export const neededBy = (names: string[]) => `Needed by ${list(names)}`;
export const offeredBy = (name: string) => `Suggested by ${name}`;

/** The one native-code warning of a change set, naming the mods. */
export function nativeWarning(names: string[]): string {
  const verb = names.length === 1 ? "runs" : "run";
  return `${list(names)} ${verb} code on this computer, outside the game's sandbox. Install only mods you trust.`;
}

export const STEP_LABEL = {
  waiting: { text: "Waiting", tone: "off" },
  installing: { text: "Installing", tone: "warn" },
  done: { text: "Installed", tone: "ok" },
  failed: { text: "Failed", tone: "err" },
} as const satisfies Record<string, Label>;

export function noteText(n: PlanNote): string {
  switch (n.kind) {
    case "olderForApi":
      return `${n.name} ${n.newest} needs the dev build (mod API ${n.api}). Installing ${n.picked}, the latest for your release.`;
    case "pack":
      return `${n.name} is a set: it installs ${n.mods.length} mods.`;
    case "online":
      return `${n.name} plays online: friends need the same mods to play with you.`;
    case "touchesSaves":
      return `${n.name} changes save files. Your saves are backed up before it's first turned on.`;
    case "revokedSkipped":
      return `${n.name} ${n.version} was withdrawn (${n.reason}). An earlier version is installed.`;
  }
}

/** Why no version of a mod can be installed. */
export function whyText(name: string, why: Why): string {
  switch (why.kind) {
    case "notForGame":
      return `${name} isn't made for this game.`;
    case "notForDisc":
      return `${name} doesn't work with this disc (${why.disc.toUpperCase()}).`;
    case "notForPlatform":
      return `${name} isn't built for this system yet.`;
    case "needsNewerApi":
      return `${name} needs a newer launcher (mod API ${why.api}).`;
    case "clash": {
      const by = why.wants.map((w) => `${w.by || "you"} ${w.version ?? "any"}`);
      return `Can't install ${name}: no version fits ${list(by)}.`;
    }
    case "cycle":
      return `${list(why.ids)} need each other, so none can install first.`;
    case "revoked":
      return `${name} was withdrawn: ${why.reason}`;
    case "onlyPrerelease":
      return `${name} has only test versions. Turn on Pre-release mods in Settings to get it.`;
  }
}

export const missingText = (id: string, by: string | null) =>
  by ? `${by} needs ${id}, which isn't in the mod list.` : `${id} isn't in the mod list.`;

export const conflictText = (name: string, withName: string, reason: string) =>
  `${name} can't be on with ${withName}${reason ? `: ${reason}` : ""}.`;
export const replaceWith = (withName: string, name: string) => `Replace ${withName} with ${name}`;

/** A mod hub error's headline, by its kind: never its detail. */
export function modErrorText(e: ModError): string {
  switch (e.kind) {
    case "transient":
      return "Couldn't reach the mod list. Check your connection and try again.";
    case "upstreamBad":
      return e.modId
        ? `${e.modId} couldn't be verified, so it wasn't installed. Nothing else changed.`
        : "The mod list couldn't be verified, so it isn't used.";
    case "userFixable":
      return e.detail;
    case "notFound":
      return e.modId ? `${e.modId} isn't there any more.` : "That mod isn't in the mod list.";
    case "bug":
      return "Something went wrong in the launcher. Copy the details and tell us.";
  }
}

export function appliedText(done: number, total: number, failed: string | null): string {
  if (failed) return `Installed ${done} of ${total}. ${failed} failed; the others are on.`;
  return done === 1 ? "Installed 1 mod. It's on." : `Installed ${done} mods. They're on.`;
}

// ---- Mods kept current and tidy (lib/modAutoUpdate, ModUpdatesPanel.svelte, PluginsPanel) -----

import type { Hold } from "./api";

export const updatedMods = (n: number) =>
  n === 1 ? "Updated 1 mod. See what changed on the Mods tab." : `Updated ${n} mods. See what changed on the Mods tab.`;

/** A withdrawn version the look for updates turned off (ModUpdatesPanel). */
export const revokedOffText = (name: string, version: string, reason: string) =>
  `${name} ${version} was turned off: ${reason}`;

export const UPDATES = {
  title: "Mod updates",
  waitingTitle: "Update available",
  heldTitle: "Waiting",
  recentTitle: "Just updated",
  changelog: "What changed",
  update: "Update",
  updating: "Updating…",
  noChangelog: "No notes for this version.",
  running: "Close the game to update mods.",
} as const;

/** Why an update waits or is held, in a line. */
export function holdText(hold: Hold | null): string {
  if (!hold) return "";
  switch (hold.kind) {
    case "major":
      return "A new major version: it may change how the game plays or your saves.";
    case "newHooks":
      return `It now changes more of the game (${hold.hooks.length} new ${hold.hooks.length === 1 ? "hook" : "hooks"}).`;
    case "heldBack":
      return `Held back: ${hold.byName} needs ${hold.wants}.`;
    case "needs":
      return `Held back: it needs ${hold.id}${hold.wants ? ` ${hold.wants}` : ""}, which can't update with it.`;
    case "needsLauncher":
      return `Needs a newer launcher (mod API ${hold.api}). It updates once the launcher does.`;
    case "blocked":
      return "Held back: it doesn't fit with your other mods yet.";
  }
}

export const DEPS = {
  shareSetup: "Share my setup",
  shareCopied: "Link copied. Friends can install the same mods from it.",
  shareNone: "No mods from the mod list are on to share.",
  shareRefused: "The link couldn't be copied.",
  turnOffYes: "Turn off",
  removeYes: "Remove",
  keep: "Keep them",
} as const;

export const alsoTurnedOn = (names: string[]) => `Also turned on ${list(names)}.`;
export const turnOffAsk = (needers: string[], name: string) =>
  `${list(needers)} ${needers.length === 1 ? "needs" : "need"} ${name}. Turn off ${needers.length === 1 ? "it" : "them"} too?`;
export const fixInstall = (id: string) => `Fix: install ${id}`;
export const removeDepsAsk = (names: string[]) =>
  `Also remove ${list(names)}? Nothing else uses ${names.length === 1 ? "it" : "them"}.`;
export const removedDeps = (n: number) => (n === 1 ? "Also removed 1 unused mod." : `Also removed ${n} unused mods.`);
export const pinnedShared = "kept at this version by a shared setup";
export const pinnedRollback = "kept at this version after a crash";
export const rollBackTo = (name: string, version: string) => `Roll back ${name} to ${version}`;
export const crashAfterUpdate = (names: string[]) =>
  `This came after ${list(names)} updated. Rolling back keeps the earlier version until a fixed one is out.`;
export const rolledBack = (name: string, version: string) => `${name} is back at ${version}.`;

/** Settings: the launcher in Steam's library (AddToSteam.svelte). */
export const STEAM_TEXT = {
  title: "Steam",
  line: "Add OpenRAC to your Steam library, with its name and pictures.",
  inLibrary: "In your Steam library.",
  noUser: "Sign in to Steam once to add OpenRAC to it.",
  flatpak: "Flatpak Steam can't start OpenRAC. Use Steam's normal package to add it.",
  add: "Add to Steam",
  update: "Update in Steam",
  inSteam: "In Steam",
  added: "Added. If Steam is open and does not show it, restart Steam.",
} as const;

/** A game's page: its own Steam entry and desktop shortcut (GameShortcuts.svelte). */
export const SHORTCUT_TEXT = {
  steamLine: "Add this game to your Steam library as its own entry. It starts without the launcher's window.",
  steamIn: "In your Steam library.",
  desktopTitle: "Desktop shortcut",
  desktopLine: "A shortcut on your Desktop that starts this game without the launcher's window.",
  create: "Create shortcut",
  created: (path: string) => `Shortcut written to ${path}.`,
} as const;

export const SETTINGS_TEXT = {
  autoUpdate: "Update mods automatically",
  autoUpdateLine: "Small updates go in by themselves before you play. Big ones wait for you.",
  cleanUp: "Clean up unused dependencies",
  cleanUpLine: "Removing a mod also removes what was installed only for it.",
} as const;

/** A mod's own page (ModTool.svelte). */
export const MOD_TOOL = {
  frameLabel: (name: string) => `${name}'s page`,
  backTo: (name: string) => `Back to ${name}`,
  failed: (why: string) => `The page asked for something the launcher could not do: ${why}`,
  onlyInApp: "A mod's page opens in the launcher app, not in this preview.",
};

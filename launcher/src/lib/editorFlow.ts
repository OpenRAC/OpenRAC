// The level editor's status panel (components/EditorFlow.svelte): where the player is, in one
// sentence, and the one step that comes next as the panel's main button. A plain function of what
// the core found in the editor's folders (core/src/editor_state.rs), the disc, Godot and the jobs
// running, so that every state can be tested without a window.
//
//   no disc → no Godot → no levels yet → reading them → ready to edit → changes to pack → packing
//   → packed, ready to play;  and, whenever the core finds it, needs an update / files missing.
import type { Carried, EditorState, OutdatedReason } from "./api";
import { levelName } from "./labels";
import type { Job } from "./queue";

export type StageKind =
  | "loading"
  | "no-disc"
  | "no-godot"
  | "no-levels"
  | "extracting"
  | "updating"
  | "packing"
  | "needs-update"
  | "broken"
  | "unpacked"
  | "repack"
  | "packed"
  | "ready";

/** What a button of the panel does. */
export type Step = "set-up" | "get-godot" | "prepare" | "open" | "update" | "repair" | "pack" | "play";

export interface Stage {
  kind: StageKind;
  tone: "ok" | "warn" | "info" | "busy";
  title: string;
  line: string;
  /** What happens when the player takes the main step, said before they do (an update). */
  detail?: string;
  primary: Step | null;
  secondary: Step[];
  /** The levels the stage is about (outdated, missing files, to pack). */
  levels: number[];
}

export interface FlowInput {
  game: string;
  state: EditorState | null;
  discFound: boolean;
  lacksGodot: boolean;
  /** The game's levels are not packed back yet (early access): no pack or play steps. */
  early: boolean;
  extracting: Job | null;
  packing: Job | null;
}

/** The buttons' words. */
export const STEP_LABEL: Record<Step, string> = {
  "set-up": "Set up the disc",
  "get-godot": "Get Godot",
  prepare: "Read the levels and open",
  open: "Open in Godot",
  update: "Update project",
  repair: "Repair",
  pack: "Pack your changes",
  play: "Play with your edits",
};

/** Level names in a sentence: "Veldin, Novalis and 3 more". */
export function levelList(game: string, levels: number[], most = 3): string {
  const names = levels.map((n) => levelName(game, n));
  if (names.length <= most) {
    return names.length <= 1 ? (names[0] ?? "") : `${names.slice(0, -1).join(", ")} and ${names.at(-1)}`;
  }
  return `${names.slice(0, most).join(", ")} and ${names.length - most} more`;
}

const plural = (n: number, one: string, many = `${one}s`) => `${n} ${n === 1 ? one : many}`;

/** Why levels need updating, in a player's words: the first reason found says it. */
export const OUTDATED_LINE: Record<OutdatedReason, string> = {
  gameplay:
    "The editor now builds PAL levels from the 60 Hz version of the game, the one OpenRAC plays. Yours come from the 50 Hz version, so they cannot be packed as they are.",
  schema:
    "This editor puts more into each level than the one that made yours, so they need reading again before they can be packed.",
  version: "They were read from another version of the game's disc than the one set up now.",
};

/** Said right above the Update button: what it does, and that nothing is lost. */
export const UPDATE_DETAIL =
  "Your changes are not lost. Update project keeps a backup of your levels, reads fresh ones from your disc and carries over the objects you moved, changed, added or deleted; anything it cannot carry is listed afterwards. About 20 seconds.";

/** The Open button while levels need an update, and what it is good for. */
export const OPEN_ANYWAY = "Open anyway";
export const OPEN_ANYWAY_HINT = "Opening an outdated level works for looking around. Pack it after the update.";

/** Where the player is, and what comes next. */
export function editorStage(input: FlowInput): Stage {
  const { game, state, early } = input;
  const base = { secondary: [] as Step[], levels: [] as number[] };
  if (!input.discFound) {
    return {
      ...base,
      kind: "no-disc",
      tone: "warn",
      title: "Set the game up first",
      line: "The editor reads the levels from your disc.",
      primary: "set-up",
    };
  }
  if (input.extracting) {
    const updating = (state?.outdated.length ?? 0) > 0 || (state?.broken.length ?? 0) > 0;
    return updating
      ? {
          ...base,
          kind: "updating",
          tone: "busy",
          title: "Updating your levels…",
          line: "Backing up your levels, reading fresh ones from your disc and carrying your changes over.",
          primary: null,
        }
      : {
          ...base,
          kind: "extracting",
          tone: "busy",
          title: "Reading the levels from your disc…",
          line: "This takes about 20 seconds, once.",
          primary: null,
        };
  }
  if (input.packing) {
    return {
      ...base,
      kind: "packing",
      tone: "busy",
      title: "Packing your changes…",
      line: "Turning what you saved in Godot into level data the game reads.",
      primary: null,
    };
  }
  if (!state) {
    return { ...base, kind: "loading", tone: "info", title: "Looking at your levels…", line: "", primary: null };
  }
  const opening: Step[] = input.lacksGodot ? ["get-godot"] : ["open"];
  if (state.outdated.length) {
    const levels = state.outdated.map((o) => o.level);
    const reason = state.outdated[0]?.reason ?? "schema";
    return {
      kind: "needs-update",
      tone: "warn",
      title:
        levels.length === state.levels.length ? "Your levels need an update" : "Some of your levels need an update",
      line: (levels.length === state.levels.length ? "" : `${levelList(game, levels)}. `) + OUTDATED_LINE[reason],
      detail: UPDATE_DETAIL,
      primary: "update",
      secondary: opening,
      levels,
    };
  }
  if (state.broken.length) {
    return {
      kind: "broken",
      tone: "warn",
      title: "Some level files are missing",
      line: `${levelList(game, state.broken)}: some of ${state.broken.length === 1 ? "its" : "their"} files are missing. Repair keeps a backup and reads ${state.broken.length === 1 ? "it" : "them"} again from your disc.`,
      primary: "repair",
      secondary: opening,
      levels: state.broken,
    };
  }
  if (!early && state.unpacked.length) {
    return {
      kind: "unpacked",
      tone: "info",
      title: "You have changes to pack",
      line: `Saved in Godot and not packed yet: ${levelList(game, state.unpacked)}. Pack them so the game plays them.`,
      primary: "pack",
      secondary: opening,
      levels: state.unpacked,
    };
  }
  if (input.lacksGodot) {
    return {
      ...base,
      kind: "no-godot",
      tone: "warn",
      title: "Get Godot to edit levels",
      line: "Levels open in Godot 4, a free editor. Get it, then choose where it is.",
      primary: "get-godot",
    };
  }
  if (!state.levels.length) {
    return {
      ...base,
      kind: "no-levels",
      tone: "info",
      title: "No levels to edit yet",
      line: early
        ? "The first time, the editor reads the levels from your disc (a few minutes), then opens the one you pick in Godot."
        : "The first time, the editor reads the levels from your disc (about 20 seconds), then opens the one you pick in Godot.",
      primary: "prepare",
    };
  }
  if (!early && state.packedAt !== null && state.packedOtherVersion) {
    return {
      ...base,
      kind: "repack",
      tone: "warn",
      title: "Pack your changes again",
      line: "Your packed levels were made from another version of the game. Pack again so they match the disc set up now.",
      primary: "pack",
      secondary: ["open"],
    };
  }
  if (!early && state.packedAt !== null && state.packedLevels > 0) {
    return {
      ...base,
      kind: "packed",
      tone: "ok",
      title: "Your changes are packed",
      line: `${plural(state.packedLevels, "level")} changed. Play with your edits starts the game with them; you can turn them off again in Mods, under Play.`,
      primary: "play",
      secondary: ["open"],
    };
  }
  return {
    ...base,
    kind: "ready",
    tone: "ok",
    title: "Ready to edit",
    line: early
      ? "Pick a level and look around its objects and textures in Godot."
      : "Pick a level and open it in Godot. Move, turn or scale objects and save, then come back here to pack your changes.",
    primary: "open",
  };
}

/** What the last update did, for the note under the status: carried and to redo, per level. */
export interface UpdateNote {
  carried: number;
  /** Levels whose changes could not be carried at all. */
  failed: number[];
  /** What to redo by hand, "Kerwan: Camera_0001: moved". */
  redo: string[];
  levels: number;
}

export function updateNote(game: string, carried: Carried[]): UpdateNote | null {
  if (!carried.length) return null;
  return {
    carried: carried.reduce((n, c) => n + c.carried.length, 0),
    failed: carried.filter((c) => c.failed).map((c) => c.level),
    redo: carried.flatMap((c) => c.notCarried.map((line) => `${levelName(game, c.level)}: ${line}`)),
    levels: carried.length,
  };
}

/** The note's sentence. */
export function updateNoteLine(note: UpdateNote): string {
  const head =
    note.carried === 0
      ? `${plural(note.levels, "level")} updated. You had no changes to carry over.`
      : `${plural(note.levels, "level")} updated, with ${plural(note.carried, "change")} of yours carried over.`;
  const redo = note.redo.length
    ? ` ${plural(note.redo.length, "change")} could not be carried: redo ${note.redo.length === 1 ? "it" : "them"} in Godot.`
    : "";
  return head + redo + " Your old levels are in the backup folder.";
}

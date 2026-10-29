// The Level editor tab's "Start over" (components/EditorReset.svelte): what each reset lists, what
// it removes with the player's choice, when it cannot run, and what a failure says to a player. The
// folders themselves are chosen and checked by the Rust core (core/src/reset.rs); the page only
// shows them, and sends the version and the choice, never a path.
import type { ResetDone, ResetKind, ResetPart, ResetPartView, ResetPreview } from "./api";
import { size } from "./format";
import type { Job } from "./queue";

/** Each part's name and the one line under it, in the confirmation. */
export const PART_TEXT: Record<ResetPart, { title: string; line: string }> = {
  gameLevels: {
    title: "The game's copy of the levels",
    line: "Read from your disc again straight after. Its enhanced lighting is made again too.",
  },
  projects: { title: "Levels opened in Godot", line: "Changes you saved in Godot but did not pack are lost." },
  exports: { title: "Models and textures you exported", line: "Files you changed there but did not import are lost." },
  packed: { title: "Your packed levels", line: "The level changes the game plays with Mods on." },
  overrides: { title: "Your imported edits and added packs", line: "For every disc of this game." },
  backups: { title: "Older copies of your levels", line: "Kept aside when an editor update replaced them." },
};

export const RESET = {
  title: "Start over",
  lead: "When levels look wrong, or for a clean editor. Your disc, saves and gameplay mods are never touched.",
  levelsTitle: "Read the levels again",
  levelsLine: "Reads the game's levels from your disc again. Your edits and packs are kept.",
  levelsButton: "Read again…",
  editorTitle: "Reset the level editor",
  editorLine: "Clears the levels opened in Godot and the models and textures you exported.",
  editorButton: "Reset…",
  busy: "Wait for the game and the tasks running to end first.",
  noDisc: "Set the game up from your disc first.",
  checking: "Looking at what is there…",
  nothing: "There is nothing to remove.",
  closeGodot: "Close Godot first if a level is open in it.",
  removes: "To delete",
  inAll: (bytes: string) => `In all: ${bytes}`,
  keeps: "Kept",
  withEdits: "Also delete my own edits and packs",
  cancel: "Cancel",
  confirmLevels: "Read the levels again",
  confirmEditor: "Delete",
  working: "Deleting…",
  previewFailed: "The launcher could not look in the editor's folders. Check the install folder in Settings.",
  refused: "Nothing was deleted: one of these folders is not one of OpenRAC's own, so it was left alone.",
  failed: "Some files could not be deleted. Close Godot and any window showing them, then try again.",
  levelsStopped: "Reading the levels stopped. The activity log says why.",
  levelsDone: "The levels were read again from your disc.",
} as const;

/** What a reset keeps, in a line under its list: so that nobody wonders about their own work. */
export function keptLine(kind: ResetKind, withEdits: boolean): string {
  if (kind === "levels") {
    return "Your levels in Godot, your packs and edits, the models and textures you exported, and your saves.";
  }
  return withEdits
    ? "The game's copy of the levels, your saves and your gameplay mods."
    : "Your packed levels, imported edits and added packs, the game's copy of the levels, and your saves.";
}

/** The parts a reset removes with the player's choice: their own only when they ticked it. */
export function chosenParts(preview: ResetPreview, withEdits: boolean): ResetPartView[] {
  return preview.parts.filter((p) => withEdits || !p.theirs);
}

/** The parts the tick box adds, for the line beside it. */
export const theirParts = (preview: ResetPreview): ResetPartView[] => preview.parts.filter((p) => p.theirs);

/** All of them together. */
export function total(parts: ResetPartView[]): { bytes: number; files: number } {
  return parts.reduce((t, p) => ({ bytes: t.bytes + p.bytes, files: t.files + p.files }), { bytes: 0, files: 0 });
}

/** A part's size as the list shows it: "1,2 GB", "empty" for a folder with nothing in it, or
 *  "not there". */
export function partSize(part: ResetPartView): string {
  if (part.folders.length === 0) return "not there";
  return part.files === 0 ? "empty" : size(part.bytes);
}

/** There is something to remove: a folder of a chosen part is there. */
export const anything = (parts: ResetPartView[]): boolean => parts.some((p) => p.folders.length > 0);

/** Why the resets cannot run now, or null: never while a job runs (the game, an extraction writing
 *  these folders) or the game is running, nor without the disc for reading the levels again. */
export function blockedBy(jobs: Job[], gameRunning: boolean): string | null {
  const live = jobs.some((j) => j.state === "queued" || j.state === "running");
  return live || gameRunning ? RESET.busy : null;
}

/** A failure in a player's words: what the core said is for the log, never shown. */
export function failureText(error: unknown, previewing: boolean): string {
  const text = String(error);
  if (text.includes("busy:")) return RESET.busy;
  if (text.includes("left alone")) return RESET.refused;
  return previewing ? RESET.previewFailed : RESET.failed;
}

/** What the editor's reset says when it is done. */
export function editorDone(done: ResetDone): string {
  if (done.folders === 0) return "There was nothing to delete. Open a level to read it from your disc.";
  return `The level editor was reset (${size(done.bytes)} freed). Open a level to read fresh copies from your disc.`;
}

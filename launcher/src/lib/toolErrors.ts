// What a player reads when one of the editor's tools stops: every failed job of the level editor,
// the asset export and the packs goes through `toolFailure`, and nothing else on the page shows a
// tool's own words. The tools say why they stopped as a code on a line of its own
// (`openrac-error: project-outdated-gameplay`, editor/errors.py); each code has a sentence here in
// a player's words and the one step that helps. A job without a code, or with one this launcher
// does not know, gets the generic sentence. What the tool printed stays in the job's log (Activity)
// and behind "Show details", its folders cut to their names (lib/paths.ts).
//
// editor/test_errors.py fails when a tool raises a code that is not in TOOL_ERRORS.
import { hidePaths } from "./paths";
import { PROGRESS_LINE } from "./progress";
import type { Line } from "./queue";

/** The line a tool says its code on (editor/errors.py LINE). */
export const ERROR_LINE = "openrac-error: ";

/** The one thing that helps, offered as the error's button. */
export type NextStep =
  | "update" // Update project: back up, read again, carry the edits over
  | "retry" // the same step again, once the player fixed what is said
  | "start-over" // the Your levels section: export again, delete
  | "set-up" // set the disc up again
  | "open" // open a level in Godot (it reads the levels first)
  | "settings" // the install folder, in Settings
  | "report" // something OpenRAC should hear about: Activity has what to send
  | "none";

export interface ToolError {
  title: string;
  line: string;
  next: NextStep;
}

export const TOOL_ERRORS: Record<string, ToolError> = {
  "project-outdated-gameplay": {
    title: "Your levels need an update",
    line: "They were made before the editor moved to the 60 Hz version of the game that OpenRAC plays, so they cannot be packed as they are. Updating keeps a backup and carries your changes over.",
    next: "update",
  },
  "project-outdated": {
    title: "Your levels need an update",
    line: "They were made by an older version of the editor. Updating keeps a backup and carries your changes over.",
    next: "update",
  },
  "static-import": {
    title: "An imported model could not be packed",
    line: "Check where the models you imported into this level sit and their collision, save in Godot, then pack again.",
    next: "retry",
  },
  "level-too-big": {
    title: "This level has grown too big",
    line: "Its objects no longer fit in the space the game keeps for them. Remove some of the objects you added, save in Godot, then pack again.",
    next: "retry",
  },
  "pack-internal": {
    title: "Packing went wrong",
    line: "The packed level did not read back as it should, so nothing was written. Please report it, with the details.",
    next: "report",
  },
  "pack-incomplete": {
    title: "Some level edits could not be packed",
    line: "Some changes in your Godot scenes could not be used in the game. Show details lists them; correct or remove those edits, then pack again.",
    next: "retry",
  },
  "folder-not-allowed": {
    title: "The editor cannot write there",
    line: "OpenRAC only writes inside its own folders. Check the install folder in Settings.",
    next: "settings",
  },
  "folder-exists": {
    title: "A folder is in the way",
    line: "Delete the level project under Your levels, then try again.",
    next: "start-over",
  },
  "folder-not-ours": {
    title: "A folder holds something else",
    line: "One of the editor's folders holds files OpenRAC did not make. Move them somewhere else, or delete the level project under Your levels.",
    next: "start-over",
  },
  "no-project": {
    title: "There are no levels to pack yet",
    line: "Open a level in Godot first: that reads the levels from your disc.",
    next: "open",
  },
  "game-not-packable": {
    title: "Packing is not ready for this game yet",
    line: "You can open its levels in Godot and make texture and model packs.",
    next: "none",
  },
  "no-such-level": {
    title: "That level is not on this disc",
    line: "Pick another level from the list.",
    next: "none",
  },
  "disc-data": {
    title: "Your disc image could not be read",
    line: "Part of it is not what OpenRAC expects. Make sure it is a full, unmodified copy of the game, then set it up again.",
    next: "set-up",
  },
  "disk-full": {
    title: "Your disk is full",
    line: "Free some space, then try again.",
    next: "retry",
  },
  "file-locked": {
    title: "A file is in use",
    line: "Close Godot and any window showing the level files, then try again.",
    next: "retry",
  },
  "file-missing": {
    title: "A file is missing",
    line: "Some of the files this step needs are gone. Export the levels again under Your levels.",
    next: "start-over",
  },
  "file-error": {
    title: "A file could not be read or written",
    line: "Try again. If it keeps happening, export the levels again under Your levels.",
    next: "retry",
  },
  "carry-failed": {
    title: "Your changes could not be carried over",
    line: "The new levels are as on your disc. Your old levels are kept in the backup, to redo your changes from.",
    next: "none",
  },
  "pack-invalid": {
    title: "That pack could not be added",
    line: "It is not a pack OpenRAC reads, or it is damaged.",
    next: "none",
  },
  "asset-invalid": {
    title: "Your edited files could not be read",
    line: "Check the models and textures you changed, then import them again.",
    next: "retry",
  },
  internal: {
    title: "Something went wrong",
    line: "OpenRAC hit a problem it did not expect. Try again; if it happens again, please report it with the details.",
    next: "report",
  },
};

/** What a failure without a known code says. */
export const UNKNOWN_ERROR: ToolError = {
  title: "That step did not finish",
  line: "Try again. If it keeps happening, Show details says what went wrong, and exporting the levels again under Your levels usually helps.",
  next: "retry",
};

/** A failed job in a player's words, with what the tool printed for "Show details". */
export interface ToolFailure extends ToolError {
  /** The tool's code, or null when it said none. */
  code: string | null;
  /** The last lines it printed, folders cut to their names: for "Show details" only. */
  details: string;
}

/** The code a tool said, the last one if more. */
export function errorCode(lines: Pick<Line, "line">[]): string | null {
  const said = lines.findLast((l) => l.line.startsWith(ERROR_LINE));
  return said ? said.line.slice(ERROR_LINE.length).trim() || null : null;
}

/** The marker lines the launcher reads, never shown even in the details. */
const MARKERS = [ERROR_LINE, "openrac-output: ", "openrac-carry: ", PROGRESS_LINE];

/** A failed job of a tool, said for a player: never the tool's own words in the sentence. */
export function toolFailure(lines: Pick<Line, "line">[], count = 15): ToolFailure {
  const code = errorCode(lines);
  const known = code !== null ? TOOL_ERRORS[code] : undefined;
  const shown = lines
    .map((l) => l.line)
    .filter((l) => !MARKERS.some((m) => l.startsWith(m)) && l.trim() !== "")
    .slice(-count);
  return { ...(known ?? UNKNOWN_ERROR), code, details: hidePaths(shown.join("\n")).text };
}

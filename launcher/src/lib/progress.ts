// What a running job tells a player: a plain name for the step it is on, and how far along it is
// when its output says so. No Svelte or Tauri here, so it can be tested. The raw output stays
// behind "Details" (JobProgress.svelte).
import type { Job, Line } from "./queue";

/** How long after it began the game still counts as "starting", in milliseconds. */
export const STARTING_MS = 10_000;

/** The steps a player sees, by the job's action id. Anything else has no plain name. */
export function phaseLabel(job: Pick<Job, "actionId" | "state" | "startedAt">, now = Date.now()): string {
  if (job.state === "queued") return "Waiting for another task to finish";
  const id = job.actionId;
  if (id.startsWith("install:")) return "Setting up from your disc";
  switch (id) {
    case "port-build":
      return "Building the game for this computer";
    case "port-levels":
      return "Reading the levels from your disc";
    case "port-levels-check":
      return "Checking the levels";
    case "probe-bake":
      return "Preparing the enhanced lighting";
    case "play":
    case "play-edited":
      return job.startedAt !== null && now - job.startedAt > STARTING_MS ? "The game is running" : "Starting the game";
    default:
      return "Working on it";
  }
}

/** The game itself is running: nothing is left to count, so no bar. */
export const isGame = (job: Pick<Job, "actionId">) => job.actionId === "play" || job.actionId === "play-edited";

export interface JobProgress {
  /** 0 to 100 when the output says how far along the job is; null otherwise (an animated bar). */
  percent: number | null;
  /** A short count in plain words when there is one but no total ("5 levels read"). */
  note: string | null;
  /** How long is left, in words, when the job says ("about 12 minutes left"). */
  left?: string | null;
}

const NINJA = /^\s*\[\s*(\d+)\s*\/\s*(\d+)\s*\]/; // "[1351/2273] Building C object ..."
const PERCENT = /(?:^|[\s[(])(\d{1,3}(?:\.\d+)?)\s?%/; // "[ 45%] Building ...", "copying 62 %"
const LEVEL = /^level \d+:/; // editor/extract.py, one line for each level it finished
// "Lighting 42.5% (about 12 minutes left)": the probe bake (bake_all.py, show_progress).
const LEFT = /\(((?:about|less than) [^)]* left)\)\s*$/;

const clamp = (n: number) => Math.min(100, Math.max(0, n));

// ---- The editor's tools: one line per step of work (editor/progress_line.py) ----

/** The line a tool says how far along it is on (editor/progress_line.py LINE). */
export const PROGRESS_LINE = "openrac-progress: ";

/** Each step a tool counts, in a player's words (editor/test_progress_line.py checks the list). */
export const STEP_WORDS: Record<string, string> = {
  read: "Reading levels",
  project: "Making the Godot project",
  carry: "Carrying your changes over",
  pack: "Packing",
  visibility: "Adding what the game can see",
  textures: "Showing your packs' textures",
  audio: "Writing the sounds",
};

export interface ToolProgress {
  /** The tool's step, as it said it ("read"). */
  step: string;
  /** The step in a player's words. */
  words: string;
  /** Done and total, when the step is counted; null for a step that is not (a bar without an end). */
  done: number | null;
  total: number | null;
  /** The level the newest item was, by number. */
  level: number | null;
}

const STEP_LINE = /^([a-z]+)(?: (\d+)\/(\d+)(?: (\d+))?)?$/;

/** One progress line read, or null for any other line (or a step this launcher does not know). */
export function parseProgress(line: string): ToolProgress | null {
  if (!line.startsWith(PROGRESS_LINE)) return null;
  const m = STEP_LINE.exec(line.slice(PROGRESS_LINE.length).trim());
  const step = m?.[1];
  if (!m || !step || !(step in STEP_WORDS)) return null;
  const n = (s: string | undefined) => (s === undefined ? null : Number(s));
  const done = n(m[2]);
  const total = n(m[3]);
  if (done !== null && total !== null && done > total) return null;
  return { step, words: STEP_WORDS[step] ?? step, done, total, level: n(m[4]) };
}

/** The newest progress line a job printed, or null when it printed none. */
export function toolProgress(lines: Pick<Line, "line">[]): ToolProgress | null {
  for (let i = lines.length - 1; i >= 0; i--) {
    const text = lines[i]?.line ?? "";
    if (text.startsWith(PROGRESS_LINE)) {
      const said = parseProgress(text);
      if (said) return said;
    }
  }
  return null;
}

/** A count's share, 0 to 100; null when the step has no count (or counts nothing). */
export const progressPercent = (p: ToolProgress | null): number | null =>
  p?.done != null && p.total ? clamp((p.done / p.total) * 100) : null;

/** When a count was seen: what a time-left estimate starts from. */
export interface Sample {
  step: string;
  done: number;
  at: number;
}

/**
 * How long is left, in words, from the first count seen of this step (FIRST) and the newest
 * (NOW): null until two items are done and a few seconds have passed, so a guess from one
 * level is never shown.
 */
export function timeLeft(first: Sample | null, now: Sample | null, total: number | null): string | null {
  if (!first || !now || total === null || first.step !== now.step) return null;
  const items = now.done - first.done;
  const spent = now.at - first.at;
  if (items < 2 || spent < 5_000 || now.done >= total) return null;
  const left = (spent / items) * (total - now.done);
  if (left < 60_000) return "less than a minute left";
  const minutes = Math.round(left / 60_000);
  return `about ${minutes} ${minutes === 1 ? "minute" : "minutes"} left`;
}

/** How far along a job is, from what it printed. The newest counter wins. */
export function readProgress(actionId: string, lines: Pick<Line, "line">[]): JobProgress {
  for (let i = lines.length - 1; i >= Math.max(0, lines.length - 200); i--) {
    const text = lines[i]?.line ?? "";
    if (text.startsWith(PROGRESS_LINE)) {
      const said = parseProgress(text);
      if (!said) continue;
      return { percent: progressPercent(said), note: null };
    }
    const n = NINJA.exec(text);
    if (n && Number(n[2]) > 0) return { percent: clamp((Number(n[1]) / Number(n[2])) * 100), note: null };
    const p = PERCENT.exec(text);
    if (p) return { percent: clamp(Number(p[1])), note: null, left: LEFT.exec(text)?.[1] ?? null };
  }
  if (actionId === "port-levels") {
    const done = lines.filter((l) => LEVEL.test(l.line)).length;
    if (done > 0) return { percent: null, note: `${done} ${done === 1 ? "level" : "levels"} read` };
  }
  return { percent: null, note: null };
}

/** The last COUNT lines of output, for the Details box. */
export function tailLines(lines: Pick<Line, "line">[], count = 8): string[] {
  return lines
    .filter((l) => !l.line.startsWith(PROGRESS_LINE))
    .slice(-count)
    .map((l) => l.line);
}

/** What the Android notification says while the launcher works with its window closed. */
export interface BackgroundStatus {
  title: string;
  text: string;
  /** 0 to 100, or null for a bar without an end. */
  percent: number | null;
}

/**
 * The notification for a disc image being copied or unpacked (`copy`, null when none is) or the
 * running job; null when there is nothing to keep the launcher alive for. The game itself runs in
 * its own activity and needs none. A job waiting for the next one in a chain keeps it.
 */
export function backgroundStatus(
  running: (Pick<Job, "actionId" | "state" | "startedAt" | "title"> & { lines: Pick<Line, "line">[] }) | undefined,
  waiting: number,
  copy: { name: string; copied: number; total: number } | null,
): BackgroundStatus | null {
  if (copy && copy.copied < copy.total) {
    return { title: "Preparing your disc", text: copy.name, percent: Math.floor((copy.copied * 100) / copy.total) };
  }
  if (running?.state === "running" && !isGame(running)) {
    const { percent } = readProgress(running.actionId, running.lines);
    return { title: phaseLabel(running), text: running.title, percent: percent === null ? null : Math.round(percent) };
  }
  if (waiting > 0) return { title: "Setting up", text: "Waiting for the next step", percent: null };
  return null;
}

// Typed wrappers around the Rust commands (src-tauri/src/lib.rs, whose data
// types live in ../core). In a plain browser (`npm run dev` without Tauri)
// every call goes to mock.ts instead, so the UI can be worked on and
// screenshotted without the desktop app, a disc or a toolchain.
//
// Adding a command: the Rust function, its name in generate_handler!, a
// wrapper and its types here, and a case in mock.ts (docs/ARCHITECTURE.md).
import { invoke } from "@tauri-apps/api/core";
import { listen, type UnlistenFn } from "@tauri-apps/api/event";
import { open } from "@tauri-apps/plugin-dialog";

export const inTauri = typeof window !== "undefined" && "__TAURI_INTERNALS__" in window;

// ---- types: the serde output of core/src (camelCase) ------------------------------

export type Platform = "linux" | "macos" | "windows";
export type Tool = "python" | "pcsx2" | "godot" | "docker";

export interface AppInfo {
  version: string;
  platform: Platform;
  /** Where launcher.json lives. */
  configFile: string | null;
}

/** core/src/config.rs */
export interface Config {
  root: string | null;
  python: string | null;
  pcsx2: string | null;
  godot: string | null;
  docker: string | null;
  setupComplete: boolean;
  lastVersion: string | null;
  discordRpc: boolean;
  discordClientId: string | null;
}

export type DiscordStatus =
  | { kind: "idle" }
  | {
      kind: "viewingGame";
      title: string;
      region: string;
      progressPct: number | null;
      gameId: string;
    }
  | {
      kind: "playingGame";
      title: string;
      region: string;
      gameId: string;
      startTime?: number | null;
    };

export interface Candidate {
  path: string;
  source: string;
}

/** core/src/detect.rs */
export interface Detected {
  roots: Candidate[];
  pythons: Candidate[];
  pcsx2s: Candidate[];
  godots: Candidate[];
  dockers: Candidate[];
}

export interface Check {
  ok: boolean;
  version: string | null;
  message: string;
}

/** core/src/actions.rs */
export type ActionKind = "setup" | "build" | "check" | "play" | "edit";
export type ActionState = "connected" | "unverified" | "planned";
export type Scope = { kind: "repository" } | { kind: "version"; key: string };

export interface ActionView {
  id: string;
  label: string;
  description: string;
  kind: ActionKind;
  state: ActionState;
  detached: boolean;
  docs: string | null;
  todo: string | null;
  thisPlatform: boolean;
  runnable: boolean;
  blockers: string[];
}

/** core/src/catalog.rs */
export interface FileSpec {
  file: string;
  size: number | null;
  sha1: string | null;
}

export interface Input {
  from: string;
  path: string;
  target: string | null;
}

export interface Source {
  name: string;
  repo: string;
  license: string | null;
  commit: string | null;
  date: string | null;
}

export interface Progress {
  matchedCode: number;
  totalCode: number;
  matchedFunctions: number | null;
  totalFunctions: number | null;
  fuzzyPercent: number | null;
  percent: number;
  date: string | null;
  note: string | null;
}

/** core/src/status.rs */
export type DiscState = "found" | "mismatch" | "missing" | "unknown";

export interface VersionStatus {
  disc: { state: DiscState; path: string | null; expected: string | null; message: string };
  boot: { path: string; present: boolean; message: string } | null;
  inputs: { from: string; path: string; present: boolean; message: string }[];
  inputsReady: boolean;
}

/** core/src/library.rs: a version, its status and its actions. */
export interface VersionView {
  key: string;
  game: string;
  name: string;
  title: string;
  region: string;
  serial: string;
  dir: string;
  disc: FileSpec | null;
  boot: FileSpec | null;
  inputs: Input[];
  target: string | null;
  setup: string | null;
  buildHost: string | null;
  readme: string | null;
  source: Source | null;
  progress: Progress | null;
  status: VersionStatus;
  actions: ActionView[];
}

export interface GameView {
  id: string;
  title: string;
  year: number | null;
  versions: VersionView[];
}

export interface Library {
  root: string;
  platform: Platform;
  toolchains: boolean;
  repository: ActionView[];
  games: GameView[];
  actionsError: string | null;
}

/** core/src/jobs.rs */
export interface Started {
  /** 0 for a detached launch (the emulator, Godot), which has no job. */
  id: number;
  title: string;
  command: string;
  detached: boolean;
}

export type JobEvent =
  | { type: "output"; id: number; stream: "stdout" | "stderr"; line: string }
  | { type: "exit"; id: number; code: number | null; cancelled: boolean };

/** core/src/iso.rs */
export type IsoMatchStatus = "exactMatch" | "revisionMismatch" | "wrongGame" | "notPs2Disc" | "invalidIso";

export interface IsoInspection {
  path: string;
  filename: string;
  size: number;
  isValidIso: boolean;
  serial: string | null;
  detectedGameId: string | null;
  detectedGameTitle: string | null;
  detectedVersionName: string | null;
  detectedRegion: string | null;
  targetGameId: string;
  targetVersionKey: string;
  targetSerial: string | null;
  targetExpectedSize: number | null;
  matchesTargetGame: boolean;
  matchesTargetVersion: boolean;
  status: IsoMatchStatus;
  message: string;
}

export interface ImportResult {
  targetKey: string;
  baseromPath: string;
  extractedAssetsDir: string;
  extractedFiles: string[];
  setupMessage: string;
}

// ---- calls ------------------------------------------------------------------------

async function call<T>(command: string, args?: Record<string, unknown>): Promise<T> {
  if (inTauri) return invoke<T>(command, args);
  // Loaded only in a browser, so the desktop build never carries the mock.
  const { mockCall } = await import("./mock");
  return mockCall(command, args) as Promise<T>;
}

export const api = {
  appInfo: () => call<AppInfo>("app_info"),
  getConfig: () => call<Config>("get_config"),
  saveConfig: (config: Config) => call<Config>("save_config", { config }),
  detect: () => call<Detected>("detect"),
  checkRoot: (path: string) => call<Check>("check_root", { path }),
  checkTool: (tool: Tool, path: string) => call<Check>("check_tool", { tool, path }),
  library: () => call<Library>("library"),
  runAction: (scope: Scope, id: string) => call<Started>("run_action", { scope, id }),
  cancelJob: (id: number) => call<null>("cancel_job", { id }),
  /** A file or folder in the checkout, relative to it (`games/rac1/pal/README.md`). */
  openPath: (path: string) => call<null>("open_path", { path }),
  openUrl: (url: string) => call<null>("open_url", { url }),
  inspectIso: (targetKey: string, isoPath: string) => call<IsoInspection>("inspect_iso", { targetKey, isoPath }),
  importIso: (targetKey: string, isoPath: string) => call<ImportResult>("import_iso", { targetKey, isoPath }),
  syncProgressFromWeb: () => call<Library>("sync_progress_from_web"),
  applyProgressJson: (json: string) => call<Library>("apply_progress_json", { json }),
  setDiscordStatus: (status: DiscordStatus) => call<null>("set_discord_status", { status }),
};

/** A folder picker; null when cancelled (or in the browser preview). */
export async function pickFolder(title: string): Promise<string | null> {
  if (!inTauri) return null;
  const picked = await open({ directory: true, title });
  return typeof picked === "string" ? picked : null;
}

/** A file picker (a program); null when cancelled (or in the browser preview). */
export async function pickFile(title: string): Promise<string | null> {
  if (!inTauri) return null;
  const picked = await open({ directory: false, multiple: false, title });
  return typeof picked === "string" ? picked : null;
}

/** An ISO image file picker; null when cancelled. */
export async function pickIsoFile(title = "Select PS2 ISO image"): Promise<string | null> {
  if (!inTauri) return "/home/lynder063/Downloads/games-ps2/Ratchet & Clank (Europe) (En,Fr,De,Es,It) (v2.00).iso";
  const picked = await open({
    directory: false,
    multiple: false,
    title,
    defaultPath: "/home/lynder063/Downloads/games-ps2",
    filters: [{ name: "PS2 ISO Image", extensions: ["iso"] }],
  });
  return typeof picked === "string" ? picked : null;
}

/** Job output and exits, from `job-output` and `job-exit` events. */
export async function onJobEvent(handler: (event: JobEvent) => void): Promise<UnlistenFn> {
  if (!inTauri) {
    const { mockListen } = await import("./mock");
    return mockListen(handler);
  }
  const offOutput = await listen<JobEvent>("job-output", (e) => {
    handler(e.payload);
  });
  const offExit = await listen<JobEvent>("job-exit", (e) => {
    handler(e.payload);
  });
  return () => {
    offOutput();
    offExit();
  };
}

/** A command's error as text: Tauri rejects with the Rust side's String. */
export function errorText(e: unknown): string {
  if (typeof e === "string") return e;
  if (e instanceof Error) return e.message;
  return JSON.stringify(e);
}

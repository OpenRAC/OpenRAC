// The browser preview's stand-in for the Rust side (`npm run dev` without
// Tauri). It reads the same files the desktop app reads (games/*/game.json,
// progress/summary.json, launcher/actions.json, bundled at build time), so
// the preview shows the real games and actions; the disc and tool states
// are made up, and jobs print a few lines and succeed.
//
// Add a case here for every new command in api.ts.
import actionsFile from "../../actions.json";
import summaryFile from "../../../progress/summary.json";
import type {
  ActionView,
  AppInfo,
  Check,
  Config,
  Detected,
  GameView,
  JobEvent,
  Library,
  Platform,
  Progress,
  Scope,
  Started,
  VersionStatus,
  VersionView,
} from "./api";

// ---- the files, as the Rust side parses them --------------------------------------

interface RawAction {
  id: string;
  label: string;
  description?: string;
  kind: ActionView["kind"];
  state?: ActionView["state"];
  program?: string;
  args?: string[];
  platforms?: Platform[];
  requires?: string[];
  artifact?: string;
  detached?: boolean;
  docs?: string;
  todo?: string;
}

interface RawVersion {
  region?: string;
  serial?: string;
  disc?: { file: string; size?: number; sha1?: string };
  boot?: { file: string; size?: number; sha1?: string };
  inputs?: { from: string; path: string; target?: string }[];
  target?: string;
  setup?: string;
  build?: { host?: string; readme?: string };
  source?: { name: string; repo: string; license?: string; commit?: string; date?: string };
}

interface RawGame {
  id: string;
  title: string;
  year?: number;
  titles?: Record<string, string>;
  versions: Record<string, RawVersion>;
}

type RawSummary = Record<
  string,
  {
    progress: {
      matched_code: number;
      total_code: number;
      matched_functions: number | null;
      total_functions: number | null;
      fuzzy_percent: number | null;
      percent: number;
      date: string | null;
    };
    note?: string;
  }
>;

const manifests = import.meta.glob<RawGame>("../../../games/*/game.json", { eager: true, import: "default" });
const actions = actionsFile as { repository: RawAction[]; versions: Record<string, RawAction[] | undefined> };
const summary = summaryFile as RawSummary;

// ---- made-up state -------------------------------------------------------------------

const platform: Platform = "linux";

let config: Config = {
  root: "/home/you/OpenRAC",
  python: "/usr/bin/python3",
  pcsx2: null,
  godot: null,
  docker: "/usr/bin/docker",
  // ?setup opens the first-run screen.
  setupComplete: !new URLSearchParams(typeof location === "undefined" ? "" : location.search).has("setup"),
  lastVersion: null,
  discordRpc: true,
  discordClientId: null,
};

/** Which versions pretend to have their disc found and their inputs placed. */
const discs: Record<string, "found" | "missing" | "mismatch"> = {
  "rac1/pal": "found",
  "rac1/ntsc": "found",
  "rac3/ntsc": "mismatch",
};
const placed = new Set(["rac1/pal"]);
const toolchains = false;

function status(key: string, v: RawVersion): VersionStatus {
  const disc = discs[key] ?? "missing";
  const file = v.disc?.file ?? null;
  const messages = {
    found: "found (size matches; Identify discs checks the checksums)",
    mismatch: `baserom/${file ?? "?"} is not the expected size: another version or a bad dump?`,
    missing: `put your own image in baserom/ (as ${file ?? "?"})`,
  };
  const inputs = (v.inputs ?? []).map((i) => ({
    from: i.from,
    path: i.path,
    present: placed.has(key),
    message: placed.has(key) ? "in place" : "not placed yet: run Place inputs",
  }));
  const boot = inputs.find((i) => i.from === "boot");
  return {
    disc: {
      state: disc,
      path: disc === "found" && file ? `${config.root ?? ""}/baserom/${file}` : null,
      expected: file,
      message: messages[disc],
    },
    boot: boot ? { path: boot.path, present: boot.present, message: boot.message } : null,
    inputs,
    inputsReady: inputs.every((i) => i.present),
  };
}

function view(action: RawAction, st: VersionStatus | null): ActionView {
  const state = action.state ?? "planned";
  const thisPlatform = !action.platforms?.length || action.platforms.includes(platform);
  const blockers: string[] = [];
  if (state === "planned") {
    blockers.push("not connected to the launcher yet");
  } else {
    if (!thisPlatform) blockers.push(`runs on ${(action.platforms ?? []).join(" and ")} only`);
    for (const need of action.requires ?? []) {
      const missing: Record<string, boolean> = {
        disc: st?.disc.state !== "found",
        inputs: !st?.inputsReady,
        toolchains: !toolchains,
        python: !config.python,
        pcsx2: !config.pcsx2,
        godot: !config.godot,
        docker: !config.docker,
        artifact: true,
      };
      const words: Record<string, string> = {
        disc: "your disc image (baserom/)",
        inputs: "the inputs Place inputs puts in the game",
        toolchains: "the compilers in toolchains/",
        python: "Python (Settings)",
        pcsx2: "PCSX2 (Settings)",
        godot: "Godot (Settings)",
        docker: "Docker",
        artifact: `${action.artifact ?? "its artifact"}, which an earlier step makes`,
      };
      if (missing[need]) blockers.push(`needs ${words[need] ?? need}`);
    }
  }
  return {
    id: action.id,
    label: action.label,
    description: action.description ?? "",
    kind: action.kind,
    state,
    detached: action.detached ?? false,
    docs: action.docs ?? null,
    todo: action.todo ?? null,
    thisPlatform,
    runnable: blockers.length === 0,
    blockers,
  };
}

function progress(key: string): Progress | null {
  const row = summary[key];
  if (!row) return null;
  const p = row.progress;
  return {
    matchedCode: p.matched_code,
    totalCode: p.total_code,
    matchedFunctions: p.matched_functions,
    totalFunctions: p.total_functions,
    fuzzyPercent: p.fuzzy_percent,
    percent: p.percent,
    date: p.date,
    note: row.note ?? null,
  };
}

function library(): Library {
  const games: GameView[] = Object.entries(manifests)
    .sort(([a], [b]) => a.localeCompare(b))
    .map(([, g]) => {
      const versions: VersionView[] = Object.entries(g.versions).map(([name, v]) => {
        const key = `${g.id}/${name}`;
        const st = status(key, v);
        return {
          key,
          game: g.id,
          name,
          title: g.titles?.[name] ?? g.title,
          region: v.region ?? "",
          serial: v.serial ?? "",
          dir: `games/${g.id}/${name}`,
          disc: v.disc ? { file: v.disc.file, size: v.disc.size ?? null, sha1: v.disc.sha1 ?? null } : null,
          boot: v.boot ? { file: v.boot.file, size: v.boot.size ?? null, sha1: v.boot.sha1 ?? null } : null,
          inputs: (v.inputs ?? []).map((i) => ({ from: i.from, path: i.path, target: i.target ?? null })),
          target: v.target ?? null,
          setup: v.setup ?? null,
          buildHost: v.build?.host ?? null,
          readme: v.build?.readme ?? null,
          source: v.source
            ? {
                name: v.source.name,
                repo: v.source.repo,
                license: v.source.license ?? null,
                commit: v.source.commit ?? null,
                date: v.source.date ?? null,
              }
            : null,
          progress: progress(key),
          status: st,
          actions: (actions.versions[key] ?? []).map((a) => view(a, st)),
        };
      });
      return { id: g.id, title: g.title, year: g.year ?? null, versions };
    });
  return {
    root: config.root ?? "",
    platform,
    toolchains,
    repository: actions.repository.map((a) => view(a, null)),
    games,
    actionsError: null,
  };
}

// ---- jobs ----------------------------------------------------------------------------

type Listener = (event: JobEvent) => void;
const listeners = new Set<Listener>();
let nextJob = 0;
const timers = new Map<number, ReturnType<typeof setInterval>>();

export function mockListen(handler: Listener): () => void {
  listeners.add(handler);
  return () => listeners.delete(handler);
}

function emit(event: JobEvent) {
  for (const l of listeners) l(event);
}

function findAction(scope: Scope, id: string): RawAction | undefined {
  const list = scope.kind === "repository" ? actions.repository : (actions.versions[scope.key] ?? []);
  return list.find((a) => a.id === id);
}

function start(scope: Scope, id: string): Started {
  const action = findAction(scope, id);
  if (!action) throw new Error(`no action ${id}`);
  const lib = library();
  const list =
    scope.kind === "repository"
      ? lib.repository
      : (lib.games.flatMap((g) => g.versions).find((v) => v.key === scope.key)?.actions ?? []);
  const v = list.find((a) => a.id === id);
  if (!v?.runnable) throw new Error(`${action.label}: ${v?.blockers.join("; ") ?? "cannot run"}`);
  const command = [action.program ?? "?", ...(action.args ?? [])].join(" ");
  if (action.detached) return { id: 0, title: action.label, command, detached: true };

  const job = ++nextJob;
  const lines = [
    `$ ${command}`,
    "(browser preview: nothing really runs)",
    "working…",
    "✔ step one",
    "✔ step two",
    "done",
  ];
  let i = 0;
  timers.set(
    job,
    setInterval(() => {
      const line = lines[i++];
      if (line !== undefined) {
        emit({ type: "output", id: job, stream: "stdout", line });
        return;
      }
      clearInterval(timers.get(job));
      timers.delete(job);
      emit({ type: "exit", id: job, code: 0, cancelled: false });
    }, 450),
  );
  return { id: job, title: action.label, command, detached: false };
}

// ---- the commands ------------------------------------------------------------------

const delay = <T>(value: T) =>
  new Promise<T>((resolve) =>
    setTimeout(() => {
      resolve(value);
    }, 120),
  );

export async function mockCall(command: string, args: Record<string, unknown> = {}): Promise<unknown> {
  switch (command) {
    case "app_info":
      return delay<AppInfo>({
        version: "0.2.0-preview",
        platform,
        configFile: "~/.config/dev.openrac.launcher/launcher.json",
      });
    case "get_config":
      return delay({ ...config });
    case "save_config":
      config = { ...(args.config as Config) };
      return delay({ ...config });
    case "detect":
      return delay<Detected>({
        roots: [{ path: "/home/you/OpenRAC", source: "next to the launcher" }],
        pythons: [{ path: "/usr/bin/python3", source: "PATH" }],
        pcsx2s: [],
        godots: [],
        dockers: [{ path: "/usr/bin/docker", source: "PATH" }],
      });
    case "check_root":
      return delay<Check>({ ok: true, version: null, message: "4 games, 5 versions" });
    case "check_tool":
      return delay<Check>({ ok: true, version: "Python 3.12.4", message: "Python 3.12.4" });
    case "library":
      return delay(library());
    case "inspect_iso": {
      const key = typeof args.targetKey === "string" ? args.targetKey : "rac1/pal";
      const path = typeof args.isoPath === "string" ? args.isoPath : "/path/to/game.iso";
      return delay({
        path,
        filename: "Ratchet & Clank.iso",
        size: 4214784000,
        isValidIso: true,
        serial: "SCES_509.16",
        detectedGameId: "rac1",
        detectedGameTitle: "Ratchet & Clank",
        detectedVersionName: "pal",
        detectedRegion: "PAL (Europe)",
        targetGameId: key.split("/")[0],
        targetVersionKey: key,
        targetSerial: "SCES_509.16",
        targetExpectedSize: 4214784000,
        matchesTargetGame: true,
        matchesTargetVersion: true,
        status: "exactMatch",
        message: "Exact match! Detected Ratchet & Clank (PAL, SCES_509.16) with expected size.",
      });
    }
    case "import_iso": {
      const key = typeof args.targetKey === "string" ? args.targetKey : "rac1/pal";
      const gId = key.split("/")[0];
      return delay({
        targetKey: key,
        baseromPath: `/home/you/OpenRAC/baserom/${gId}.iso`,
        extractedAssetsDir: `/home/you/OpenRAC/${gId}`,
        extractedFiles: ["SYSTEM.CNF", "SCES_509.16", "IOPRP243.IMG"],
        setupMessage: `${key}: inputs placed successfully`,
      });
    }
    case "sync_progress_from_web":
    case "apply_progress_json":
      return delay(library());
    case "run_action":
      return delay(start(args.scope as Scope, args.id as string));
    case "cancel_job": {
      const id = args.id as number;
      clearInterval(timers.get(id));
      timers.delete(id);
      emit({ type: "exit", id, code: null, cancelled: true });
      return delay(null);
    }
    case "open_path":
    case "open_url":
      console.info(`[preview] ${command}`, args);
      return delay(null);
    case "set_discord_status":
      return delay(null);
    default:
      throw new Error(`the preview has no ${command}: add it to src/lib/mock.ts`);
  }
}

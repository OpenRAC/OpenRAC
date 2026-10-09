// The UI's shared state: settings, the library, navigation, toasts and the
// job queue. Pages read `app` and call the functions below; nothing else
// talks to the Rust side except through api.ts.
import {
  api,
  errorText,
  onJobEvent,
  type ActionView,
  type AppInfo,
  type Config,
  type Library,
  type Scope,
  type VersionView,
} from "./api";
import { Queue, type Job } from "./queue";

export type Page = "library" | "game" | "tasks" | "settings";

interface AppState {
  info: AppInfo | null;
  config: Config | null;
  library: Library | null;
  /** Why the library could not be read (no checkout set, a broken game.json). */
  libraryError: string | null;
  page: Page;
  /** The version the game page shows (`rac1/pal`). */
  version: string | null;
  toasts: { id: number; text: string; kind: "info" | "error" }[];
}

export const app = $state<AppState>({
  info: null,
  config: null,
  library: null,
  libraryError: null,
  page: "library",
  version: null,
  toasts: [],
});

/** The jobs, oldest first; reactive. */
export const jobs = $state<{ list: Job[]; selected: number | null }>({ list: [], selected: null });
const queue = new Queue(jobs.list);

// ---- toasts ---------------------------------------------------------------------------

let toastId = 0;
export function toast(text: string, kind: "info" | "error" = "info") {
  const id = ++toastId;
  app.toasts.push({ id, text, kind });
  setTimeout(
    () => {
      app.toasts = app.toasts.filter((t) => t.id !== id);
    },
    kind === "error" ? 8000 : 4000,
  );
}

/** Runs `work`; a failure becomes an error toast and `undefined`. */
export async function guard<T>(work: Promise<T>): Promise<T | undefined> {
  try {
    return await work;
  } catch (e) {
    toast(errorText(e), "error");
    return undefined;
  }
}

// ---- loading ----------------------------------------------------------------------------

export async function start() {
  await onJobEvent((event) => {
    queue.apply(event);
    if (event.type === "exit") {
      const job = queue.jobs.find((j) => j.id === event.id);
      if (job?.state === "failed") toast(`${job.title} failed`, "error");
      void refresh();
      pump();
    }
  });
  app.info = await api.appInfo();
  app.config = await api.getConfig();
  if (app.config.lastVersion) app.version = app.config.lastVersion;
  await refresh();
  void syncProgressWeb();
}

/** Reads the library again: after a job, on focus, after settings change. */
export async function refresh() {
  if (!app.config?.root) {
    app.library = null;
    app.libraryError = "Choose your OpenRAC folder in Settings.";
    return;
  }
  try {
    app.library = await api.library();
    app.libraryError = null;
  } catch (e) {
    app.libraryError = errorText(e);
  }
}

/** Fetches latest progress from openrac.dev; caches in summary.json for offline use. */
export async function syncProgressWeb() {
  if (!app.config?.root) return;
  try {
    const res = await fetch("https://openrac.dev/progress.json", {
      signal: AbortSignal.timeout(4000),
    });
    if (res.ok) {
      const text = await res.text();
      try {
        localStorage.setItem("openrac_progress_cache", text);
      } catch {
        // ignore localStorage errors
      }
      app.library = await api.applyProgressJson(text);
      return;
    }
  } catch {
    // Offline or network error: try backend or keep using cached summary.json
  }

  try {
    app.library = await api.syncProgressFromWeb();
  } catch {
    // Offline: keep cached library
  }
}

export async function saveConfig(config: Config) {
  const saved = await guard(api.saveConfig(config));
  if (saved) {
    app.config = saved;
    await refresh();
  }
}

// ---- navigation ---------------------------------------------------------------------------

export function openVersion(key: string) {
  app.version = key;
  app.page = "game";
  updateDiscordPresence();
  if (app.config && app.config.lastVersion !== key) {
    void guard(api.saveConfig({ ...app.config, lastVersion: key })).then((saved) => {
      if (saved) app.config = saved;
    });
  }
}

export function currentVersion(): VersionView | null {
  const versions = app.library?.games.flatMap((g) => g.versions) ?? [];
  return versions.find((v) => v.key === app.version) ?? null;
}

export function updateDiscordPresence(page: Page = app.page, version: string | null = app.version) {
  if (page === "game" && version) {
    const v = currentVersion();
    if (v) {
      void api.setDiscordStatus({
        kind: "viewingGame",
        title: v.title,
        region: v.region,
        progressPct: v.progress?.percent ?? null,
        gameId: v.game,
      });
      return;
    }
  }
  void api.setDiscordStatus({ kind: "idle" });
}

// ---- actions and jobs ---------------------------------------------------------------------

/** Runs an action: a detached one (the emulator) at once, anything else through the queue. */
export async function run(scope: Scope, action: ActionView) {
  if (action.detached) {
    const started = await guard(api.runAction(scope, action.id));
    if (started) toast(`Started: ${started.title}`);
    return;
  }
  const job = queue.add(scope, action.id, action.label);
  jobs.selected = job.key;
  toast(queue.running() ? `Queued: ${action.label}` : `Started: ${action.label}`);
  pump();
}

/** Starts the next queued job, if nothing is running. */
function pump() {
  const job = queue.next();
  if (!job) return;
  job.state = "running";
  api.runAction(job.scope, job.actionId).then(
    (started) => {
      job.title = started.title;
      queue.started(job, started.id, started.command);
    },
    (e: unknown) => {
      queue.refused(job, errorText(e));
      toast(errorText(e), "error");
      pump();
    },
  );
}

export async function cancel(job: Job) {
  if (job.state === "queued") {
    queue.dequeue(job);
    return;
  }
  if (job.state === "running" && job.id !== null) await guard(api.cancelJob(job.id));
}

export function clearFinished() {
  queue.clear();
}

export function runningJob(): Job | undefined {
  return queue.running();
}

export function pendingJobs(): number {
  return queue.pending();
}

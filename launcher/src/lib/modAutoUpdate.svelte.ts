// Mods kept current by themselves (core/src/mod_updates.rs): at start, every 6 hours while the
// launcher is open, and before each game start, from one fetch of the index. Patch and minor
// versions go in silently and a toast says so; majors and native versions with new hooks wait on
// the Mods tab ("Update available"). Never while the game runs, and an error never blocks Play.
// The once-a-day look at mods from a link (modUpdates.svelte.ts) stays for mods not in the index.
import { api, type UpdateItem, type UpdatePlan, type VersionView } from "./api";
import { app, jobs, toast } from "./app.svelte";
import { playerVersion } from "./player.svelte";
import { updatedMods } from "./modsText";

export const SIX_HOURS = 6 * 60 * 60 * 1000;
/** How long Play waits for updates before it starts anyway. */
export const BEFORE_PLAY_MS = 20_000;

export const autoUpdates = $state<{
  /** By game: what the last look found. */
  plans: Record<string, UpdatePlan>;
  /** By game: what went in by itself last, with the changelogs. */
  recent: Record<string, UpdateItem[]>;
  running: boolean;
}>({ plans: {}, recent: {}, running: false });

/** Whether a game is being played now: its Play job, or Android's game activity. */
export function gameRunning(): boolean {
  if (app.game?.running) return true;
  return jobs.list.some((j) => j.actionId === "play" && (j.state === "running" || j.state === "queued"));
}

function playable(): VersionView[] {
  const out: VersionView[] = [];
  for (const game of app.library?.games ?? []) {
    const v = playerVersion(game);
    const play = v?.actions.find((a) => a.id === "play");
    if (v && play && play.state !== "planned") out.push(v);
  }
  return out;
}

/** One version's look and silent updates; what went in. */
export async function updateVersion(v: VersionView, ids: string[] | null = null): Promise<UpdateItem[]> {
  const run = await api.modsAutoUpdate(v.game, v.name, ids, gameRunning());
  autoUpdates.plans[v.game] = run.plan;
  if (run.updated.length > 0) {
    autoUpdates.recent[v.game] = run.updated;
    // What went in leaves the lists of what waits.
    const done = run.updated.map((u) => u.id);
    const plan = run.plan;
    autoUpdates.plans[v.game] = {
      silent: plan.silent.filter((u) => !done.includes(u.id)),
      waiting: plan.waiting.filter((u) => !done.includes(u.id)),
      held: plan.held,
    };
  }
  return run.updated;
}

/** Every playable game's mods, when the setting is on and no game runs. Errors are quiet. */
export async function autoUpdateAll(): Promise<void> {
  if (!app.config?.autoUpdateMods || autoUpdates.running || gameRunning()) return;
  autoUpdates.running = true;
  let count = 0;
  try {
    for (const v of playable()) {
      count += (await updateVersion(v).catch(() => [])).length;
    }
  } finally {
    autoUpdates.running = false;
  }
  if (count > 0) toast(updatedMods(count));
}

/** Before Play: the game's updates, waited for at most BEFORE_PLAY_MS; Play goes on whatever happens. */
export async function beforePlay(versionKey: string): Promise<void> {
  if (!app.config?.autoUpdateMods || gameRunning()) return;
  const v = playable().find((x) => x.key === versionKey);
  if (!v) return;
  const work = updateVersion(v)
    .then((done) => {
      if (done.length > 0) toast(updatedMods(done.length));
    })
    .catch(() => undefined);
  await Promise.race([work, new Promise((r) => setTimeout(r, BEFORE_PLAY_MS))]);
}

/** At start, then every 6 hours while the launcher is open. */
export function startAutoUpdates(): () => void {
  void autoUpdateAll();
  const timer = setInterval(() => void autoUpdateAll(), SIX_HOURS);
  return () => {
    clearInterval(timer);
  };
}

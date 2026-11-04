// openrac:// links and the change set they open (DeepLinkInstall.svelte), on every platform:
// getCurrent() for the link the launcher was started with, onOpenUrl() for one sent to the open
// launcher (Tauri's deep-link plugin; on Linux and Windows a second start forwards it).
//
// A link only opens the change set: nothing installs without the player's click. One link is shown
// at a time, and one more may wait; a flood of links beyond that is dropped. Each link is read by
// core/src/deeplink.rs (deeplink_parse), never fetched. The Browse view opens the same change set
// with `openChangeSet`.
import { api, errorText, modErrorOf, onAndroid, type ApplyResult, type IndexView, type InstallPlan } from "./api";
import type { ModAction, ModError, WantedMod } from "./api";
import { app, lookForUpdate, runChain, saveConfig } from "./app.svelte";
import { playerVersion } from "./player.svelte";

export type Phase = "idle" | "loading" | "ready" | "applying" | "done" | "error";

export interface ChangeSet {
  /** The link shown, or "browse" for one opened from the Browse view. */
  link: string | null;
  phase: Phase;
  wanted: WantedMod[];
  /** The game and disc the mods are for, its title and disc serial (for the save backup). */
  game: string | null;
  version: string | null;
  title: string;
  serial: string | null;
  index: IndexView | null;
  /** The plan as it now stands: `wanted`, the ticked offers, `turnOff` counted as off. */
  plan: InstallPlan | null;
  /** The offers of the first plan: recommended ones start ticked, suggested ones not. */
  offers: { recommends: InstallPlan["recommends"]; suggests: InstallPlan["suggests"] };
  ticked: string[];
  /** Mods a conflict's "Replace" turns off first. */
  turnOff: string[];
  progress: Record<string, "waiting" | "installing" | "done" | "failed">;
  result: ApplyResult | null;
  error: ModError | null;
  /** The next link, waiting for this one to close. */
  queued: string | null;
}

const empty = (): ChangeSet => ({
  link: null,
  phase: "idle",
  wanted: [],
  game: null,
  version: null,
  title: "",
  serial: null,
  index: null,
  plan: null,
  offers: { recommends: [], suggests: [] },
  ticked: [],
  turnOff: [],
  progress: {},
  result: null,
  error: null,
  queued: null,
});

export const changeSet = $state<ChangeSet>(empty());

/** The hub's error from a rejected call, or one made from its text. */
const asModError = (e: unknown): ModError =>
  modErrorOf(e) ?? { kind: "bug", modId: null, detail: errorText(e), action: null };

/** A link arrived: shown now, or kept for when the one shown closes. Anything more is dropped. */
export function receive(url: string | null | undefined): void {
  if (!url) return;
  if (changeSet.link === null) {
    void openLink(url);
  } else if (changeSet.queued === null && changeSet.link !== url) {
    changeSet.queued = url;
  }
}

/** The game a change set is for: the link's hint, else the one last opened, else the first. */
function target(hint: string | null) {
  const games = app.library?.games ?? [];
  const game =
    games.find((g) => g.id === hint) ??
    games.find((g) => g.id === app.version?.split("/")[0]) ??
    games.find((g) => g.id === "rac1") ??
    games[0];
  const v = game ? playerVersion(game) : null;
  return game && v ? { game: game.id, version: v.name, title: game.title, serial: v.serial, key: v.key } : null;
}

export async function openLink(url: string): Promise<void> {
  Object.assign(changeSet, { ...empty(), link: url, phase: "loading", queued: changeSet.queued });
  try {
    const install = await api.deeplinkParse(url);
    await openChangeSet(install.mods, install.game, url);
  } catch (e) {
    changeSet.error = asModError(e);
    changeSet.phase = "error";
  }
}

/** Opens the change set for `wanted` (a link's mods, or Browse's Install), on the Mods page. */
export async function openChangeSet(wanted: WantedMod[], game: string | null = null, link = "browse"): Promise<void> {
  const where = target(game);
  Object.assign(changeSet, { ...empty(), link, wanted, phase: "loading", queued: changeSet.queued });
  // On a desktop the Mods tab and the game; Android's Mods page is one page.
  app.page = "mods";
  if (where) {
    Object.assign(changeSet, { game: where.game, version: where.version, title: where.title, serial: where.serial });
    if (!onAndroid) app.version = where.key;
  } else {
    changeSet.error = { kind: "userFixable", modId: null, detail: "Set a game up first.", action: null };
    changeSet.phase = "error";
    return;
  }
  try {
    changeSet.index = await api.indexFetch(changeSet.game, changeSet.version);
    await resolve(true);
  } catch (e) {
    changeSet.error = asModError(e);
    changeSet.phase = "error";
  }
}

/** Plans again: after a tick, a fix or at the start (`first`: the offers are taken from it). */
export async function resolve(first = false): Promise<void> {
  const { game, version } = changeSet;
  if (!game || !version) return;
  const extra = changeSet.ticked.map((id) => ({ id, version: null }));
  const plan = await api.indexResolve(game, version, [...changeSet.wanted, ...extra], changeSet.turnOff);
  if (first) {
    changeSet.offers = { recommends: plan.recommends, suggests: plan.suggests };
    // Recommended mods start ticked: planned again with them, so their own needs show too.
    changeSet.ticked = plan.recommends.map((o) => o.id);
    if (changeSet.ticked.length > 0) return resolve();
  }
  changeSet.plan = plan;
  changeSet.phase = "ready";
}

export async function toggle(id: string): Promise<void> {
  changeSet.ticked = changeSet.ticked.includes(id)
    ? changeSet.ticked.filter((x) => x !== id)
    : [...changeSet.ticked, id];
  await resolve().catch((e: unknown) => {
    changeSet.error = asModError(e);
  });
}

/** A fix button: Replace (turn the other mod off), Switch to the dev build, Retry. */
export async function fix(action: ModAction): Promise<void> {
  switch (action.type) {
    case "replaceConflict":
      if (!changeSet.turnOff.includes(action.replace)) changeSet.turnOff = [...changeSet.turnOff, action.replace];
      await resolve();
      return;
    case "switchToDev":
      await switchToDev();
      return;
    case "retry":
    case "useCached":
      if (changeSet.link && changeSet.link !== "browse") await openLink(changeSet.link);
      else await openChangeSet(changeSet.wanted, changeSet.game);
      return;
    case "report":
      await api.openUrl(action.url);
      return;
    default:
      return;
  }
}

/** The dev build's channel, with this link kept to open again after its update and restart. */
export async function switchToDev(): Promise<void> {
  if (!app.config) return;
  const link = changeSet.link && changeSet.link !== "browse" ? changeSet.link : null;
  await saveConfig({ ...app.config, updateChannel: "dev", pendingDeepLink: link });
  close();
  if (await lookForUpdate().catch(() => false)) app.showUpdateModal = true;
}

/** Installs the plan: each mod's progress from `index-apply` events. */
export async function apply(): Promise<void> {
  const { game, version, plan } = changeSet;
  if (!game || !version || !plan || plan.blocked || changeSet.phase === "applying") return;
  changeSet.phase = "applying";
  changeSet.progress = Object.fromEntries(plan.install.map((m) => [m.id, "waiting" as const]));
  const { onIndexApply } = await import("./api");
  const stop = await onIndexApply((p) => {
    changeSet.progress[p.id] = p.step;
  });
  try {
    const extra = changeSet.ticked.map((id) => ({ id, version: null }));
    // A link from openrac.dev with exact versions (a shared setup) keeps those mods at them.
    const shared = changeSet.link !== "browse" && changeSet.wanted.some((w) => w.version !== null);
    const result = await api.indexApply(
      game,
      version,
      [...changeSet.wanted, ...extra],
      changeSet.turnOff,
      changeSet.serial,
      shared,
    );
    changeSet.result = result;
    // Data packs go to the overrides, as from a link (lib/addMods.ts).
    const v = app.library?.games.find((g) => g.id === game)?.versions.find((x) => x.name === version);
    const adder = v?.actions.find((a) => a.id === "overrides-add" && a.thisPlatform);
    for (const pack of result.packs) {
      if (v && adder?.runnable) await runChain({ kind: "version", key: v.key }, [adder], { path: pack.path });
    }
    changeSet.phase = "done";
  } catch (e) {
    changeSet.error = asModError(e);
    changeSet.phase = "error";
  } finally {
    stop();
  }
}

/** Closes the change set; the waiting link, if any, opens next. */
export function close(): void {
  if (changeSet.phase === "applying") return;
  const next = changeSet.queued;
  Object.assign(changeSet, empty());
  if (next) void openLink(next);
}

/** Where links come from: the plugin, or the tests' stand-in. */
export interface LinkSource {
  getCurrent: () => Promise<string[] | null>;
  onOpenUrl: (handler: (urls: string[]) => void) => Promise<() => void>;
}

/** Listens for links: the one the launcher started with (or kept across Switch to the dev build),
 *  then every one sent to it. In the browser preview, `?deeplink=<url>`. */
export async function startDeepLinks(source?: LinkSource): Promise<() => void> {
  const pending = app.config?.pendingDeepLink ?? null;
  if (pending && app.config) {
    await saveConfig({ ...app.config, pendingDeepLink: null });
    receive(pending);
  }
  if (!source) {
    const { inTauri } = await import("./api");
    if (!inTauri) {
      const asked = /[?&]deeplink=([^&#]+)/.exec(typeof location === "undefined" ? "" : location.search)?.[1];
      receive(asked ? decodeURIComponent(asked) : null);
      return () => undefined;
    }
    source = await import("@tauri-apps/plugin-deep-link");
  }
  const stop = await source.onOpenUrl((urls) => {
    receive(urls[0]);
  });
  receive((await source.getCurrent().catch(() => null))?.[0]);
  return stop;
}

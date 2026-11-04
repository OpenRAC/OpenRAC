// The browser preview's mod hub (mock.ts hands it index_fetch, index_resolve, index_apply and
// deeplink_parse): the shared fixture index (core/tests/fixtures/index.json, the one the Rust
// tests and the website use), and a small stand-in for core/src/resolver.rs. It knows the rules
// the preview shows (dependencies first, the API fit with a "dev build" fix, revoked versions,
// conflicts with what is on), not every edge the Rust tests cover.
//
// ?offline: the list is from the cache, 3 hours old. ?stale: past its expiry. ?conflict: Classic
// controls is on. ?applyfail: the second mod of a change set fails its hash. ?indexfail: no list.
import fixture from "../../core/tests/fixtures/index.json";
import type { ApplyProgress } from "./generated/ApplyProgress";
import type { ApplyResult } from "./generated/ApplyResult";
import type { DeepLinkInstall } from "./generated/DeepLinkInstall";
import type { IndexMod } from "./generated/IndexMod";
import type { IndexVersion } from "./generated/IndexVersion";
import type { IndexView } from "./generated/IndexView";
import type { InstallPlan } from "./generated/InstallPlan";
import type { ModError } from "./generated/ModError";
import type { ModIndex } from "./generated/ModIndex";
import type { PlannedMod } from "./generated/PlannedMod";
import type { WantedMod } from "./generated/WantedMod";
import type { LastUpdate } from "./generated/LastUpdate";
import type { UpdateItem } from "./generated/UpdateItem";
import type { UpdatePlan } from "./generated/UpdatePlan";
import type { UpdateRun } from "./generated/UpdateRun";

const query = new URLSearchParams(typeof location === "undefined" ? "" : location.search);

/** The fixture as index_fetch sends it: each mod with its index's media address. */
export function mockIndex(): ModIndex {
  const index = structuredClone(fixture) as unknown as ModIndex;
  for (const m of index.mods) m.mediaBase = index.media;
  return index;
}

/** The mods the preview's player has (mock.ts's gameplay mods, by id and version). */
const mockInstalled = (): { id: string; version: string; enabled: boolean }[] => [
  { id: "camera-base", version: "1.1.0", enabled: true },
  ...(query.has("conflict") ? [{ id: "classic-controls", version: "1.0.0", enabled: true }] : []),
];

const API: [number, number] = [1, 10];

function parseVersion(v: string): [number, number, number, boolean] | null {
  const m = /^(\d+)\.(\d+)\.(\d+)(-.+)?$/.exec(v);
  return m ? [Number(m[1]), Number(m[2]), Number(m[3]), !!m[4]] : null;
}

function compare(a: string, b: string): number {
  const x = parseVersion(a);
  const y = parseVersion(b);
  if (!x || !y) return a.localeCompare(b);
  for (let i = 0; i < 3; i++) if (x[i] !== y[i]) return (x[i] as number) - (y[i] as number);
  return Number(!x[3]) - Number(!y[3]);
}

function meets(version: string, wanted: string | null | undefined): boolean {
  if (!wanted) return true;
  if (wanted.startsWith(">=")) return compare(version, wanted.slice(2)) >= 0;
  if (wanted.startsWith("=")) return compare(version, wanted.slice(1)) === 0;
  return false;
}

function apiFits(api: string | null, have: [number, number]): boolean {
  if (!api) return true;
  const [major, minor] = api.split(".").map(Number);
  return major === have[0] && (minor ?? 0) <= have[1];
}

const revoked = (index: ModIndex, id: string, v: string) =>
  index.revoked.find((r) => r.id === id && (r.versions.includes("*") || r.versions.includes(v)))?.reason ?? null;

function planned(m: IndexMod, v: IndexVersion, auto: boolean, by: string[], installed: string | null): PlannedMod {
  return {
    id: m.id,
    name: m.name,
    version: v.version,
    installed,
    kind: m.type,
    auto,
    requiredBy: by,
    repo: m.repo,
    path: m.path,
    tag: v.tag,
    asset: (m.type === "native" ? v.platforms["linux-x86_64"] : v.platforms.any) ?? null,
    impact: m.impact,
    hooks: v.hooks,
    enable: false,
  };
}

/** A stand-in for resolver.rs's resolve, on the fixture. */
export function mockResolve(wanted: WantedMod[], turnOff: string[] = [], api: [number, number] = API): InstallPlan {
  const index = mockIndex();
  const installed = mockInstalled().map((m) => ({ ...m, enabled: m.enabled && !turnOff.includes(m.id) }));
  const plan: InstallPlan = {
    install: [],
    alreadyOk: [],
    updates: [],
    packs: [],
    recommends: [],
    suggests: [],
    conflicts: [],
    missing: [],
    incompatible: [],
    notes: [],
    blocked: false,
    action: null,
  };
  const roots = new Set(wanted.map((w) => w.id));
  const seen = new Set<string>();
  const visit = (id: string, want: string | null, by: string | null) => {
    const prior = [...plan.install, ...plan.alreadyOk].find((p) => p.id === id);
    if (prior) {
      if (by && !prior.requiredBy.includes(by)) prior.requiredBy.push(by);
      return;
    }
    if (seen.has(id)) return;
    seen.add(id);
    const m = index.mods.find((x) => x.id === id);
    if (!m) {
      plan.missing.push({ id, requiredBy: by });
      return;
    }
    const have = installed.find((i) => i.id === id);
    const first = m.versions[0];
    if (have && first && meets(have.version, want)) {
      plan.alreadyOk.push({
        ...planned(m, first, !roots.has(id), by ? [by] : [], have.version),
        version: have.version,
      });
      return;
    }
    const ok = m.versions.filter((v) => !v.prerelease && !revoked(index, id, v.version) && meets(v.version, want));
    const fit = ok.find((v) => apiFits(v.api, api));
    if (!fit) {
      const dev = ok.find((v) => apiFits(v.api, [1, 11]));
      plan.incompatible.push({
        id,
        name: m.name,
        why: { kind: "needsNewerApi", api: ok[0]?.api ?? "" },
        action: dev ? { type: "switchToDev" } : null,
      });
      return;
    }
    if (fit !== ok[0]) {
      plan.notes.push({
        kind: "olderForApi",
        id,
        name: m.name,
        picked: fit.version,
        newest: ok[0]?.version ?? "",
        api: ok[0]?.api ?? "",
      });
    }
    for (const r of fit.requires) visit(r.id, r.version ?? null, id);
    if (m.type === "pack") {
      plan.notes.push({ kind: "pack", id, name: m.name, mods: fit.requires.map((r) => r.id) });
      plan.packs.push(planned(m, fit, !roots.has(id), by ? [by] : [], null));
      return;
    }
    plan.install.push(planned(m, fit, !roots.has(id), by ? [by] : [], have?.version ?? null));
    if (have) plan.updates.push(id);
    if (m.impact.includes("online")) plan.notes.push({ kind: "online", id, name: m.name });
    if (m.impact.includes("touches-saves") && !have) plan.notes.push({ kind: "touchesSaves", id, name: m.name });
  };
  for (const w of wanted) visit(w.id, w.version ? `=${w.version}` : null, null);
  const on = new Set([...installed.filter((i) => i.enabled).map((i) => i.id), ...plan.install.map((p) => p.id)]);
  for (const p of plan.install) {
    const v = index.mods.find((m) => m.id === p.id)?.versions.find((x) => x.version === p.version);
    for (const c of v?.conflicts ?? []) {
      if (!on.has(c.id)) continue;
      const isInstalled = !plan.install.some((x) => x.id === c.id);
      plan.conflicts.push({
        id: p.id,
        name: p.name,
        with: c.id,
        withName: index.mods.find((m) => m.id === c.id)?.name ?? c.id,
        reason: c.reason,
        installed: isInstalled,
        action: isInstalled ? { type: "replaceConflict", replace: c.id, with: p.id } : null,
      });
    }
  }
  const offered = new Set<string>();
  for (const p of plan.install) {
    const m = index.mods.find((x) => x.id === p.id);
    for (const [list, into] of [
      [m?.recommends ?? [], plan.recommends],
      [m?.suggests ?? [], plan.suggests],
    ] as const) {
      for (const id of list) {
        if (on.has(id) || offered.has(id) || installed.some((i) => i.id === id)) continue;
        const o = index.mods.find((x) => x.id === id);
        if (!o) continue;
        offered.add(id);
        into.push({ id, name: o.name, description: o.description, by: m?.name ?? "" });
      }
    }
  }
  plan.blocked = plan.conflicts.length + plan.missing.length + plan.incompatible.length > 0;
  plan.action = plan.incompatible.find((i) => i.action?.type === "switchToDev")?.action ?? null;
  return plan;
}

export function mockIndexView(): IndexView {
  if (query.has("indexfail")) {
    // Tauri rejects with the error's JSON, not an Error: the preview does the same.
    // eslint-disable-next-line @typescript-eslint/only-throw-error
    throw {
      kind: "transient",
      modId: null,
      detail: "The mod list could not be fetched (offline).",
      action: { type: "retry" },
    } satisfies ModError;
  }
  const now = Math.floor(Date.now() / 1000);
  return {
    index: mockIndex(),
    stale: query.has("stale"),
    offline: query.has("offline"),
    fetchedAt: query.has("offline") ? now - 3 * 3600 : now,
    newerFormat: false,
    errors: [],
    turnedOff: [],
  };
}

/** A stand-in for deeplink.rs's parse: the three forms, nothing checked as hard. */
export function mockParse(url: string): DeepLinkInstall {
  const m = /^openrac:\/\/install\/?([^?]*)(?:\?(.*))?$/i.exec(url);
  if (!m) {
    const e: ModError = { kind: "userFixable", modId: null, detail: "This link can't be opened.", action: null };
    // eslint-disable-next-line @typescript-eslint/only-throw-error
    throw e;
  }
  const params = new URLSearchParams(m[2] ?? "");
  const specs = m[1] ? [m[1]] : (params.get("mods") ?? "").split(",").filter(Boolean);
  return {
    mods: specs.map((s) => {
      const [id = "", version = null] = s.split("@");
      return { id, version };
    }),
    game: params.get("game"),
    disc: params.get("disc"),
  };
}

const listeners = new Set<(p: ApplyProgress) => void>();

export function mockIndexListen(handler: (p: ApplyProgress) => void): () => void {
  listeners.add(handler);
  return () => listeners.delete(handler);
}

const wait = (ms: number) => new Promise((r) => setTimeout(r, ms));

/** Each mod "installed" in turn, its progress heard, the second failing with ?applyfail. */
export async function mockApply(wanted: WantedMod[], turnOff: string[]): Promise<ApplyResult> {
  const plan = mockResolve(wanted, turnOff);
  const result: ApplyResult = {
    done: [],
    packs: [],
    failed: null,
    skipped: [],
    notOn: [],
    backup: null,
    rolledBack: false,
  };
  const total = plan.install.length;
  for (const [at, m] of plan.install.entries()) {
    for (const l of listeners) l({ id: m.id, step: "installing", at, total });
    await wait(500);
    if (query.has("applyfail") && at === 1) {
      for (const l of listeners) l({ id: m.id, step: "failed", at, total });
      result.failed = { kind: "upstreamBad", modId: m.id, detail: "the SHA-256 differs", action: null };
      result.skipped = plan.install.slice(at + 1).map((x) => x.id);
      return result;
    }
    if (m.impact.includes("touches-saves") && !result.backup) result.backup = "~/.local/share/openrac/saves.backup/now";
    result.done.push(m.id);
    for (const l of listeners) l({ id: m.id, step: "done", at, total });
  }
  return result;
}

// ---- Updates (mod_updates.rs), as the preview shows them: camera-base goes in by itself, a major
// of lock-strafe waits with its changelog, item-randomizer waits for a newer launcher.
// ?noupdates: none. ?crashupdate: the last start's crash came after an update of camera-base.

const updateItem = (id: string, from: string, to: string, hold: UpdateItem["hold"]): UpdateItem => {
  const m = mockIndex().mods.find((x) => x.id === id);
  return {
    id,
    name: m?.name ?? id,
    from,
    to,
    native: m?.type === "native",
    changelog: m?.versions.find((v) => v.version === to)?.changelog ?? "",
    hold,
  };
};

export function mockUpdates(): UpdatePlan {
  if (query.has("noupdates")) return { silent: [], waiting: [], held: [] };
  return {
    silent: [updateItem("camera-base", "1.1.0", "1.2.0", null)],
    waiting: [
      updateItem("lock-strafe", "1.1.0", "1.2.0", { kind: "newHooks", hooks: ["rac1.player.turn_to_heading"] }),
    ],
    held: [updateItem("item-randomizer", "1.3.0", "1.4.0", { kind: "needsLauncher", api: "1.11" })],
  };
}

export function mockAutoUpdate(ids: string[] | null, running: boolean): UpdateRun {
  if (running) {
    const e: ModError = {
      kind: "userFixable",
      modId: null,
      detail: "Mods are not updated while the game runs.",
      action: { type: "closeGame" },
    };
    // eslint-disable-next-line @typescript-eslint/only-throw-error
    throw e;
  }
  const plan = mockUpdates();
  const updated = ids ? [...plan.silent, ...plan.waiting].filter((i) => ids.includes(i.id)) : plan.silent;
  return { updated, plan, result: null };
}

export function mockLastUpdate(): LastUpdate | null {
  return query.has("crashupdate")
    ? {
        at: Math.floor(Date.now() / 1000) - 600,
        mods: [{ id: "camera-base", name: "Camera base", from: "1.1.0", to: "1.2.0" }],
      }
    : null;
}

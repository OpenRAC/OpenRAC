// The Going Commando save editor's checks and summaries on the page: the same limits
// core/src/save_edit_rac2.rs enforces, so a field shows its problem before Save is pressed. The
// Rust side checks again before it writes.
import type { Rac2SaveFields, Rac2SaveView } from "./api";
import type { Rac2ItemState } from "./generated/Rac2ItemState";
import type { Rac2VersionInfo } from "./generated/Rac2VersionInfo";

/** The editor's sections, in page order. */
export type Rac2Section = "ratchet" | "weapons" | "gadgets" | "planets" | "skills" | "advanced";

/** Ship paint jobs: the game numbers them, 0 the one the ship starts with. */
export const SHIP_PAINTS = 18;
/** Armour levels: none, then the four the armour vendors sell. */
export const ARMOR_LEVELS = 5;

const whole = (n: number | null, max: number) => n !== null && Number.isInteger(n) && n >= 0 && n <= max;
const num = (n: number) => n.toLocaleString("en-US");

/** The version an item is in, in `f` (the selected one), or `null` for an item with one version. */
export function selectedVersion(view: Rac2SaveView, f: Rac2SaveFields, id: number): Rac2VersionInfo | null {
  const item = f.items.find((i) => i.id === id);
  const info = view.items.find((i) => i.id === id);
  if (item?.version == null || !info) return null;
  return info.versions[item.version] ?? null;
}

/** A version as the selector shows it: "V2", or "V3 Mega" for one only challenge mode reaches. */
export function versionLabel(v: Rac2VersionInfo): string {
  return v.challengeOnly ? `${v.label} Mega` : v.label;
}

/** How the game reaches a version, in a player's words. */
export function versionHow(v: Rac2VersionInfo): string {
  switch (v.how) {
    case "first":
      return "The version the weapon comes in";
    case "experience":
      return `Upgrades itself at ${num(v.stepXp ?? 0)} experience${v.challengeOnly ? ", in challenge mode" : ""}`;
    case "bought":
      return "Bought from a vendor in challenge mode; experience starts again at 0";
    case "challengeStart":
      return "Given when challenge mode starts";
  }
}

/** The most ammo and the experience range an item takes in the version chosen in `f`. */
function limits(view: Rac2SaveView, f: Rac2SaveFields, id: number) {
  const info = view.items.find((i) => i.id === id);
  const v = selectedVersion(view, f, id);
  return {
    maxAmmo: v ? v.maxAmmo : (info?.maxAmmo ?? null),
    minXp: v ? (v.minXp ?? 0) : 0,
    maxXp: v ? v.maxXp : (info?.maxXp ?? null),
  };
}

/** A problem with what was typed, by field ("bolts", "ammo-30", "xp-30", "version-30"), in a
 *  player's words. */
export function fieldErrors(view: Rac2SaveView, f: Rac2SaveFields): Record<string, string> {
  const errors: Record<string, string> = {};
  if (!whole(f.bolts, view.maxBolts)) errors.bolts = `From 0 to ${num(view.maxBolts)}`;
  if (!whole(f.raritanium, view.maxBolts)) errors.raritanium = `From 0 to ${num(view.maxBolts)}`;
  if (!whole(f.nanotechXp, view.maxXp)) errors.nanotechXp = `From 0 to ${num(view.maxXp)}`;
  for (const item of f.items) {
    const info = view.items.find((i) => i.id === item.id);
    const was = view.fields.items.find((i) => i.id === item.id);
    const key = String(item.id);
    const versionChanged = item.version !== was?.version;
    const { maxAmmo, minXp, maxXp } = limits(view, f, item.id);
    if (maxAmmo !== null && (item.ammo !== was?.ammo || versionChanged) && !whole(item.ammo, maxAmmo)) {
      errors[`ammo-${key}`] = `From 0 to ${num(maxAmmo)}`;
    }
    if (maxXp !== null && (item.xp !== was?.xp || versionChanged)) {
      if (!whole(item.xp, maxXp) || (item.xp ?? 0) < minXp) errors[`xp-${key}`] = `From ${num(minXp)} to ${num(maxXp)}`;
    }
    if (versionChanged && info) {
      const v = selectedVersion(view, f, item.id);
      if (info.worn) errors[`version-${key}`] = "In Ratchet's hand: switch weapons in the game and save first";
      else if (!item.owned) errors[`version-${key}`] = "Give the weapon to change its version";
      else if (v?.challengeOnly && view.challengeMode === 0) {
        errors[`version-${key}`] = `${versionLabel(v)} exists only in challenge mode, which this save has not started`;
      }
    }
  }
  return errors;
}

/** Whether `v` can be chosen for this save: Mega versions need challenge mode. */
export function versionAllowed(view: Rac2SaveView, v: Rac2VersionInfo): boolean {
  return !v.challengeOnly || view.challengeMode > 0;
}

/**
 * Moves an item to version `to`, with the experience and ammo the game would leave: an upgrade by
 * experience keeps at least its step, the challenge-mode purchase starts again at 0, and a version
 * reached by moving up comes with full ammo (as the game's upgrade and purchase give it). Moving
 * down keeps what fits.
 */
export function setVersion(view: Rac2SaveView, f: Rac2SaveFields, id: number, to: number): void {
  const item: Rac2ItemState | undefined = f.items.find((i) => i.id === id);
  const info = view.items.find((i) => i.id === id);
  const v = info?.versions[to];
  if (item?.version == null || !info || !v) return;
  const from = item.version;
  if (from === to) return;
  item.version = to;
  if (item.xp !== null && v.minXp !== null && v.maxXp !== null) {
    const up = to > from;
    const bought = up && info.versions.slice(from + 1, to + 1).some((x) => x.how !== "experience");
    // Back to the experience the save had when the version is the one it holds.
    const was = view.fields.items.find((i) => i.id === id);
    if (was?.version === to && was.xp !== null) item.xp = was.xp;
    else if (bought && v.how !== "experience") item.xp = v.minXp;
    else item.xp = Math.min(Math.max(item.xp, v.minXp), v.maxXp);
  }
  if (item.ammo !== null && v.maxAmmo !== null) {
    item.ammo = to > from ? v.maxAmmo : Math.min(item.ammo, v.maxAmmo);
  }
}

/** Whether `f` differs from what the save holds. */
export function changed(view: Rac2SaveView, f: Rac2SaveFields): boolean {
  return JSON.stringify(view.fields) !== JSON.stringify(f);
}

/** Turns an item on or off; a weapon given with no ammo comes full, so it is ready to use. */
export function setOwned(view: Rac2SaveView, f: Rac2SaveFields, id: number, owned: boolean): void {
  const item = f.items.find((i) => i.id === id);
  const info = view.items.find((i) => i.id === id);
  if (!item || !info) return;
  if (!owned && info.worn) return;
  item.owned = owned;
  const { maxAmmo } = limits(view, f, id);
  if (owned && maxAmmo !== null && item.ammo === 0) item.ammo = maxAmmo;
}

/** Items of a kind owned, of how many. */
export function ownedCount(view: Rac2SaveView, f: Rac2SaveFields, kind: "weapon" | "gadget"): [number, number] {
  const ids = new Set(view.items.filter((i) => i.kind === kind).map((i) => i.id));
  const items = f.items.filter((i) => ids.has(i.id));
  return [items.filter((i) => i.owned).length, items.length];
}

/** Platinum bolts found, over every level. */
export function platinumCount(f: Rac2SaveFields): number {
  return f.planets.flatMap((p) => p.platinum).filter(Boolean).length;
}

/** The weapon's name in the version chosen in `f` (the Heavy Lancer from V2 on). */
export function versionName(view: Rac2SaveView, f: Rac2SaveFields, id: number): string | null {
  return selectedVersion(view, f, id)?.name ?? null;
}

/** One line of what the edits change, in a player's words, and the section it belongs to. */
export interface Rac2Edit {
  section: Rac2Section;
  text: string;
}

/** What `f` changes in the save, a line per kind of change, for the review before saving. */
export function editList(view: Rac2SaveView, f: Rac2SaveFields): Rac2Edit[] {
  const was = view.fields;
  const out: Rac2Edit[] = [];
  const add = (section: Rac2Section, label: string, names: string[]) => {
    if (names.length) out.push({ section, text: `${label}: ${names.join(", ")}` });
  };
  const from = (a: number, b: number) => `${num(a)} → ${num(b)}`;
  if (f.bolts !== was.bolts) out.push({ section: "ratchet", text: `Bolts ${from(was.bolts, f.bolts)}` });
  if (f.raritanium !== was.raritanium) {
    out.push({ section: "ratchet", text: `Raritanium ${from(was.raritanium, f.raritanium)}` });
  }
  if (f.nanotechXp !== was.nanotechXp) {
    out.push({ section: "ratchet", text: `Nanotech experience ${from(was.nanotechXp, f.nanotechXp)}` });
  }
  const boosts = (b: boolean[]) => b.filter(Boolean).length;
  if (f.boosts.join() !== was.boosts.join()) {
    out.push({
      section: "ratchet",
      text: `Nanotech Boosts ${String(boosts(was.boosts))} → ${String(boosts(f.boosts))}`,
    });
  }
  if (f.armor !== was.armor)
    out.push({ section: "ratchet", text: `Armour ${armorName(was.armor)} → ${armorName(f.armor)}` });
  if (f.shipPaint !== was.shipPaint) {
    out.push({ section: "ratchet", text: `Ship paint ${String(was.shipPaint + 1)} → ${String(f.shipPaint + 1)}` });
  }
  const name = (id: number) => view.items.find((i) => i.id === id)?.name ?? `Item ${String(id)}`;
  for (const kind of ["weapon", "gadget"] as const) {
    const section = kind === "weapon" ? "weapons" : "gadgets";
    const ids = view.items.filter((i) => i.kind === kind).map((i) => i.id);
    const pairs = ids.map((id) => [was.items.find((i) => i.id === id), f.items.find((i) => i.id === id)] as const);
    add(
      section,
      "Given",
      pairs.filter(([a, b]) => a && b && !a.owned && b.owned).map(([, b]) => name(b?.id ?? 0)),
    );
    add(
      section,
      "Taken",
      pairs.filter(([a, b]) => a && b && a.owned && !b.owned).map(([, b]) => name(b?.id ?? 0)),
    );
    add(
      section,
      "Ammo",
      pairs.filter(([a, b]) => a?.ammo !== b?.ammo).map(([, b]) => `${name(b?.id ?? 0)} ${String(b?.ammo)}`),
    );
    const version = (id: number, s: Rac2ItemState | undefined) => {
      const v = s?.version == null ? undefined : view.items.find((i) => i.id === id)?.versions[s.version];
      return v ? `${v.name} ${versionLabel(v)}` : "?";
    };
    add(
      section,
      "Version",
      pairs
        .filter(([a, b]) => a && b && a.version !== b.version)
        .map(([a, b]) => `${version(a?.id ?? 0, a)} → ${version(b?.id ?? 0, b)}`),
    );
    add(
      section,
      "Experience",
      pairs.filter(([a, b]) => a?.xp !== b?.xp).map(([, b]) => `${name(b?.id ?? 0)} ${num(b?.xp ?? 0)}`),
    );
  }
  const planet = (id: number) => view.planets.find((p) => p.id === id)?.name ?? `Level ${String(id)}`;
  const pp = f.planets.map((p) => [was.planets.find((q) => q.id === p.id), p] as const);
  add(
    "planets",
    "Available",
    pp.filter(([a, b]) => a && !a.available && b.available).map(([, b]) => planet(b.id)),
  );
  add(
    "planets",
    "No longer available",
    pp.filter(([a, b]) => a?.available && !b.available).map(([, b]) => planet(b.id)),
  );
  add(
    "planets",
    "Visited",
    pp.filter(([a, b]) => a?.visited !== b.visited).map(([, b]) => `${planet(b.id)} ${b.visited ? "yes" : "no"}`),
  );
  add(
    "planets",
    "Platinum bolts",
    pp.filter(([a, b]) => a?.platinum.join() !== b.platinum.join()).map(([, b]) => planet(b.id)),
  );
  const sp = (s: boolean[]) => s.filter(Boolean).length;
  if (f.skillPoints.join() !== was.skillPoints.join()) {
    out.push({ section: "skills", text: `Skill points ${String(sp(was.skillPoints))} → ${String(sp(f.skillPoints))}` });
  }
  for (const flag of f.flags) {
    const old = was.flags.find((x) => x.id === flag.id);
    const info = view.flags.find((x) => x.id === flag.id);
    if (!old || !info || old.on === flag.on) continue;
    const planet =
      info.level === null
        ? "Story"
        : (view.planets.find((p) => p.id === info.level)?.name ?? `Level ${String(info.level)}`);
    out.push({ section: "advanced", text: `${planet}: ${info.name} ${flag.on ? "set" : "cleared"}` });
  }
  return out;
}

/** The Advanced view's flags, grouped by planet in the planet list's order. */
export function flagGroups(view: Rac2SaveView): { planet: string; flags: Rac2SaveView["flags"] }[] {
  return [
    ...view.planets.map((p) => ({ planet: p.name, flags: view.flags.filter((f) => f.level === p.id) })),
    { planet: "Across the game", flags: view.flags.filter((f) => f.level === null) },
  ].filter((g) => g.flags.length > 0);
}

/** Armour by level, as the editor names it. */
export function armorName(level: number): string {
  return level === 0 ? "none" : `level ${String(level)}`;
}

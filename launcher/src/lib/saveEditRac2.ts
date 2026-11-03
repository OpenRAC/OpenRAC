// The Going Commando save editor's checks and summaries on the page: the same limits
// core/src/save_edit_rac2.rs enforces, so a field shows its problem before Save is pressed. The
// Rust side checks again before it writes.
import type { Rac2SaveFields, Rac2SaveView } from "./api";

/** The editor's sections, in page order. */
export type Rac2Section = "ratchet" | "weapons" | "gadgets" | "planets" | "skills" | "advanced";

/** Ship paint jobs: the game numbers them, 0 the one the ship starts with. */
export const SHIP_PAINTS = 18;
/** Armour levels: none, then the four the armour vendors sell. */
export const ARMOR_LEVELS = 5;

const whole = (n: number | null, max: number) => n !== null && Number.isInteger(n) && n >= 0 && n <= max;
const num = (n: number) => n.toLocaleString("en-US");

/** A problem with what was typed, by field ("bolts", "ammo-30", "xp-30"), in a player's words. */
export function fieldErrors(view: Rac2SaveView, f: Rac2SaveFields): Record<string, string> {
  const errors: Record<string, string> = {};
  if (!whole(f.bolts, view.maxBolts)) errors.bolts = `From 0 to ${num(view.maxBolts)}`;
  if (!whole(f.raritanium, view.maxBolts)) errors.raritanium = `From 0 to ${num(view.maxBolts)}`;
  if (!whole(f.nanotechXp, view.maxXp)) errors.nanotechXp = `From 0 to ${num(view.maxXp)}`;
  for (const item of f.items) {
    const info = view.items.find((i) => i.id === item.id);
    const was = view.fields.items.find((i) => i.id === item.id);
    if (info?.maxAmmo != null && item.ammo !== was?.ammo && !whole(item.ammo, info.maxAmmo)) {
      errors[`ammo-${String(item.id)}`] = `From 0 to ${num(info.maxAmmo)}`;
    }
    if (info?.maxXp != null && item.xp !== was?.xp && !whole(item.xp, info.maxXp)) {
      errors[`xp-${String(item.id)}`] = `From 0 to ${num(info.maxXp)}`;
    }
  }
  return errors;
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
  if (owned && info.maxAmmo !== null && item.ammo === 0) item.ammo = info.maxAmmo;
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

/** The name of the version of a weapon in the save. */
export function versionName(view: Rac2SaveView, id: number): string | null {
  const info = view.items.find((i) => i.id === id);
  if (!info?.version) return null;
  if (info.version === 1) return info.upgradeName ?? "Upgraded";
  return `${info.upgradeName ?? info.name} (later version ${String(info.version)})`;
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

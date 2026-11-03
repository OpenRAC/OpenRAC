// The Going Commando save editor's advanced fields on the page: the limits
// core/src/save_edit_rac2_extra.rs enforces, so a field shows its problem before Save is pressed.
// The Rust side checks again before it writes.
import type { Rac2ExtraFields, Rac2ExtraView } from "./api";

const whole = (n: number | null, max: number) => n !== null && Number.isInteger(n) && n >= 0 && n <= max;
const num = (n: number) => n.toLocaleString("en-US");

/** A problem with what was typed, by field, in a player's words. */
export function extraErrors(view: Rac2ExtraView, f: Rac2ExtraFields): Record<string, string> {
  const errors: Record<string, string> = {};
  if (!whole(f.crystals, view.maxGems)) errors.crystals = `From 0 to ${num(view.maxGems)}`;
  if (!whole(f.moonstones, view.maxGems)) errors.moonstones = `From 0 to ${num(view.maxGems)}`;
  if (f.platinumSpent !== view.fields.platinumSpent && !whole(f.platinumSpent, view.platinumFound)) {
    errors.platinumSpent =
      view.platinumFound === 0
        ? "No platinum bolts are found in this save yet"
        : `From 0 to ${num(view.platinumFound)}, the platinum bolts found`;
  }
  return errors;
}

/** Whether `f` differs from what the save holds. */
export function extraChanged(view: Rac2ExtraView, f: Rac2ExtraFields): boolean {
  return JSON.stringify(view.fields) !== JSON.stringify(f);
}

/** The weapons that take mods, each with its mods' places in the field list, in the game's order. */
export function modGroups(view: Rac2ExtraView): { item: number; name: string; owned: boolean; mods: number[] }[] {
  const groups: { item: number; name: string; owned: boolean; mods: number[] }[] = [];
  view.mods.forEach((m, k) => {
    const last = groups.at(-1);
    if (last?.item === m.item) last.mods.push(k);
    else groups.push({ item: m.item, name: m.itemName, owned: m.owned, mods: [k] });
  });
  return groups;
}

/** Platinum bolts the mods ticked here cost more than those in the save (negative: fewer). */
export function modCost(view: Rac2ExtraView, f: Rac2ExtraFields): number {
  return f.mods.reduce((sum, m, k) => {
    const was = view.fields.mods[k]?.bought ?? false;
    const price = view.mods[k]?.price ?? 0;
    return sum + (m.bought === was ? 0 : m.bought ? price : -price);
  }, 0);
}

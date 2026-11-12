// Mod save fields shown beside the save editor's own elements (a field's `attach` in its mod's
// `save_data`, core/src/mod_save_data.rs; docs/port/MODS.md, "A mod's save data"). An item
// field `as: "version"` continues the item's version picker with the field's values above its
// `min`; `as: "row"` adds a control to the item's row. A target the editor does not know leaves
// the field in the mod's own section. `requires: {version: "last"}` ties the field's values above
// `min` to the item's last game version: picking one moves the item there, and picking a game
// version puts the field back to `min`.
import type { ModSaveData, ModSaveField, Rac2SaveFields, Rac2SaveView } from "./api";
import { setVersion } from "./saveEditRac2";

/** The values being edited: per mod id, per field key. */
export type ModValues = Record<string, Record<string, number>>;

/** A mod's field attached to an item. */
export interface Attached {
  mod: ModSaveData;
  field: ModSaveField;
}

/** The attachments of one item, stacked in the mods' load order. */
export interface ItemAttachments {
  version: Attached[];
  row: Attached[];
}

/** Whether the editor shows `f` beside one of `itemIds` (else in its mod's section). */
export function isAttached(f: ModSaveField, itemIds: ReadonlySet<number>): boolean {
  const a = f.attach;
  return !!a && a.to === "item" && (a.as === "version" || a.as === "row") && itemIds.has(a.id);
}

/** The attached fields of every item that has some. */
export function attachments(mods: ModSaveData[], itemIds: ReadonlySet<number>): Map<number, ItemAttachments> {
  const out = new Map<number, ItemAttachments>();
  for (const mod of mods) {
    for (const field of mod.fields) {
      if (!isAttached(field, itemIds) || !field.attach) continue;
      const at = out.get(field.attach.id) ?? { version: [], row: [] };
      (field.attach.as === "version" ? at.version : at.row).push({ mod, field });
      out.set(field.attach.id, at);
    }
  }
  return out;
}

export function valueOf(values: ModValues, a: Attached): number {
  return values[a.mod.id]?.[a.field.key] ?? a.field.min;
}

function setValue(values: ModValues, a: Attached, v: number): void {
  values[a.mod.id] = { ...(values[a.mod.id] ?? {}), [a.field.key]: v };
}

/** A value's name: its choice, or the number. */
export function choiceName(f: ModSaveField, v: number): string {
  return f.choices?.[v] ?? String(v);
}

/** One option of a combined version picker: a game version, or a mod's tier. */
export type VersionOption =
  | { kind: "game"; value: string; index: number }
  | { kind: "mod"; value: string; attached: Attached; tier: number; name: string };

const modValue = (a: Attached, tier: number) => `m:${a.mod.id}:${a.field.key}:${String(tier)}`;

/** The mod tiers that follow the game's versions in an item's picker. */
export function tierOptions(version: Attached[]): VersionOption[] {
  const out: VersionOption[] = [];
  for (const a of version) {
    for (let t = a.field.min + 1; t <= a.field.max; t++) {
      out.push({ kind: "mod", value: modValue(a, t), attached: a, tier: t, name: choiceName(a.field, t) });
    }
  }
  return out;
}

/** The tiers set (above `min`) on an item: more than one is a conflict between mods. */
export function activeTiers(values: ModValues, version: Attached[]): { attached: Attached; tier: number }[] {
  return version
    .map((attached) => ({ attached, tier: valueOf(values, attached) }))
    .filter((x) => x.tier > x.attached.field.min);
}

/** What the picker shows as picked: the first mod tier set, else the game version. */
export function pickedValue(values: ModValues, version: Attached[], gameVersion: number | null): string {
  const t = activeTiers(values, version)[0];
  return t ? modValue(t.attached, t.tier) : `g:${String(gameVersion ?? 0)}`;
}

function lastVersion(view: Rac2SaveView, id: number): number | null {
  const n = view.items.find((i) => i.id === id)?.versions.length ?? 0;
  return n > 0 ? n - 1 : null;
}

/**
 * Applies a pick from an item's combined version picker. A game version puts every attached
 * tier back to its `min` (the picker then shows that version); a mod tier sets that field, puts
 * the item's other version tiers back, and, when the field requires the last version, moves the
 * item to it (as the game's own upgrade does, `setVersion`).
 */
export function pickVersion(
  view: Rac2SaveView,
  f: Rac2SaveFields,
  values: ModValues,
  id: number,
  version: Attached[],
  value: string,
): void {
  if (value.startsWith("g:")) {
    for (const a of version) setValue(values, a, a.field.min);
    setVersion(view, f, id, Number(value.slice(2)));
    return;
  }
  const pick = tierOptions(version).find((o) => o.value === value);
  if (pick?.kind !== "mod") return;
  for (const a of version) setValue(values, a, a === pick.attached ? pick.tier : a.field.min);
  const last = lastVersion(view, id);
  if (pick.attached.field.requires?.version === "last" && last !== null) setVersion(view, f, id, last);
}

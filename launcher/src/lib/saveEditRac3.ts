// The Up Your Arsenal save editor's page logic (SaveEditorRac3.svelte): what changed and what the
// core would refuse (core/src/save_edit_rac3.rs, validate and SaveFile::apply), so the page can
// say so before saving.
import type { Rac3SaveFields, Rac3SaveView } from "$lib/api";

/** Whether `fields` differs from the save as read. */
export function changedRac3(view: Rac3SaveView, fields: Rac3SaveFields): boolean {
  return (
    fields.bolts !== view.fields.bolts ||
    fields.weapons.some((w, i) => w.ammo !== view.fields.weapons[i]?.ammo)
  );
}

/** The fields the core would refuse, by key ("bolts", "ammo-<item>"), with why. */
export function fieldErrorsRac3(
  view: Rac3SaveView,
  fields: Rac3SaveFields,
): Record<string, string> {
  const out: Record<string, string> = {};
  const whole = (n: number) => Number.isInteger(n) && n >= 0;

  if (!whole(fields.bolts) || fields.bolts > view.maxBolts)
    out.bolts = `0 to ${view.maxBolts.toLocaleString("en-US")}`;
  fields.weapons.forEach((w, i) => {
    const info = view.weapons[i];

    if (!info || w.ammo === view.fields.weapons[i]?.ammo) return;
    if (!w.owned || !whole(w.ammo) || w.ammo > info.maxAmmo)
      out[`ammo-${String(w.id)}`] = `0 to ${String(info.maxAmmo)}`;
  });

  return out;
}

/** Seconds played as the game's Load page shows them: hours and minutes. */
export function playTime(seconds: number): string {
  const h = Math.floor(seconds / 3600);
  const m = Math.floor((seconds % 3600) / 60);

  return `${String(h)}:${String(m).padStart(2, "0")}`;
}

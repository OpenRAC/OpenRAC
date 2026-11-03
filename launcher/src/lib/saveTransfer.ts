// Saves carried between devices (core/src/save_transfer.rs): the words of the Saves panel's
// Export and Import dialogs, and the choice an import sends. No Svelte or Tauri here, so it can
// be tested.
import type { SaveArchiveSlot, SaveImportChoice, SaveImportSlot } from "./api";
import { grouped } from "./format";

export const TRANSFER = {
  exportTitle: "Export saves",
  exportLead:
    "Puts saves in one file you can copy to another computer or phone, and import there from its Saves panel.",
  exportAll: "All saves",
  exportButton: "Export…",
  importTitle: "Import saves",
  merge: "Bring in the slots ticked below",
  mergeLine: "The card's other slots stay as they are.",
  replace: "Replace every save on this card",
  replaceLine: "The card's saves become the file's.",
  intoEmpty: "Into an empty slot",
  backedUp: "Your current saves are backed up first: Backups has them.",
  importButton: "Import",
  cancel: "Cancel",
  reading: "Reading the file…",
  nothing: "This file holds no saves.",
} as const;

/** A slot in a player's words: "Slot 2 · Kerwan · 4 890 bolts". */
export function slotLine(s: Pick<SaveArchiveSlot, "slotIndex" | "planetName" | "bolts">): string {
  const parts = [`Slot ${s.slotIndex + 1}`, s.planetName ?? "Unknown planet"];
  if (s.bolts !== null) parts.push(`${grouped(s.bolts)} bolts`);
  return parts.join(" · ");
}

/** What importing a slot does to the card's own: "Replaces Kerwan, 4 890 bolts". */
export function replacesLine(s: SaveImportSlot): string {
  const r = s.replaces;
  if (!r) return TRANSFER.intoEmpty;
  const bolts = r.bolts !== null ? `, ${grouped(r.bolts)} bolts` : "";
  return `Replaces ${r.planetName ?? "a save"}${bolts}`;
}

/** A slot's key in the dialog's ticks. */
export function slotKey(s: Pick<SaveArchiveSlot, "folder" | "slotIndex">): string {
  return `${s.folder}/${s.slotIndex}`;
}

/** The slots ticked at first: the ones that land in an empty slot, so nothing is replaced unless
 *  the player ticks it; every slot when the card has none. */
export function firstTicks(slots: SaveImportSlot[]): Set<string> {
  return new Set(slots.filter((s) => !s.replaces).map((s) => slotKey(s.slot)));
}

/** The import's choice from the dialog: everything replaced, or the ticked slots merged. */
export function importChoice(replaceAll: boolean, slots: SaveImportSlot[], ticked: Set<string>): SaveImportChoice {
  if (replaceAll) return { replaceAll: true, slots: [] };
  return {
    replaceAll: false,
    slots: slots
      .filter((s) => ticked.has(slotKey(s.slot)))
      .map((s) => ({ folder: s.slot.folder, slotIndex: s.slot.slotIndex })),
  };
}

/** How many saves, in words: "1 save", "3 saves". */
export function savesCount(n: number): string {
  return n === 1 ? "1 save" : `${n} saves`;
}

/** The toast after an export: where the file went. */
export function exportedText(saved: string, count: number): string {
  return `Exported ${savesCount(count)} to ${saved}.`;
}

/** The toast after an import. */
export function importedText(replaceAll: boolean, slots: number, backedUp: boolean): string {
  const what = replaceAll ? "Replaced the card's saves with the file's" : `Imported ${savesCount(slots)}`;
  return `${what}.${backedUp ? " The saves that were there are in Backups." : ""}`;
}

import { describe, expect, it } from "vitest";

import type { SaveArchiveSlot, SaveImportSlot } from "./api";
import { grouped } from "./format";
import {
  copiedText,
  copyButtonText,
  copyLead,
  exportedText,
  firstTicks,
  importChoice,
  importedText,
  replacesLine,
  slotKey,
  slotLine,
  sourceLine,
} from "./saveTransfer";

const slot = (
  slotIndex: number,
  planetName: string | null,
  bolts: number | null,
): SaveArchiveSlot => ({
  folder: "BESCES-50916RATCHET",
  slotIndex,
  file: `save${slotIndex}.bin`,
  empty: false,
  planetId: null,
  planetName,
  bolts,
  timestamp: null,
  mods: null,
});

const batalia: SaveImportSlot = {
  slot: slot(1, "Batalia", 12650),
  replaces: slot(1, "Kerwan", 4890),
};
const hoven: SaveImportSlot = { slot: slot(2, "Hoven", 30210), replaces: null };
const incoming = [batalia, hoven];

describe("slotLine", () => {
  it("names the slot as people count, its planet and bolts", () => {
    expect(slotLine(slot(1, "Kerwan", 4890))).toBe(`Slot 2 · Kerwan · ${grouped(4890)} bolts`);
    expect(slotLine(slot(0, null, null))).toBe("Slot 1 · Unknown planet");
  });
});

describe("replacesLine", () => {
  it("says what the card loses, or that the slot is empty", () => {
    expect(replacesLine(batalia)).toBe(`Replaces Kerwan, ${grouped(4890)} bolts`);
    expect(replacesLine(hoven)).toBe("Into an empty slot");
  });
});

describe("firstTicks", () => {
  it("ticks only the slots that replace nothing", () => {
    expect([...firstTicks(incoming)]).toEqual(["BESCES-50916RATCHET/2"]);
  });
});

describe("importChoice", () => {
  it("sends the ticked slots for a merge", () => {
    const ticked = new Set([slotKey(batalia.slot)]);

    expect(importChoice(false, incoming, ticked)).toEqual({
      replaceAll: false,
      slots: [{ folder: "BESCES-50916RATCHET", slotIndex: 1 }],
    });
  });

  it("sends no slots when everything is replaced", () => {
    expect(importChoice(true, incoming, new Set())).toEqual({ replaceAll: true, slots: [] });
  });
});

describe("toasts", () => {
  it("say what was done", () => {
    expect(exportedText("~/Downloads/x.zip", 2)).toBe("Exported 2 saves to ~/Downloads/x.zip.");
    expect(importedText(false, 1, true)).toBe(
      "Imported 1 save. The saves that were there are in Backups.",
    );
    expect(importedText(true, 0, false)).toBe("Replaced the card's saves with the file's.");
    expect(copiedText("PAL", false, 1, true)).toBe(
      "Copied 1 save from the PAL disc. The saves that were there are in Backups.",
    );
    expect(copiedText("NTSC-U", true, 0, false)).toBe("Copied every save of the NTSC-U disc.");
  });
});

describe("copies between discs", () => {
  it("name the disc on the button when there is one, and in the choice", () => {
    expect(copyButtonText([{ region: "PAL" }])).toBe("Copy from PAL");
    expect(copyButtonText([{ region: "PAL" }, { region: "NTSC-J" }])).toBe("Copy from…");
    expect(sourceLine({ region: "PAL", saves: 2 })).toBe("PAL · 2 saves");
    expect(copyLead({ region: "PAL" })).toContain("PAL disc");
  });
});

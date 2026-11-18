import { describe, expect, it } from "vitest";
import { backupLabel, emptySlots, newestSave, savedSeconds, stampSeconds } from "./saves";

describe("emptySlots", () => {
  it("says one slot, a run, or a list", () => {
    expect(emptySlots([])).toBe("");
    expect(emptySlots([4])).toBe("Slot 4 is empty");
    expect(emptySlots([5, 3, 4])).toBe("Slots 3 to 5 are empty");
    expect(emptySlots([1, 3, 5])).toBe("Slots 1, 3 and 5 are empty");
  });
});

describe("stampSeconds", () => {
  it("reads the core's save time as local time", () => {
    expect(stampSeconds("2026-10-09 17:01:21")).toBe(new Date(2026, 9, 9, 17, 1, 21).getTime() / 1000);
  });

  it("gives null for what it cannot read", () => {
    expect(stampSeconds(null)).toBeNull();
    expect(stampSeconds("yesterday")).toBeNull();
  });
});

describe("backupLabel", () => {
  it("names a backup by its note", () => {
    expect(backupLabel({ note: "before the last boss" })).toBe("Before the last boss");
    expect(backupLabel({ note: "Before editing slot 2" })).toBe("Before editing slot 2");
    expect(backupLabel({ note: null })).toBe("Backup");
    expect(backupLabel({ note: "  " })).toBe("Backup");
  });
});

describe("savedSeconds", () => {
  it("takes the game's clock first, else the file's time", () => {
    const stamp = new Date(2026, 9, 9, 17, 1, 21).getTime() / 1000;
    expect(savedSeconds({ timestamp: "2026-10-09 17:01:21", modifiedMillis: 1 })).toBe(stamp);
    expect(savedSeconds({ timestamp: null, modifiedMillis: 5_500 })).toBe(5);
    expect(savedSeconds({ timestamp: null, modifiedMillis: null })).toBeNull();
  });
});

describe("newestSave", () => {
  const slot = (slotIndex: number, timestamp: string | null, modifiedMillis: number | null, used = true) => ({
    slotIndex,
    exists: used,
    isEmpty: !used,
    timestamp,
    modifiedMillis,
  });

  it("picks the slot the game saved last, not the file touched last", () => {
    const slots = [
      slot(0, "2026-10-01 10:00:00", 9_000_000_000_000),
      slot(1, "2026-10-05 10:00:00", 1_000),
      slot(2, "2026-12-01 10:00:00", null, false),
    ];
    expect(newestSave(slots)?.slotIndex).toBe(1);
  });

  it("is null without a save", () => {
    expect(newestSave([slot(0, null, null, false)])).toBeNull();
    expect(newestSave([])).toBeNull();
  });
});

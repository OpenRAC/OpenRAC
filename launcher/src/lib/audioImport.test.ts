import { describe, expect, it } from "vitest";
import { importOutcome } from "./audioImport";
import { AUDIO } from "./modsText";
import type { Job } from "./queue";

const lines = (...text: string[]) => text.map((line) => ({ line })) as Job["lines"];

describe("what the Audio tab says after an import", () => {
  it("says the pack is made", () => {
    expect(importOutcome(true, lines("Imported 1 sound(s) into x as y"))).toEqual({
      text: AUDIO.imported,
      tone: "ok",
    });
  });

  it("counts what was refused, the rest being imported", () => {
    const said = importOutcome(
      true,
      lines("refused: a.wav: not a WAV file", "refused: b.wav: 6 channels", "Imported 1"),
    );
    expect(said).toEqual({ text: AUDIO.importedRefused(2), tone: "warn" });
    expect(said.text).toContain("Imported: your sounds are a pack in My mods.");
    expect(said.text).toContain("2 files were refused");
  });

  it("says when nothing differs, which is no failure of the tool's", () => {
    const nothing = lines("Nothing to import: no file in the folder differs from what was exported.");
    expect(importOutcome(false, nothing)).toEqual({ text: AUDIO.nothing, tone: "warn" });
  });

  it("says a file was refused when that left nothing to import", () => {
    const said = importOutcome(
      false,
      lines("refused: a.wav: not a WAV file", "Nothing to import: (1 refused, above.)"),
    );
    expect(said).toEqual({ text: AUDIO.importRefused(1), tone: "err" });
    expect(said.text).toContain("Nothing imported: 1 file was refused.");
  });

  it("puts anything else in a player's words", () => {
    const said = importOutcome(false, lines("openrac-error: asset-invalid", "audio: x is not an audio export"));
    expect(said.tone).toBe("err");
    expect(said.text).not.toContain("openrac-error");
  });

  it("does not claim a pack was made when the import failed after refusing a file", () => {
    const said = importOutcome(false, lines("refused: a.wav: not a WAV file", "audio: could not write the pack"));
    expect(said.tone).toBe("err");
    expect(said.text).not.toContain("Imported:");
    expect(said.text).not.toContain("Nothing imported:");
  });
});

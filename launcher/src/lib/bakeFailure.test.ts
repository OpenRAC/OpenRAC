import { describe, expect, it } from "vitest";
import { bakeFailure } from "./bakeFailure";

const record = {
  version: 1,
  code: "input",
  title: "Lighting data is missing or out of date",
  explanation: "The exported level data cannot be read by this baker.",
  suggestion: "Read the levels from your disc again, then retry.",
  detail: "manifest format 3 (this baker reads 4)",
  level: "level_01",
  log: "C:/Game/lighting/probe-bake.log",
};
const line = (text: string) => ({ stream: "stderr" as const, line: text });

describe("bake failure notices", () => {
  it("keeps the last structured failure after progress and preserves the log action", () => {
    const job = { code: 1, lines: [line("Lighting 100%"), line("openrac-bake-error: " + JSON.stringify(record))] };
    expect(bakeFailure(job)).toMatchObject(record);
  });
  it("ignores malformed, unversioned and incorrectly typed records", () => {
    for (const value of ["{", JSON.stringify({ ...record, version: 2 }), JSON.stringify({ ...record, title: {} })]) {
      expect(bakeFailure({ code: -1, lines: [line("openrac-bake-error: " + value)] }).code).toBe("unreported");
    }
  });
  it("does not offer a file action for an old tool's generic failure", () => {
    const failure = bakeFailure({ code: 1, lines: [line("probe bake: could not open scene")] });
    expect(failure.detail).toContain("could not open scene");
    expect(failure.log).toBeUndefined();
    expect(failure.suggestion).toContain("Activity");
  });
  it("does not accept extra data as toast actions", () => {
    const failure = bakeFailure({
      code: 1,
      lines: [line("openrac-bake-error: " + JSON.stringify({ ...record, onclick: "bad" }))],
    });
    expect(failure).not.toHaveProperty("onclick");
  });
});

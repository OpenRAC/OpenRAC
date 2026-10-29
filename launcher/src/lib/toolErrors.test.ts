import { describe, expect, it } from "vitest";
import { ERROR_LINE, TOOL_ERRORS, UNKNOWN_ERROR, errorCode, toolFailure } from "./toolErrors";

const RAW =
  "extract: level 00: this PAL project predates the 60 Hz gameplay fix. Extract the levels again to back up the old project and create a 60 Hz project, then reapply your intended edits. Do not copy the whole old scene.";
const lines = (...l: string[]) => l.map((line) => ({ line }));

describe("toolFailure", () => {
  it("says a known code in a player's words, with the step that helps", () => {
    const f = toolFailure(lines("$ python3 editor/extract.py pack …", `${ERROR_LINE}project-outdated-gameplay`, RAW));
    expect(f.code).toBe("project-outdated-gameplay");
    expect(f.title).toBe(TOOL_ERRORS["project-outdated-gameplay"]?.title);
    expect(f.next).toBe("update");
    expect(f.line).not.toContain("Do not copy");
  });

  it("gives an unknown code, or none, the generic sentence", () => {
    expect(toolFailure(lines(`${ERROR_LINE}a-code-from-the-future`, "boom")).title).toBe(UNKNOWN_ERROR.title);
    const none = toolFailure(lines("Traceback (most recent call last):", "KeyError: 'x'"));
    expect(none.code).toBeNull();
    expect(none.title).toBe(UNKNOWN_ERROR.title);
  });

  it("keeps the tool's words for the details only, without markers or folders", () => {
    const f = toolFailure(
      lines(
        `${ERROR_LINE}file-locked`,
        "extract: [Errno 13] Permission denied: '/home/ana/OpenRAC/assets/levels/x.tscn'",
      ),
    );
    expect(f.details).toContain("Permission denied");
    expect(f.details).not.toContain("/home/ana");
    expect(f.details).not.toContain(ERROR_LINE);
  });

  it("never puts a tool's raw message in the sentence, whatever the code", () => {
    for (const code of [...Object.keys(TOOL_ERRORS), "unknown-code", ""]) {
      const f = toolFailure(lines(`${ERROR_LINE}${code}`, RAW));
      expect(`${f.title} ${f.line}`).not.toContain("extract:");
      expect(`${f.title} ${f.line}`).not.toContain("predates");
    }
  });

  it("reads the last code said", () => {
    expect(errorCode(lines(`${ERROR_LINE}disk-full`, "x", `${ERROR_LINE}file-error`))).toBe("file-error");
    expect(errorCode(lines("nothing"))).toBeNull();
  });

  it("has a sentence and a next step for every code", () => {
    for (const [code, e] of Object.entries(TOOL_ERRORS)) {
      expect(e.title.length, code).toBeGreaterThan(5);
      expect(e.line.endsWith("."), code).toBe(true);
      expect(e.line, code).not.toMatch(/[A-Za-z]:\\|\/home\/|Error|\.py\b/);
    }
  });
});

import { describe, expect, it } from "vitest";
import type { ResetPartView, ResetPreview } from "./api";
import {
  PART_TEXT,
  RESET,
  anything,
  blockedBy,
  chosenParts,
  editorDone,
  failureText,
  keptLine,
  partSize,
  theirParts,
  total,
} from "./editorReset";
import type { Job } from "./queue";

const part = (p: ResetPartView["part"], theirs: boolean, bytes: number, files = 1): ResetPartView => ({
  part: p,
  theirs,
  bytes,
  files,
  folders: files ? [`/x/${p}`] : [],
});

const editor: ResetPreview = {
  reset: "editor",
  parts: [
    part("projects", false, 800_000_000, 9000),
    part("exports", false, 40_000_000, 300),
    part("packed", true, 3_000_000, 4),
    part("overrides", true, 20_000_000, 80),
    part("backups", true, 0, 0),
  ],
};

function job(state: Job["state"]): Job {
  return {
    key: 1,
    scope: { kind: "version", key: "rac1/pal" },
    actionId: "editor-extract",
    title: "Extract the levels",
    state,
    id: 1,
    command: "cmd",
    lines: [],
    code: null,
    queuedAt: 0,
    startedAt: 0,
    endedAt: null,
    outputs: [],
    inputs: "",
  };
}

describe("what a reset removes", () => {
  it("keeps the player's own edits and packs unless they tick the box", () => {
    expect(chosenParts(editor, false).map((p) => p.part)).toEqual(["projects", "exports"]);
    expect(chosenParts(editor, true).map((p) => p.part)).toEqual([
      "projects",
      "exports",
      "packed",
      "overrides",
      "backups",
    ]);
    expect(theirParts(editor).map((p) => p.part)).toEqual(["packed", "overrides", "backups"]);
  });

  it("adds up what goes, and says when nothing is there", () => {
    expect(total(chosenParts(editor, false))).toEqual({ bytes: 840_000_000, files: 9300 });
    expect(total(chosenParts(editor, true)).bytes).toBe(863_000_000);
    expect(anything(chosenParts(editor, false))).toBe(true);
    expect(anything([part("projects", false, 0, 0)])).toBe(false);
    expect(partSize(part("backups", true, 0, 0))).toBe("not there");
    expect(partSize({ ...part("exports", false, 0, 0), folders: ["/x"] })).toBe("empty");
    expect(partSize(part("projects", false, 812_000_000))).toBe("812 MB");
  });

  it("names every part in a player's words, without a folder", () => {
    for (const { title, line } of Object.values(PART_TEXT)) {
      expect(title.length).toBeGreaterThan(0);
      expect(line.length).toBeLessThanOrEqual(120);
      expect(`${title} ${line}`).not.toMatch(/[\\/]|assets|\{data\}|overrides|probes/);
    }
  });

  it("says what stays", () => {
    expect(keptLine("levels", false)).toMatch(/levels in Godot.*packs and edits/);
    expect(keptLine("editor", false)).toMatch(/packed levels, imported edits and added packs/);
    expect(keptLine("editor", true)).not.toMatch(/packed|edits/);
  });
});

describe("when a reset can run", () => {
  it("waits while a job or the game runs", () => {
    expect(blockedBy([], false)).toBeNull();
    expect(blockedBy([job("succeeded"), job("failed")], false)).toBeNull();
    expect(blockedBy([job("running")], false)).toBe(RESET.busy);
    expect(blockedBy([job("queued")], false)).toBe(RESET.busy);
    expect(blockedBy([], true)).toBe(RESET.busy);
  });
});

describe("what a player reads", () => {
  it("never shows the core's own error", () => {
    const shown = [
      failureText("busy: wait for what is running to end", false),
      failureText("/home/p/OpenRAC/assets/levels/rac1/pal is a link, not a folder of OpenRAC's: left alone", false),
      failureText("cannot remove /home/p/x: Directory not empty (os error 66)", false),
      failureText(new Error("permission denied"), true),
    ];
    expect(shown).toEqual([RESET.busy, RESET.refused, RESET.failed, RESET.previewFailed]);
    for (const text of shown) expect(text).not.toMatch(/[\\/]|os error/);
  });

  it("reports the editor's reset in plain words", () => {
    expect(editorDone({ bytes: 858_000_000, files: 9732, folders: 2 })).toBe(
      "The level editor was reset (858 MB freed). Open a level to read fresh copies from your disc.",
    );
    expect(editorDone({ bytes: 0, files: 0, folders: 0 })).toMatch(/nothing to delete/);
  });

  it("keeps each line short", () => {
    for (const [key, text] of Object.entries(RESET)) {
      if (typeof text === "string") expect(text.length, key).toBeLessThanOrEqual(120);
    }
  });
});

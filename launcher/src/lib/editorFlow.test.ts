import { describe, expect, it } from "vitest";
import type { EditorState } from "./api";
import { editorStage, levelList, updateNote, updateNoteLine, type FlowInput } from "./editorFlow";
import type { Job } from "./queue";

const state = (over: Partial<EditorState> = {}): EditorState => ({
  levels: [0, 1, 2],
  outdated: [],
  unpacked: [],
  broken: [],
  packedAt: null,
  packedLevels: 0,
  packedOtherVersion: false,
  carried: [],
  backups: [],
  gameLevelsFound: true,
  formatKnown: true,
  ...over,
});

const job = (actionId: string): Job =>
  ({ key: 1, actionId, state: "running", lines: [], scope: { kind: "version", key: "rac1/pal" } }) as unknown as Job;

const input = (over: Partial<FlowInput> = {}): FlowInput => ({
  game: "rac1",
  state: state(),
  discFound: true,
  lacksGodot: false,
  early: false,
  extracting: null,
  packing: null,
  ...over,
});

describe("editorStage", () => {
  it("walks a new player from the disc to playing", () => {
    expect(editorStage(input({ discFound: false })).primary).toBe("set-up");
    expect(editorStage(input({ state: state({ levels: [] }), lacksGodot: true })).primary).toBe("get-godot");
    expect(editorStage(input({ state: state({ levels: [] }) })).kind).toBe("no-levels");
    expect(editorStage(input({ state: state({ levels: [] }) })).primary).toBe("prepare");
    expect(editorStage(input({ extracting: job("editor-extract") })).kind).toBe("extracting");
    expect(editorStage(input()).kind).toBe("ready");
    expect(editorStage(input()).primary).toBe("open");
    const edited = editorStage(input({ state: state({ unpacked: [0, 3] }) }));
    expect([edited.kind, edited.primary]).toEqual(["unpacked", "pack"]);
    expect(edited.line).toContain("Veldin");
    expect(editorStage(input({ packing: job("editor-pack") })).kind).toBe("packing");
    const packed = editorStage(input({ state: state({ packedAt: 1, packedLevels: 2 }) }));
    expect([packed.kind, packed.primary]).toEqual(["packed", "play"]);
  });

  it("says an outdated project before anything else, with Update project and what it does", () => {
    const s = editorStage(
      input({
        state: state({ outdated: [0, 1, 2].map((level) => ({ level, reason: "gameplay" as const })), unpacked: [1] }),
      }),
    );
    expect([s.kind, s.primary]).toEqual(["needs-update", "update"]);
    expect(s.title).toBe("Your levels need an update");
    expect(s.line).toContain("60 Hz");
    expect(s.line).not.toMatch(/copy the whole|scene|gameplay_source/);
    expect(s.detail).toContain("backup");
    // Some levels only: they are named.
    const some = editorStage(input({ state: state({ outdated: [{ level: 1, reason: "schema" }] }) }));
    expect(some.title).toBe("Some of your levels need an update");
    expect(some.line.startsWith("Novalis")).toBe(true);
  });

  it("an update without Godot still offers the update", () => {
    const s = editorStage(input({ lacksGodot: true, state: state({ outdated: [{ level: 0, reason: "schema" }] }) }));
    expect([s.primary, s.secondary]).toEqual(["update", ["get-godot"]]);
  });

  it("missing files offer Repair; packed for another disc offers packing again", () => {
    expect(editorStage(input({ state: state({ broken: [5] }) })).primary).toBe("repair");
    const other = editorStage(input({ state: state({ packedAt: 1, packedLevels: 1, packedOtherVersion: true }) }));
    expect([other.kind, other.primary]).toEqual(["repack", "pack"]);
  });

  it("an update in progress is said as one", () => {
    const s = editorStage(
      input({ extracting: job("editor-extract"), state: state({ outdated: [{ level: 0, reason: "schema" }] }) }),
    );
    expect(s.kind).toBe("updating");
  });

  it("early access has no pack or play step", () => {
    expect(editorStage(input({ early: true, state: state({ unpacked: [0] }) })).kind).toBe("ready");
    expect(editorStage(input({ early: true, state: state({ packedAt: 1, packedLevels: 1 }) })).primary).toBe("open");
  });
});

describe("the update's note", () => {
  it("counts what was carried and lists what to redo", () => {
    const note = updateNote("rac1", [
      {
        level: 0,
        carried: ["Bolt_0002: moved", "Bolt_0003: deleted"],
        notCarried: ["Camera_0001: moved"],
        failed: false,
      },
      { level: 1, carried: [], notCarried: [], failed: false },
    ]);
    expect(note).not.toBeNull();
    if (!note) return;
    expect(note.carried).toBe(2);
    expect(note.redo).toEqual(["Veldin: Camera_0001: moved"]);
    expect(updateNoteLine(note)).toBe(
      "2 levels updated, and 2 changes of yours carried over. 1 change could not be carried: redo it in Godot. Your old levels are kept in the backup folder.",
    );
    expect(updateNote("rac1", [])).toBeNull();
  });

  it("names levels in a sentence", () => {
    expect(levelList("rac1", [0])).toBe("Veldin");
    expect(levelList("rac1", [0, 1])).toBe("Veldin and Novalis");
    expect(levelList("rac1", [0, 1, 2, 3, 4])).toMatch(/ and 2 more$/);
  });
});

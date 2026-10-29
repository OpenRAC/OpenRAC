// The status panel rendered (svelte/server, no window): a failed step reads in a player's words,
// and the tool's own message is not on the page.
import { render } from "svelte/server";
import { beforeEach, describe, expect, it } from "vitest";
import EditorFlow from "../components/EditorFlow.svelte";
import type { GameView, VersionView } from "./api";
import { jobs } from "./app.svelte";
import type { Job } from "./queue";
import { TOOL_ERRORS, UNKNOWN_ERROR } from "./toolErrors";

const RAW =
  "extract: level 00: this PAL project predates the 60 Hz gameplay fix. Extract the levels again to back up the old project and create a 60 Hz project, then reapply your intended edits. Do not copy the whole old scene.";

const action = (id: string) => ({
  id,
  label: id,
  description: "",
  kind: "edit",
  area: "editor",
  state: "unverified",
  detached: false,
  docs: null,
  todo: null,
  thisPlatform: true,
  runnable: true,
  blockers: [],
  done: true,
  developer: false,
  warning: null,
});

const version = {
  key: "rac1/pal",
  game: "rac1",
  name: "pal",
  actions: ["editor-extract", "editor-open", "editor-pack", "port-levels"].map(action),
  status: { disc: { state: "found", path: "/discs/rac1.iso", expected: null, message: "" } },
} as unknown as VersionView;
const game = { id: "rac1", title: "Ratchet & Clank" } as unknown as GameView;

function failedPack(...lines: string[]): Job {
  return {
    key: 7,
    scope: { kind: "version", key: "rac1/pal" },
    actionId: "editor-pack",
    title: "Pack your edited levels",
    state: "failed",
    id: 3,
    command: "python3 editor/extract.py pack …",
    lines: lines.map((line) => ({ stream: "stderr" as const, line })),
    code: 1,
    queuedAt: 1,
    startedAt: 1,
    endedAt: 2,
    outputs: [],
    inputs: "",
  };
}

describe("EditorFlow", () => {
  beforeEach(() => {
    jobs.list.length = 0;
  });

  it("says a failed pack in a player's words and never the tool's message", () => {
    jobs.list.push(failedPack("openrac-error: project-outdated-gameplay", RAW));
    const { body } = render(EditorFlow, { props: { game, version } });
    expect(body).toContain(TOOL_ERRORS["project-outdated-gameplay"]?.title);
    expect(body).toContain("Update project");
    expect(body).toContain("Show details");
    expect(body).not.toContain("predates the 60 Hz gameplay fix");
    expect(body).not.toContain("Do not copy the whole old scene");
    expect(body).not.toContain("openrac-error");
  });

  it("an error without a code gets the generic sentence, still not the raw text", () => {
    jobs.list.push(failedPack("Traceback (most recent call last):", "KeyError: 'rc1_index'"));
    const { body } = render(EditorFlow, { props: { game, version } });
    expect(body).toContain(UNKNOWN_ERROR.title);
    expect(body).not.toContain("KeyError");
    expect(body).not.toContain("Traceback");
  });

  it("keeps Start over in the panel, under Troubleshoot", () => {
    const { body } = render(EditorFlow, { props: { game, version } });
    expect(body).toContain("Troubleshoot and start over");
    expect(body).toContain("Reset the level editor");
  });
});

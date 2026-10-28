import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import { acceptWarning, extractorError, forgetWarning, jobs } from "./app.svelte";
import { isBuilt, isPlaying, playerState, rebuildSteps } from "./player.svelte";
import type { Job } from "./queue";
import type { ActionView, GameView, VersionView } from "./api";

const version = { key: "rac1/pal", game: "rac1" } as VersionView;

function job(actionId: string): Job {
  return {
    key: 1,
    scope: { kind: "version", key: "rac1/pal" },
    actionId,
    title: actionId,
    state: "running",
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

describe("isPlaying", () => {
  beforeEach(() => {
    jobs.list.length = 0;
  });

  it("is true while the game runs", () => {
    jobs.list.push(job("play"));
    expect(isPlaying(version)).toBe(true);
  });

  it("is true while the game with the player's mods runs: the same program, the same card", () => {
    jobs.list.push(job("play-edited"));
    expect(isPlaying(version)).toBe(true);
  });

  it("is false while another job of the version runs", () => {
    jobs.list.push(job("port-build"));
    expect(isPlaying(version)).toBe(false);
  });

  it("is false when the job has ended", () => {
    const ended = job("play-edited");
    ended.state = "succeeded";
    jobs.list.push(ended);
    expect(isPlaying(version)).toBe(false);
  });
});

/** An action of rac1/pal for this platform, ready to run unless told otherwise. */
function step(id: string, more: Partial<ActionView> = {}): ActionView {
  return {
    id,
    thisPlatform: true,
    runnable: true,
    done: false,
    blockers: [],
    state: "unverified",
    ...more,
  } as ActionView;
}

function versionWith(actions: ActionView[]): VersionView {
  return { ...version, actions };
}

function gameOf(v: VersionView): GameView {
  const extracted = { version: v.key } as NonNullable<GameView["install"]>["extracted"];
  return { id: "rac1", versions: [v], install: { compiled: false, extracted } } as unknown as GameView;
}

describe("rebuild", () => {
  beforeEach(() => {
    jobs.list.length = 0;
  });

  it("builds from scratch where the platform builds", () => {
    const v = versionWith([step("port-build"), step("port-rebuild"), step("port-levels")]);
    expect(rebuildSteps(v)?.map((a) => a.id)).toEqual(["port-rebuild"]);
  });

  it("reads and checks the levels again where nothing is built (Android)", () => {
    const v = versionWith([step("port-levels"), step("port-levels-check", { runnable: false })]);
    expect(rebuildSteps(v)?.map((a) => a.id)).toEqual(["port-levels", "port-levels-check"]);
  });

  it("is not offered when it cannot run", () => {
    expect(rebuildSteps(versionWith([step("port-build", { runnable: false })]))).toBeNull();
    expect(rebuildSteps(versionWith([step("port-levels", { thisPlatform: false })]))).toBeNull();
  });

  it("counts the game built once its program is there, levels or not", () => {
    const v = versionWith([step("play", { done: true, runnable: false })]);
    expect(isBuilt(gameOf(v), v)).toBe(true);
  });

  it("turns a Play that cannot start into Rebuild when the game is not built", () => {
    const blockers = ["needs a check of the levels (Check the levels)"];
    const v = versionWith([
      step("play", { runnable: false, blockers }),
      step("port-levels", { done: true }),
      step("port-levels-check", { done: true }),
    ]);
    const state = playerState(gameOf(v), v);
    expect(state.kind).toBe("rebuild");
    expect(state.label).toBe("Rebuild");
    expect(state.disabled).toBe(false);
  });
});

describe("extractorError", () => {
  it("is the reason of the extractor's last error line", () => {
    const lines = [
      { line: "disc.iso: 3 files on the disc, boot executable SCES_509.16" },
      { line: "error 4001: the image is not a version of this game that OpenRAC knows (SLUS_123.45)" },
    ];
    expect(extractorError(lines)).toBe("The image is not a version of this game that OpenRAC knows (SLUS_123.45)");
  });

  it("is nothing when the extractor named no error", () => {
    expect(extractorError([{ line: "Traceback (most recent call last):" }])).toBeUndefined();
  });
});

describe("acceptWarning", () => {
  const scope = { kind: "version" as const, key: "rac2/ntsc" };
  const play = step("play", { kind: "play", warning: "For development only." });
  let answer = true;
  const confirm = vi.fn(() => answer);

  beforeEach(() => {
    // The browser preview asks with the page's confirm() (api.ts, ask).
    vi.stubGlobal("window", { confirm });
    confirm.mockClear();
    answer = true;
    forgetWarning(scope, "play");
  });
  afterEach(() => vi.unstubAllGlobals());

  it("starts an action that warns of nothing without asking", async () => {
    expect(await acceptWarning(scope, step("play", { kind: "play", warning: null }))).toBe(true);
    expect(confirm).not.toHaveBeenCalled();
  });

  it("asks before an action with a warning, with its words, and stops on Cancel", async () => {
    answer = false;
    expect(await acceptWarning(scope, play)).toBe(false);
    expect(confirm).toHaveBeenCalledWith("For development only.");
    answer = true;
    expect(await acceptWarning(scope, play)).toBe(true);
  });

  it("asks once for a build and the start that follows it, and again next time", async () => {
    expect(await acceptWarning(scope, play, true)).toBe(true);
    expect(await acceptWarning(scope, play)).toBe(true);
    expect(confirm).toHaveBeenCalledTimes(1);
    expect(await acceptWarning(scope, play)).toBe(true);
    expect(confirm).toHaveBeenCalledTimes(2);
  });
});

import { describe, expect, it } from "vitest";
import type { Review, ReviewMod } from "./api";
import {
  canApply,
  dateText,
  groupMods,
  looksLikeCode,
  matching,
  optionalMods,
  untrusted,
  valueText,
  versions,
} from "./modCodes";
import { MOD_CODES, ADD } from "./modsText";

const mod = (id: string, action: ReviewMod["action"], more: Partial<ReviewMod> = {}): ReviewMod => ({
  id,
  name: id,
  version: "1.0.0",
  action,
  from: null,
  source: "OpenRAC/rac1-mods",
  native: true,
  optional: false,
  why: null,
  nearest: null,
  fresh: false,
  published: null,
  ...more,
});

const review = (more: Partial<Review> = {}): Review => ({
  set: "{}",
  code: "K7M2-9QXR",
  name: "Ada's mods",
  created: "2026-10-31T12:00:00Z",
  game: "rac1",
  wrongGame: false,
  mods: [],
  disable: [],
  settings: [],
  manual: [],
  packs: [],
  ask: [],
  otherOpenrac: null,
  partial: false,
  nothing: false,
  ...more,
});

describe("what counts as a code", () => {
  it("takes a code however it is typed, a full code and a link", () => {
    for (const text of [
      "K7M2-9QXR",
      "k7m2-9qxr",
      "K7M29QXR",
      " K7M2 9QXR ",
      "ORMS1.eNpl",
      "openrac://modset/K7M2-9QXR",
      "openrac://modset/ORMS1.eNpl",
    ]) {
      expect(looksLikeCode(text), text).toBe(true);
    }
  });

  it("leaves a wrong letter to the core, which says which", () => {
    expect(looksLikeCode("K7M0-9QXR")).toBe(true);
  });

  it("does not take a link for one", () => {
    for (const text of [
      "OpenRAC/rac1-mods",
      "github.com/OpenRAC/rac1-mods",
      "https://github.com/OpenRAC/rac1-mods",
      "openrac://join/ABC",
      "K7M2-9QX",
      "K7M2-9QXR1",
      "",
    ]) {
      expect(looksLikeCode(text), text).toBe(false);
    }
  });
});

describe("a review's groups", () => {
  it("come in reading order, without what already matches or what is optional", () => {
    const groups = groupMods(
      review({
        mods: [
          mod("gone", "unavailable"),
          mod("keep", "keep"),
          mod("up", "update"),
          mod("new", "install"),
          mod("extra", "install", { optional: true }),
        ],
      }),
    );
    expect(groups.map((g) => g.key)).toEqual(["install", "update", "unavailable"]);
    expect(groups[0]?.mods.map((m) => m.id)).toEqual(["new"]);
    expect(groups[0]?.title).toBe(MOD_CODES.group.install);
  });

  it("show a suggestion only when it can be taken", () => {
    const r = review({
      mods: [
        mod("take", "install", { optional: true }),
        mod("have", "keep", { optional: true }),
        mod("gone", "unavailable", { optional: true }),
      ],
    });
    expect(optionalMods(r).map((m) => m.id)).toEqual(["take"]);
    expect(matching(r)).toBe(1);
  });
});

describe("Apply", () => {
  const url = "https://github.com/someone/mods";
  const asking = review({
    mods: [mod("a", "install", { source: "someone/mods" })],
    ask: [{ url, label: "someone/mods" }],
  });

  it("waits for a source the player has not said yes to", () => {
    expect(untrusted(asking, [], [])).toEqual([url]);
    expect(canApply(asking, [], [], false)).toBe(false);
    expect(canApply(asking, [], [url], false)).toBe(true);
  });

  it("only asks about a suggestion's source when it is ticked", () => {
    const r = review({
      mods: [mod("base", "install"), mod("a", "install", { source: "someone/mods", optional: true })],
      ask: [{ url, label: "someone/mods" }],
    });
    expect(canApply(r, [], [], false)).toBe(true);
    expect(canApply(r, ["a"], [], false)).toBe(false);
    expect(canApply(r, ["a"], [url], false)).toBe(true);
  });

  it("waits for the game to close", () => {
    expect(canApply(review(), [], [], true)).toBe(false);
    expect(canApply(review(), [], [], false)).toBe(true);
  });

  it("has nothing to do when everything matches, unless a suggestion is ticked", () => {
    const r = review({ nothing: true, mods: [mod("a", "install", { optional: true })] });
    expect(canApply(r, [], [], false)).toBe(false);
    expect(canApply(r, ["a"], [], false)).toBe(true);
  });

  it("is never offered for another game", () => {
    expect(canApply(review({ wrongGame: true }), [], [], false)).toBe(false);
  });
});

describe("the words", () => {
  it("show a change of version, and a plain version", () => {
    expect(versions(mod("a", "update", { from: "0.9.0" }))).toBe("0.9.0 → 1.0.0");
    expect(versions(mod("a", "install"))).toBe("1.0.0");
    expect(versions(mod("a", "enable", { from: "1.0.0" }))).toBe("1.0.0");
  });

  it("say settings as words", () => {
    expect(valueText(true)).toBe("On");
    expect(valueText(false)).toBe("Off");
    expect(valueText(3)).toBe("3");
    expect(valueText("hard")).toBe("hard");
  });

  it("read a date, and leave what is not one", () => {
    expect(dateText("2026-10-31T12:00:00Z", "en-GB")).toBe("31 Oct 2026");
    expect(dateText("whenever", "en-GB")).toBe("whenever");
  });

  it("never name a disc's region, and stay on a line", () => {
    for (const text of Object.values(MOD_CODES)) {
      if (typeof text !== "string") continue;
      expect(text).not.toMatch(/PAL|NTSC/);
      expect(text.length, text).toBeLessThanOrEqual(120);
    }
  });

  it("tell the field what it takes", () => {
    expect(ADD.linkPlaceholder).toContain("code");
  });
});

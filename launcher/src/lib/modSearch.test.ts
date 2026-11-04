// The Browse view's search on the shared fixture (core/tests/fixtures/index.json), the one the
// website's scorer is tested on too.
import { describe, expect, it } from "vitest";
import fixture from "../../core/tests/fixtures/index.json";
import { closest, distance, highlight, normalize, score, search, type Searchable } from "./modSearch";

const mods = fixture.mods as Searchable[];
const ids = (q: string) => search(mods, q).map((m) => m.id);

describe("normalize", () => {
  it("lowers, strips accents and reads & and R&C", () => {
    expect(normalize("Écran Über")).toBe("ecran uber");
    expect(normalize("Café")).toBe("cafe");
    expect(normalize("Tips & Tricks")).toBe("tips and tricks");
    expect(normalize("R&C mods")).toBe("ratchet and clank mods");
  });
});

describe("search", () => {
  it("ranks an exact name over a prefix over a word over a tag, author and description", () => {
    const m = (name: string, extra: Partial<Searchable> = {}): Searchable => ({
      id: name.toLowerCase().replace(/ /g, "-"),
      name,
      author: "",
      description: "",
      tags: [],
      aliases: [],
      ...extra,
    });
    const list = [
      m("Desc", { description: "camera stuff" }),
      m("Author", { author: "Camera Crew" }),
      m("Tagged", { tags: ["camera"] }),
      m("Free Camera"),
      m("Camera Tweaks"),
      m("Camera"),
    ];
    expect(search(list, "camera").map((x) => x.name)).toEqual([
      "Camera",
      "Camera Tweaks",
      "Free Camera",
      "Tagged",
      "Author",
      "Desc",
    ]);
  });

  it("ANDs the words and keeps everything for an empty query", () => {
    // The name first; the pack only says it in its description.
    expect(ids("camera tweaks")).toEqual(["camera-tweaks", "randomizer-pack"]);
    expect(ids("camera nothing-like-this")).toEqual([]);
    expect(search(mods, "  ")).toHaveLength(mods.length);
  });

  it("forgives one typo in names and tags, on words of four letters or more", () => {
    expect(ids("strafr")).toContain("lock-strafe");
    expect(ids("camrea")).toContain("camera-base");
    expect(ids("lok")).not.toContain("lock-strafe");
    for (const m of mods) expect(score(m, "zzzz")).toBe(0);
  });

  it("finds the closest name when nothing matches", () => {
    expect(closest(mods, "Lock strafr")?.id).toBe("lock-strafe");
    expect(closest(mods, "qqqqqqqqqqqqqqqqqqqqqq")).toBeNull();
  });
});

describe("distance", () => {
  it("is Damerau-Levenshtein", () => {
    expect(distance("abcd", "abdc")).toBe(1);
    expect(distance("abcd", "abcd")).toBe(0);
    expect(distance("abc", "xyz", 5)).toBe(3);
  });
});

describe("highlight", () => {
  it("marks the matching starts of words, keeping the text as it was", () => {
    expect(highlight("Lock-strafe for Camera", "cam")).toEqual([
      { text: "Lock-strafe for ", mark: false },
      { text: "Cam", mark: true },
      { text: "era", mark: false },
    ]);
    expect(highlight("Anything", "")).toEqual([{ text: "Anything", mark: false }]);
  });
});

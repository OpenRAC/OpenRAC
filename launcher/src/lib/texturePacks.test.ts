import { describe, expect, it } from "vitest";
import { packSummary, previewSummary, textureCount } from "./texturePacks";
import type { TexturePackView } from "./api";

const view = (over: Partial<TexturePackView>): TexturePackView => ({
  id: "rac2-ntsc",
  enabled: true,
  dumping: false,
  packCount: 0,
  packAnimated: 0,
  pack: [],
  dumpCount: 0,
  dump: [],
  ...over,
});

describe("textureCount", () => {
  it("counts in words", () => {
    expect(textureCount(1)).toBe("1 texture");
    expect(textureCount(1342)).toBe("1\u202f342 textures");
  });
});

describe("packSummary", () => {
  it("says what the pack holds and whether it is on", () => {
    expect(packSummary(view({}))).toMatch(/^Empty for now/);
    expect(packSummary(view({ packCount: 38, packAnimated: 2 }))).toBe(
      "38 textures, 2 of them animated. The game uses it from its next start.",
    );
    expect(packSummary(view({ packCount: 1, enabled: false }))).toBe(
      "1 texture. Off: the game shows its own pictures.",
    );
  });
});

describe("previewSummary", () => {
  const preview = {
    textures: 120,
    animated: 1,
    folders: [
      { path: "Interface", label: "Interface", count: 40 },
      { path: "Shared", label: "Shared", count: 80 },
    ],
    name: "Crisp HUD",
    current: 0,
    replacing: 0,
    skipped: 0,
  };
  it("names the pack, its folders and what it would replace", () => {
    expect(previewSummary(preview)).toBe("“Crisp HUD” has 120 textures (1 animated): Interface 40, Shared 80.");
    expect(previewSummary({ ...preview, name: null, animated: 0, current: 38, replacing: 12 })).toBe(
      "This pack has 120 textures: Interface 40, Shared 80. Your pack has 38 textures; 12 of them would be replaced if you add it to yours.",
    );
  });
});

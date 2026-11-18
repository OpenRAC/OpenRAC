import { describe, expect, it } from "vitest";
import { packSummary, refusesPack, replaceSummary, textureCount } from "./texturePacks";
import type { TexturePackView } from "./api";

const view = (over: Partial<TexturePackView>): TexturePackView => ({
  id: "rac2-ntsc",
  enabled: true,
  pcsx2Path: null,
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
  it("recognises an external PCSX2 pack when the OpenRAC pack is empty", () => {
    expect(packSummary(view({ pcsx2Path: "D:/HD pack/SCUS-97199/replacements" }))).toBe(
      "PCSX2 pack selected for the next game start.",
    );
    expect(packSummary(view({ pcsx2Path: "D:/HD pack/SCUS-97199/replacements", enabled: false }))).toBe(
      "Texture packs are off.",
    );
  });
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

describe("replaceSummary", () => {
  const preview = {
    textures: 120,
    animated: 1,
    folders: [{ path: "Interface", label: "Interface", count: 120 }],
    name: "Crisp HUD",
    current: 0,
    replacing: 0,
    skipped: 0,
  };
  it("names the pack and says the old one is kept", () => {
    expect(replaceSummary(preview)).toBe("“Crisp HUD” has 120 textures (1 animated). It becomes your texture pack.");
    expect(replaceSummary({ ...preview, name: null, animated: 0, current: 38 })).toBe(
      "This pack has 120 textures. It becomes your texture pack. Your pack (38 textures) is kept beside it.",
    );
  });
});

describe("refusesPack", () => {
  it("tells a texture pack that cannot be added from another kind of file", () => {
    expect(refusesPack("This pack is for Ratchet & Clank (PAL), not Going Commando (NTSC-U). Add it…")).toBe(true);
    expect(refusesPack("The pack has two pictures for the same texture (a.png and b.png).")).toBe(true);
    expect(refusesPack("The animation folder Shared/5d0c has no frames.")).toBe(true);
    expect(refusesPack("The pack's manifest cannot be read.")).toBe(true);
    expect(refusesPack("This .zip holds no texture pictures.")).toBe(false);
    expect(refusesPack("3 picture(s) in the pack are not named after a texture (for example tie_0004.png).")).toBe(
      false,
    );
  });
});

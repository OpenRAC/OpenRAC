import { describe, expect, it } from "vitest";
import { FALLBACK, THEMES, theme, themeStyle } from "./themes";

// Every game in the checkout has a theme: a new games/<id>/ needs one too.
const manifests = import.meta.glob<{ id: string }>("../../../games/*/game.json", { eager: true, import: "default" });

describe("themes", () => {
  it("covers every game in the checkout", () => {
    const ids = Object.values(manifests).map((g) => g.id);
    expect(ids.length).toBeGreaterThanOrEqual(4);
    for (const id of ids) expect(THEMES[id], `no theme for ${id} in src/lib/themes.ts`).toBeDefined();
  });

  it("falls back for an unknown game", () => {
    expect(theme("rac9")).toBe(FALLBACK);
  });

  it("writes the site's custom properties", () => {
    const style = themeStyle("rac1");
    expect(style).toContain("--bd: #747669");
    expect(style).toContain("--title-font: var(--font-brand)");
  });
});

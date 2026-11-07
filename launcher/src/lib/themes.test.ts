import { describe, expect, it } from "vitest";
import {
  ACCENT_PROPERTIES,
  ACCENTS,
  FALLBACK,
  THEMES,
  accentProperties,
  applyAccent,
  storedAccent,
  theme,
  themeStyle,
} from "./themes";

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
    expect(style).toContain("--tx: var(--gold)");
    expect(style).toContain("--title-font: var(--font-brand)");
  });

  it("gives each game but Ratchet & Clank an accent, and keeps the launcher's own for it", () => {
    expect(accentProperties("rac1")).toEqual([]);
    expect(accentProperties(null)).toEqual([]);
    for (const id of Object.keys(THEMES).filter((id) => id !== "rac1")) {
      expect(ACCENTS[id], `no accent for ${id}`).toBeDefined();
      const names = accentProperties(id).map(([name]) => name);
      expect(names.sort()).toEqual([...ACCENT_PROPERTIES].sort());
    }
  });

  it("gives an accent only the light under the bar, so every game wears the launcher's gold", () => {
    expect(ACCENT_PROPERTIES).toEqual(["--glow"]);
  });

  it("keeps the accent's game for the next start's first paint", () => {
    const kept = new Map<string, string>();
    globalThis.localStorage = {
      getItem: (k: string) => kept.get(k) ?? null,
      setItem: (k: string, v: string) => void kept.set(k, v),
      removeItem: (k: string) => void kept.delete(k),
    } as Storage;
    const props = new Map<string, string>();
    const root = {
      style: {
        setProperty: (k: string, v: string) => props.set(k, v),
        removeProperty: (k: string) => props.delete(k),
      },
    } as unknown as HTMLElement;

    applyAccent("rac2", root);
    expect(storedAccent()).toBe("rac2");
    expect([...props.keys()].sort()).toEqual([...ACCENT_PROPERTIES].sort());

    applyAccent(null, root);
    expect(storedAccent()).toBeNull();
    expect(props.size).toBe(0);
  });
});

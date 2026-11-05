// The website's links pasted into Add a mod.
import { describe, expect, it } from "vitest";
import { hubLink, isSiteLink } from "./siteLinks";

describe("a pasted hub link", () => {
  it("opens a mod's page as its install link", () => {
    for (const link of [
      "https://openrac.dev/mods/lock-strafe",
      "https://www.openrac.dev/mods/lock-strafe/",
      "http://OpenRAC.dev/mods/lock-strafe?from=x#top",
      "  openrac.dev/mods/lock-strafe ",
    ]) {
      expect(hubLink(link)).toBe("openrac://install/lock-strafe");
    }
    expect(hubLink("openrac://install/a@1.0.0")).toBe("openrac://install/a@1.0.0");
  });

  it("leaves everything else alone", () => {
    for (const link of [
      "https://openrac.dev/mods/submit",
      "https://openrac.dev/mods/set?mods=a",
      "https://openrac.dev/mods",
      "https://openrac.dev/mods/..",
      "https://openrac.dev/mods/a%2Fb",
      "https://openrac.dev/mods/a/b",
      "https://openrac.dev.evil.example/mods/a",
      "https://evil.example/openrac.dev/mods/a",
      "github.com/OpenRAC/openrac-mods",
    ]) {
      expect(hubLink(link)).toBeNull();
    }
  });

  it("knows the website's links from a repository's", () => {
    expect(isSiteLink("https://openrac.dev/mods/submit")).toBe(true);
    expect(isSiteLink("openrac.dev")).toBe(true);
    expect(isSiteLink("https://openrac.dev.evil.example/a")).toBe(false);
    expect(isSiteLink("github.com/OpenRAC/openrac-mods")).toBe(false);
  });
});

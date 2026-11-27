import { describe, expect, it } from "vitest";

import type { IndexMod } from "./api";
import { holdText, reportUrl, trustAuthor, trustKey, whyText } from "./modsText";
import { hiddenByCheck, modTrust, trustOf } from "./modTrust.svelte";

describe("the labels", () => {
  it("reads the index's trust, and none for an older index", () => {
    expect(trustKey("verified")).toBe("verified");
    expect(trustKey("")).toBeNull();
    expect(trustKey("something-new")).toBeNull();
  });

  it("hides only Not checked mods with the filter on", () => {
    modTrust.byId = {
      a: { trust: "unchecked" } as IndexMod,
      b: { trust: "trusted" } as IndexMod,
    };
    expect(trustOf("a")).toBe("unchecked");
    expect(trustOf("nowhere")).toBe("");
    expect(hiddenByCheck("a", true)).toBe(true);
    expect(hiddenByCheck("b", true)).toBe(false);
    expect(hiddenByCheck("nowhere", true)).toBe(false);
    expect(hiddenByCheck("a", false)).toBe(false);
  });

  it("words the consent, the holds and the report link", () => {
    expect(trustAuthor(["new-modder"])).toBe("I trust new-modder (asked once for each author)");
    expect(holdText({ kind: "newReach", new: ["Network"], had: [] })).toContain("Network");
    expect(whyText("HUD", { kind: "held", version: "1.1.0" })).toContain("waiting for OpenRAC");
    expect(reportUrl("a b")).toContain("template=report-mod.yml&mod=a%20b");
  });
});

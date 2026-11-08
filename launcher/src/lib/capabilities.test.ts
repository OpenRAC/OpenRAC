import { describe, expect, it } from "vitest";
import { supports } from "./capabilities";

describe("host capabilities", () => {
  it("preserves existing hosts without platform-specific declarations", () => {
    expect(supports({}, "mods")).toBe(true);
    expect(supports({}, "saveEditing")).toBe(true);
  });
  it("honours each host limitation without disabling other features", () => {
    const host = { capabilities: { mods: false, saveEditing: false, saveTransfer: false } };
    expect(supports(host, "mods")).toBe(false);
    expect(supports(host, "saveEditing")).toBe(false);
    expect(supports(host, "saveTransfer")).toBe(false);
    expect(supports(host, "openFolders")).toBe(true);
  });
});

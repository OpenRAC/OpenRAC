import { describe, expect, it } from "vitest";

import { isMap } from "./addMods";

describe("isMap", () => {
  it("knows a shared map by its name, also with .zip after it", () => {
    expect(isMap("/x/openrac-museum-ntsc.openrac-map")).toBe(true);
    expect(isMap("/Download/openrac-museum-ntsc.openrac-map.zip")).toBe(true);
    expect(isMap("C:\\maps\\Museum.OPENRAC-MAP")).toBe(true);
    expect(isMap("/x/lock-strafe.zip")).toBe(false);
    expect(isMap("/x/openrac-map.zip")).toBe(false);
  });
});

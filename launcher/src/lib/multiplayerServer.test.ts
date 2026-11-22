import { describe, expect, it } from "vitest";

import {
  hostProblem,
  isOfficial,
  isValidHost,
  OFFICIAL_HOST,
  OFFICIAL_PORT,
  portProblem,
  serverAddress,
  serverProblem,
} from "./multiplayerServer";

describe("multiplayer server", () => {
  it("takes host names and IP addresses, as the core does", () => {
    for (const good of [
      "game.openrac.dev",
      "localhost",
      "my-box",
      "192.168.1.20",
      "::1",
      "fe80::1",
      "a.b.c.",
      "x1",
    ])
      expect(isValidHost(good), good).toBe(true);
    for (const bad of [
      "",
      "-box",
      "box-",
      "a..b",
      "two words",
      "émile.dev",
      "a_b.dev",
      `${"a".repeat(64)}.dev`,
    ])
      expect(isValidHost(bad), bad).toBe(false);
  });

  it("says what is wrong with the host and the port", () => {
    expect(hostProblem(" ")).toBe("Type the server's host name or IP address.");
    expect(hostProblem("game.openrac.dev:7770")).toBe("Put the port in its own field.");
    expect(hostProblem("two words")).toBe("Not a host name or an IP address.");
    expect(hostProblem(" localhost ")).toBeNull();
    for (const bad of [0, 65536, 1.5, null, undefined, Number.NaN])
      expect(portProblem(bad), String(bad)).toBe("The port is a number from 1 to 65535.");
    expect(portProblem(1)).toBeNull();
    expect(portProblem(65535)).toBeNull();
    expect(serverProblem(OFFICIAL_HOST, OFFICIAL_PORT)).toBeNull();
    expect(serverProblem("localhost", 0)).toBe("The port is a number from 1 to 65535.");
  });

  it("writes the address the game gets, and knows the official server", () => {
    expect(serverAddress(OFFICIAL_HOST, OFFICIAL_PORT)).toBe("game.openrac.dev:7770");
    expect(serverAddress(" ::1 ", 7000)).toBe("[::1]:7000");
    expect(isOfficial("Game.OpenRAC.dev", 7770)).toBe(true);
    expect(isOfficial("game.openrac.dev", 7771)).toBe(false);
    expect(isOfficial("localhost", 7770)).toBe(false);
  });
});

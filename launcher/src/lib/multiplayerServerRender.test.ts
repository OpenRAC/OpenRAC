// Settings' Multiplayer server rendered (svelte/server, no window): the official hub by
// default with "Reset to official" off, the player's own server, and what is wrong with a bad one
// (which also turns "Test connection" off).
import { render } from "svelte/server";
import { describe, expect, it } from "vitest";

import MultiplayerServer from "../components/MultiplayerServer.svelte";

const html = (host: string, port: number) =>
  render(MultiplayerServer, { props: { host, port } }).body.replace(/<!--[^>]*-->/g, "");

/** Whether the button reading `label` is disabled. */
const disabled = (body: string, label: string) => {
  const button = [...body.matchAll(/<button[^>]*>([^<]*)<\/button>/g)].find(
    (m) => m[1]?.trim() === label,
  );

  if (!button) throw new Error(`no button ${label} in ${body}`);

  return /\sdisabled(=|\s|>)/.test(button[0]);
};

describe("the multiplayer server setting", () => {
  it("shows the official hub, with nothing to reset", () => {
    const body = html("hub.openrac.dev", 7177);

    expect(body).toContain("Multiplayer server");
    expect(body).toContain('value="hub.openrac.dev"');
    expect(body).toContain('value="7177"');
    expect(body).toContain("The official OpenRAC hub.");
    expect(disabled(body, "Reset to official")).toBe(true);
    expect(disabled(body, "Test connection")).toBe(false);
  });

  it("shows the player's own server, which can be reset", () => {
    const body = html("192.168.1.20", 7000);

    expect(body).toContain("Your own hub");
    expect(disabled(body, "Reset to official")).toBe(false);
    expect(disabled(body, "Test connection")).toBe(false);
  });

  it("says what is wrong, and tests nothing, until the server is right", () => {
    let body = html("hub.openrac.dev:7177", 7177);

    expect(body).toContain("Put the port in its own field.");
    expect(disabled(body, "Test connection")).toBe(true);
    body = html("localhost", 0);
    expect(body).toContain("The port is a number from 1 to 65535.");
    expect(disabled(body, "Test connection")).toBe(true);
  });
});

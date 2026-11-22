// The multiplayer server, as the launcher's core checks it (core/src/multiplayer.rs): what Settings
// tells the player while it is typed, so the save bar never sends a server the core refuses.

/** The official OpenRAC server (the OpenRAC/Server repository), the default. */
export const OFFICIAL_HOST = "game.openrac.dev";

export const OFFICIAL_PORT = 7770;

const LABEL = /^[A-Za-z0-9-]{1,63}$/;
const IPV4 = /^(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}$/;

/** Whether `host` is an IPv6 address (without brackets), as the URL parser reads one. */
function isIpv6(host: string): boolean {
  if (!host.includes(":") || !/^[0-9A-Fa-f:.]+$/.test(host)) return false;

  try {
    return new URL(`http://[${host}]/`).hostname.length > 2;
  } catch {
    return false;
  }
}

/** Whether `host` names a server: an IPv4 or IPv6 address, or a host name of dot-separated labels
 *  of letters, digits and hyphens (1 to 63 each, no hyphen at either end, 253 in all). */
export function isValidHost(host: string): boolean {
  if (IPV4.test(host) || isIpv6(host)) return true;

  const name = host.endsWith(".") ? host.slice(0, -1) : host;

  return (
    name.length > 0 &&
    name.length <= 253 &&
    name.split(".").every((l) => LABEL.test(l) && !l.startsWith("-") && !l.endsWith("-"))
  );
}

/** Why `host` is not a server's host, or null when it is one. */
export function hostProblem(host: string): string | null {
  const trimmed = host.trim();

  if (!trimmed) return "Type the server's host name or IP address.";
  if (trimmed.includes(":") && !isIpv6(trimmed) && !trimmed.startsWith("["))
    return "Put the port in its own field.";
  if (!isValidHost(trimmed)) return "Not a host name or an IP address.";

  return null;
}

/** Why `port` is not a port, or null when it is one (1 to 65535). An emptied number field is null. */
export function portProblem(port: number | null | undefined): string | null {
  if (port == null || !Number.isInteger(port) || port < 1 || port > 65535)
    return "The port is a number from 1 to 65535.";

  return null;
}

/** What is wrong with the server typed, if anything. */
export function serverProblem(host: string, port: number | null | undefined): string | null {
  return hostProblem(host) ?? portProblem(port);
}

/** `host:port`, with an IPv6 address in brackets, as the game gets it (`OPENRAC_SERVER`). */
export function serverAddress(host: string, port: number): string {
  const h = host.trim();

  return h.includes(":") ? `[${h}]:${port}` : `${h}:${port}`;
}

/** Whether the server is the official one. */
export function isOfficial(host: string, port: number | null | undefined): boolean {
  return host.trim().toLowerCase() === OFFICIAL_HOST && port === OFFICIAL_PORT;
}

// What the Browse view decides about each mod of the index (Mods.svelte, ModBrowse.svelte): which
// version would install here, the presets, and where its pictures and videos are. The resolver
// (core/src/resolver.rs) makes the real change set; this only sorts and labels the list.
import type { IndexImage, IndexMod, IndexVersion } from "./api";

/** This launcher's mod interface (core/src/plugins.rs API_MAJOR, API_MINOR). */
export const LAUNCHER_API: [number, number] = [1, 10];

export interface Here {
  game: string;
  disc: string;
  /** plugins::platform_key(): "linux-x86_64", "android-arm64", ... */
  platform: string;
  api: [number, number];
  prerelease: boolean;
}

export function apiFits(api: string | null, have: [number, number]): boolean {
  if (!api) return true;
  const [major, minor] = api.split(".").map(Number);
  return major === have[0] && (minor ?? Number.NaN) <= have[1];
}

/** The newest version that would install here, or null. Revoked versions never count. */
export function versionHere(m: IndexMod, here: Here, revoked: (id: string, v: string) => boolean): IndexVersion | null {
  return (
    m.versions.find(
      (v) =>
        (here.prerelease || !v.prerelease) &&
        !revoked(m.id, v.version) &&
        (v.games[here.game] ?? []).includes(here.disc) &&
        (m.type !== "native" || here.platform in v.platforms) &&
        apiFits(v.api, here.api),
    ) ?? null
  );
}

export type Preset = "compatible" | "installed" | "upgradeable" | "notInstalled" | "all";
export const PRESETS: Preset[] = ["compatible", "installed", "upgradeable", "notInstalled", "all"];

/** A row of the list: the mod, the version for here, and what is installed. */
export interface Row {
  mod: IndexMod;
  here: IndexVersion | null;
  installed: string | null;
}

function newer(a: string, b: string): boolean {
  const p = (v: string) => /^(\d+)\.(\d+)\.(\d+)(-.+)?$/.exec(v);
  const x = p(a);
  const y = p(b);
  if (!x || !y) return false;
  for (let i = 1; i <= 3; i++) if (Number(x[i]) !== Number(y[i])) return Number(x[i]) > Number(y[i]);
  return !x[4] && !!y[4];
}

export const upgradeable = (r: Row) => !!r.installed && !!r.here && newer(r.here.version, r.installed);

export function inPreset(r: Row, preset: Preset): boolean {
  switch (preset) {
    case "compatible":
      return !!r.here;
    case "installed":
      return !!r.installed;
    case "upgradeable":
      return upgradeable(r);
    case "notInstalled":
      return !r.installed;
    case "all":
      return true;
  }
}

/** The smallest picture at least `want` pixels wide (else the largest): its .webp address. */
export function imageUrl(base: string, image: IndexImage, want: number): string | null {
  if (!/^[0-9a-f]{12}$/.test(image.hash) || !base.startsWith("https://")) return null;
  const widths = [...image.widths].sort((a, b) => a - b);
  const w = widths.find((x) => x >= want) ?? widths[widths.length - 1];
  if (w === undefined) return null;
  return `${base.endsWith("/") ? base : `${base}/`}${image.hash}-${w}.webp`;
}

const YOUTUBE_ID = /^[\w-]{11}$/;

/** A video's still and its page: shown as a picture that opens the browser, never played here. */
export function youtube(id: string): { thumb: string; watch: string } | null {
  if (!YOUTUBE_ID.test(id)) return null;
  return { thumb: `https://i.ytimg.com/vi/${id}/hqdefault.jpg`, watch: `https://www.youtube.com/watch?v=${id}` };
}

/** The mod's page on the website. */
export const siteUrl = (id: string) => `https://openrac.dev/mods/${encodeURIComponent(id)}`;

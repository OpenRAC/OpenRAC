// What the mod code windows decide, apart from how they look (ModSetReview.svelte): which text is a
// code, how a review groups, and whether Apply can go. The real checks are the core's
// (core/src/modset.rs): nothing here is trusted by it.
import type { Review, ReviewMod } from "./api";
import { MOD_CODES as T } from "./modsText";

/** Whether a text is meant as a code, not a link: `K7M2-9QXR` in any case, with or without its
 *  dash, a full code, or its link. A wrong letter still counts: the core says which, at once. */
export function looksLikeCode(text: string): boolean {
  const t = text.trim();
  return t.startsWith("ORMS1.") || t.startsWith("openrac://modset/") || /^[a-z0-9]{4}[-\s]?[a-z0-9]{4}$/i.test(t);
}

/** A mod's line: its version, or from and to. */
export function versions(m: ReviewMod): string {
  return m.from && m.from !== m.version ? `${m.from} → ${m.version}` : m.version;
}

/** A setting's value, as words. */
export function valueText(value: unknown): string {
  if (value === true) return "On";
  if (value === false) return "Off";
  return String(value);
}

/** A date as a person reads it; what is not one, as it came. */
export function dateText(iso: string, locale?: string): string {
  const date = new Date(iso);
  if (Number.isNaN(date.getTime())) return iso;
  return date.toLocaleDateString(locale, { day: "numeric", month: "short", year: "numeric" });
}

export interface ReviewGroup {
  key: "install" | "update" | "downgrade" | "reinstall" | "enable" | "unavailable";
  title: string;
  mods: ReviewMod[];
}

const ORDER = ["install", "update", "downgrade", "reinstall", "enable", "unavailable"] as const;

/** The mods the set requires, by what happens to each, in the order a player reads them. The
 *  optional ones have their own list, and what already matches is only counted. */
export function groupMods(review: Review): ReviewGroup[] {
  return ORDER.map((key) => ({
    key,
    title: T.group[key],
    mods: review.mods.filter((m) => m.action === key && !m.optional),
  })).filter((g) => g.mods.length > 0);
}

/** The suggested mods that can be taken. */
export function optionalMods(review: Review): ReviewMod[] {
  return review.mods.filter((m) => m.optional && m.action !== "keep" && m.action !== "unavailable");
}

/** How many mods need nothing. */
export function matching(review: Review): number {
  return review.mods.filter((m) => m.action === "keep").length;
}

/** The sources still to say yes to, given what the player ticked: those of mods it will install. */
export function untrusted(review: Review, extras: string[], trusted: string[]): string[] {
  const needed = review.mods
    .filter((m) => m.action !== "keep" && m.action !== "unavailable" && (!m.optional || extras.includes(m.id)))
    .map((m) => m.source);
  return review.ask.filter((s) => needed.includes(s.label) && !trusted.includes(s.url)).map((s) => s.url);
}

/** Whether Apply can go, with nothing else in its way. */
export function canApply(review: Review, extras: string[], trusted: string[], running: boolean): boolean {
  if (running || review.wrongGame) return false;
  const something = !review.nothing || extras.length > 0;
  return something && untrusted(review, extras, trusted).length === 0;
}

// Links to the website pasted into Add a mod (AddMod.svelte): a mod's page opens the change set
// (lib/deepLink.svelte.ts) as its install link; any other page of the site says it isn't a mod.

const SITE_LINK = /^(?:https?:\/\/)?(?:www\.)?openrac\.dev(?:[/?#]|$)/i;
const MOD_PAGE = /^(?:https?:\/\/)?(?:www\.)?openrac\.dev\/mods\/([A-Za-z0-9._-]+)\/?(?:[?#].*)?$/i;
/** The site's pages under /mods/ that are not a mod. */
const NOT_MODS = new Set(["submit", "set"]);

/** Any link to the website, mod page or not. */
export const isSiteLink = (text: string) => SITE_LINK.test(text.trim());

/** A pasted link the change set can open: openrac:// as it is, a mod's page as its install link. */
export function hubLink(text: string): string | null {
  const link = text.trim();
  if (/^openrac:\/\//i.test(link)) return link;
  const id = MOD_PAGE.exec(link)?.[1];
  if (!id || id.includes("..") || NOT_MODS.has(id.toLowerCase())) return null;
  return `openrac://install/${id}`;
}

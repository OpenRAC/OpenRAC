// Search over the mod index for the Browse view, with no library (the index stays under a
// thousand mods): the scorer the website uses too, on the same fixture.
//
// - Text is lower-cased, accents stripped (NFKD), `&` read as "and", and "R&C" as "ratchet and
//   clank"; "and" itself never has to match.
// - The query's words are ANDed. Each scores by its best field: the whole name exactly, a name
//   that starts with it, a word of the name (or an old name) that does, a tag, the author, then
//   the description.
// - A word of 4 letters or more may be one typo off (Damerau-Levenshtein 1), in names and tags.
// - `highlight` cuts a text into marked and plain pieces, so the page draws <mark> with no HTML.

export interface Searchable {
  id: string;
  name: string;
  author: string;
  description: string;
  tags: string[];
  aliases: string[];
}

export function normalize(text: string): string {
  return text
    .normalize("NFKD")
    .replace(/[̀-ͯ]/g, "")
    .toLowerCase()
    .replace(/\br\s*&\s*c\b/g, "ratchet and clank")
    .replace(/&/g, " and ")
    .replace(/[^a-z0-9]+/g, " ")
    .trim();
}

const words = (text: string) =>
  normalize(text)
    .split(" ")
    .filter((w) => w && w !== "and");

/** Damerau-Levenshtein (optimal string alignment) distance, stopping early past `most`. */
export function distance(a: string, b: string, most = 1): number {
  if (Math.abs(a.length - b.length) > most) return most + 1;
  const w = b.length + 1;
  // One flat table, row by row: d[i * w + j] is the distance of a[..i] and b[..j].
  const d = new Array<number>((a.length + 1) * w).fill(0);
  const at = (i: number, j: number) => d[i * w + j] ?? 0;
  for (let i = 0; i <= a.length; i++) d[i * w] = i;
  for (let j = 0; j <= b.length; j++) d[j] = j;
  for (let i = 1; i <= a.length; i++) {
    for (let j = 1; j <= b.length; j++) {
      const cost = a[i - 1] === b[j - 1] ? 0 : 1;
      let v = Math.min(at(i - 1, j) + 1, at(i, j - 1) + 1, at(i - 1, j - 1) + cost);
      if (i > 1 && j > 1 && a[i - 1] === b[j - 2] && a[i - 2] === b[j - 1]) v = Math.min(v, at(i - 2, j - 2) + 1);
      d[i * w + j] = v;
    }
  }
  return at(a.length, b.length);
}

const near = (word: string, candidates: string[]) => word.length >= 4 && candidates.some((c) => distance(word, c) <= 1);

/** How well `mod` matches `query`: 0 for no match (every word must match somewhere). */
export function score(mod: Searchable, query: string): number {
  const q = words(query);
  if (q.length === 0) return 1;
  const name = normalize(mod.name);
  const nameWords = words(mod.name);
  const aliasWords = mod.aliases.flatMap(words);
  const tags = mod.tags.map(normalize);
  const author = normalize(mod.author);
  const description = words(mod.description);
  let total = name === q.join(" ") || normalize(mod.id) === q.join(" ") ? 1000 : 0;
  for (const w of q) {
    let best = 0;
    if (name.startsWith(w)) best = 100;
    else if (
      nameWords.some((n) => n.startsWith(w)) ||
      normalize(mod.id)
        .split(" ")
        .some((n) => n.startsWith(w))
    )
      best = 80;
    else if (aliasWords.some((n) => n.startsWith(w))) best = 70;
    else if (tags.some((t) => t.startsWith(w))) best = 50;
    else if (author.split(" ").some((a) => a.startsWith(w))) best = 30;
    else if (description.some((d) => d.startsWith(w))) best = 20;
    else if (near(w, [...nameWords, ...aliasWords])) best = 40;
    else if (near(w, tags)) best = 25;
    if (best === 0) return 0;
    total += best;
  }
  return total;
}

/** The mods that match, best first (then by name). An empty query keeps them all, in order. */
export function search<T extends Searchable>(mods: T[], query: string): T[] {
  if (words(query).length === 0) return mods;
  return mods
    .map((m) => ({ m, s: score(m, query) }))
    .filter((x) => x.s > 0)
    .sort((a, b) => b.s - a.s || a.m.name.localeCompare(b.m.name))
    .map((x) => x.m);
}

/** The name closest to a query that found nothing: "Did you mean …?". */
export function closest<T extends Searchable>(mods: T[], query: string): T | null {
  const q = normalize(query);
  let best: { m: T; d: number } | null = null;
  for (const m of mods) {
    const d = distance(q, normalize(m.name), 64);
    if (!best || d < best.d) best = { m, d };
  }
  return best && best.d <= Math.max(2, Math.floor(q.length / 2)) ? best.m : null;
}

/** `text` in pieces, the ones that match a query word marked (accents and case ignored). */
export function highlight(text: string, query: string): { text: string; mark: boolean }[] {
  const q = words(query);
  if (q.length === 0) return [{ text, mark: false }];
  const out: { text: string; mark: boolean }[] = [];
  // Word by word, keeping the spaces and punctuation between them as they were.
  for (const part of text.split(/(\s+)/)) {
    const plain = normalize(part).replace(/ /g, "");
    const hit = q.find((w) => plain.startsWith(w));
    if (hit && plain.length > 0) {
      const cut = Math.min(part.length, hit.length);
      out.push({ text: part.slice(0, cut), mark: true });
      if (cut < part.length) out.push({ text: part.slice(cut), mark: false });
    } else {
      out.push({ text: part, mark: false });
    }
  }
  // Neighbours of one kind joined.
  return out.reduce<{ text: string; mark: boolean }[]>((acc, p) => {
    const last = acc[acc.length - 1];
    if (last?.mark === p.mark) last.text += p.text;
    else acc.push({ ...p });
    return acc;
  }, []);
}

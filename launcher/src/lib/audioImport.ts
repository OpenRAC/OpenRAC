// What the Audio tab says after an import (AudioPanel.svelte), from what editor/audio.py printed: no
// Svelte here, so it can be tested.
import { AUDIO } from "./modsText";
import { toolFailure } from "./toolErrors";
import type { Job } from "./queue";

export function importOutcome(ok: boolean, lines: Job["lines"]): { text: string; tone: "ok" | "warn" | "err" } {
  const refused = lines.filter((l) => l.line.startsWith("refused: ")).length;
  if (ok) {
    return refused ? { text: AUDIO.importedRefused(refused), tone: "warn" } : { text: AUDIO.imported, tone: "ok" };
  }
  // Every file it found was refused, or none differed: no pack, but the tool stopped on purpose.
  if (lines.some((l) => l.line.startsWith("Nothing to import"))) {
    return refused ? { text: AUDIO.importRefused(refused), tone: "err" } : { text: AUDIO.nothing, tone: "warn" };
  }
  // In a player's words, never the tool's (lib/toolErrors.ts); Activity has what it printed.
  const failure = toolFailure(lines);
  return { text: `${failure.title}. ${failure.line}`, tone: "err" };
}

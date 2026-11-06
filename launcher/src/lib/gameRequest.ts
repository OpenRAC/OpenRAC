// A game a shortcut asked for (`--play`) that could not start: the launcher opens on its page
// instead of failing silently. The start's own request is read once (startup_game), one made to a
// launcher that was open already arrives as an `open-game` event.
import { api, onOpenGame } from "./api";

/** Where requests come from: the app's commands, or the tests' stand-in. */
export interface GameSource {
  startupGame: () => Promise<string | null>;
  onOpenGame: (handler: (key: string) => void) => Promise<() => void>;
}

/** Opens `open(key)` for each request. Returns the function that stops listening. */
export async function startGameRequests(
  open: (key: string) => void,
  source: GameSource = { startupGame: api.startupGame, onOpenGame },
): Promise<() => void> {
  const stop = await source.onOpenGame(open);
  const key = await source.startupGame().catch(() => null);
  if (key) open(key);
  return stop;
}

// The player's multiplayer name, as the launcher's core checks it (core/src/multiplayer.rs): what
// Settings tells the player while it is typed, so the save bar never sends one the core refuses.

/** The shortest and the longest name, in characters. */
export const NAME_MIN = 3;
export const NAME_MAX = 16;

const ALLOWED = /^[A-Za-z0-9_]*$/;

/** Why `name` is not a multiplayer name, or null when it is one. */
export function nameProblem(name: string): string | null {
  if (!ALLOWED.test(name)) return "Only letters, digits and underscores.";
  if (name.length < NAME_MIN) return `At least ${NAME_MIN} characters.`;
  if (name.length > NAME_MAX) return `At most ${NAME_MAX} characters.`;
  return null;
}

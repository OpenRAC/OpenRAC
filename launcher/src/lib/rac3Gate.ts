// The Up Your Arsenal switch in Settings, under Developer: rac3 is still in development, so its
// build, set-up and Play stay blocked (core/src/actions.rs, RAC3_GATE) until the player has
// confirmed three prompts, one after another. Cancelling any of them leaves the switch off.
// The words are here; components/Rac3Gate.svelte shows them.

export interface Rac3Prompt {
  title: string;
  lines: string[];
  confirm: string;
  cancel: string;
}

export const RAC3_SWITCH = {
  title: "Up Your Arsenal (development)",
  hint: "Lets the launcher build, set up and play Ratchet & Clank: Up Your Arsenal. It is not finished. Turning it on asks you a few questions first.",
};

/** As core/src/actions.rs (RAC3_GATE): why rac3's actions are blocked while the switch is off. */
export const RAC3_GATE = "needs the Up Your Arsenal switch in Settings, under Developer";

/** The one-line note under rac3's actions while the switch is off (as core's RAC3_GATE says it). */
export const RAC3_NOTE =
  "Up Your Arsenal is in development: turn it on in Settings, under Developer.";

export const RAC3_PROMPTS: Rac3Prompt[] = [
  {
    title: "This is a development build",
    lines: [
      "Up Your Arsenal is not finished. It crashes, it looks wrong, and whole parts of it are not there yet.",
      "Yes, even the part that looked fine a minute ago.",
    ],
    confirm: "Yes, it is a development build",
    cancel: "Back",
  },
  {
    title: "No bug reports on Discord",
    lines: [
      "We are not looking for bug reports. Not on Discord, not in a DM, not as a tiny little question.",
      "We know it is broken. That is what development means.",
    ],
    confirm: "I will not report bugs on Discord",
    cancel: "Back",
  },
  {
    title: "Do not bother the developers",
    lines: [
      "No, really. Read this again.",
      "Until we announce Up Your Arsenal in a stable release, you will not ping, message or nudge the developers about it. They are busy. We will tell you when it works.",
    ],
    confirm: "I will wait for the announcement",
    cancel: "Take me back",
  },
];

/**
 * Asks each prompt in turn: `ask` shows one and answers whether it was confirmed. The first
 * cancel ends it with false and the later prompts are never shown; only three confirmations give
 * true.
 */
export async function confirmRac3(
  ask: (prompt: Rac3Prompt, step: number, of: number) => Promise<boolean>,
): Promise<boolean> {
  for (const [i, prompt] of RAC3_PROMPTS.entries()) {
    if (!(await ask(prompt, i + 1, RAC3_PROMPTS.length))) return false;
  }

  return true;
}

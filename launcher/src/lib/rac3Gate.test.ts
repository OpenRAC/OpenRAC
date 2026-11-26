import { describe, expect, it } from "vitest";

import { confirmRac3, RAC3_PROMPTS } from "./rac3Gate";

/** Answers the prompts from `answers`, and records which were shown. */
function player(answers: boolean[]) {
  const shown: string[] = [];
  const ask = (p: { title: string }, step: number) => {
    shown.push(p.title);

    return Promise.resolve(answers[step - 1] ?? false);
  };

  return { shown, ask };
}

describe("confirmRac3", () => {
  it("has three prompts, each with its own title and buttons", () => {
    expect(RAC3_PROMPTS).toHaveLength(3);
    expect(new Set(RAC3_PROMPTS.map((p) => p.title)).size).toBe(3);

    for (const p of RAC3_PROMPTS) expect(p.confirm && p.cancel && p.lines.length).toBeTruthy();
  });

  it("turns the switch on after all three are confirmed, in order", async () => {
    const { shown, ask } = player([true, true, true]);

    expect(await confirmRac3(ask)).toBe(true);
    expect(shown).toEqual(RAC3_PROMPTS.map((p) => p.title));
  });

  it.each([1, 2, 3])("leaves it off when cancelled at step %i, and asks no more", async (step) => {
    const answers = [true, true, true].map((_, i) => i + 1 < step);
    const { shown, ask } = player(answers);

    expect(await confirmRac3(ask)).toBe(false);
    expect(shown).toHaveLength(step);
  });
});

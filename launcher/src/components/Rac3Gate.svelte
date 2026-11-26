<script lang="ts">
  // The three prompts that stand before the Up Your Arsenal switch (Settings, Developer): one
  // dialog at a time, each with a confirm and a back button. `run()` answers true only when all
  // three were confirmed; Back, Escape or a click beside the dialog answers false at once.
  import { modal } from "$lib/portal";
  import { confirmRac3, type Rac3Prompt } from "$lib/rac3Gate";

  let asking = $state<{ prompt: Rac3Prompt; step: number; of: number } | null>(null);
  let answer: ((ok: boolean) => void) | null = null;

  export function run(): Promise<boolean> {
    return confirmRac3(
      (prompt, step, of) =>
        new Promise<boolean>((resolve) => {
          answer = resolve;
          asking = { prompt, step, of };
        }),
    ).finally(() => {
      asking = null;
    });
  }

  function done(ok: boolean) {
    const resolve = answer;

    answer = null;
    resolve?.(ok);
  }
</script>

<svelte:window
  onkeydown={(e) => {
    if (asking && e.key === "Escape") done(false);
  }}
/>

{#if asking}
  <div class="overlay" role="presentation" use:modal>
    <button
      class="backdrop"
      onclick={() => {
        done(false);
      }}
      tabindex="-1"
      aria-label="Back"
    ></button>
    <div class="dialog" role="dialog" aria-modal="true" aria-labelledby="rac3-title" tabindex="-1">
      <small class="step">Question {asking.step} of {asking.of}</small>
      <h2 id="rac3-title">{asking.prompt.title}</h2>
      {#each asking.prompt.lines as line (line)}<p>{line}</p>{/each}
      <div class="actions">
        <button
          class="ghost"
          onclick={() => {
            done(false);
          }}>{asking.prompt.cancel}</button
        >
        <button
          class="primary"
          onclick={() => {
            done(true);
          }}>{asking.prompt.confirm}</button
        >
      </div>
    </div>
  </div>
{/if}

<style>
  .overlay {
    position: fixed;
    inset: 0;
    z-index: 100;
    display: flex;
    align-items: center;
    justify-content: center;
    padding: 24px 16px;
  }

  .backdrop {
    position: absolute;
    inset: 0;
    width: 100%;
    height: 100%;
    border: none;
    border-radius: 0;
    padding: 0;
    background: color-mix(in srgb, var(--ink-deep) 75%, transparent);
    backdrop-filter: blur(4px);
    cursor: default;
  }

  .dialog {
    position: relative;
    z-index: 1;
    width: 100%;
    max-width: 480px;
    max-height: 100%;
    overflow-y: auto;
    display: flex;
    flex-direction: column;
    gap: 10px;
    padding: 20px 24px;
    background: var(--card);
    --control: var(--control-card);
    --control-hi: var(--control-card-hi);
    border-radius: var(--radius);
    box-shadow: var(--shadow);
  }

  h2 {
    margin: 0;
    font-family: var(--font-head);
    font-size: 1.125rem;
    font-weight: 600;
    color: var(--text);
  }

  p {
    margin: 0;
    font-size: 0.875rem;
  }

  .step {
    font-size: 0.75rem;
    font-weight: 700;
    letter-spacing: 0.06em;
    text-transform: uppercase;
    color: var(--gold);
  }

  .actions {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    justify-content: flex-end;
    gap: 8px;
    margin-top: 8px;
  }
</style>

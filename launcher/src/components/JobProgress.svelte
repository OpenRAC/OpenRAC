<script lang="ts">
  // What the job under the Play button is doing: its step in plain words, a slim bar (a percentage
  // when the output says how far along it is, an animated bar when it does not) and, behind
  // "Details", the last lines it printed. A strip of fixed layout across the hero card: nothing
  // here changes the width of the button above it, and no line of output can push it wider.
  // An editor tool says its step and count itself (editor/progress_line.py): then the step is in
  // its words, the count names the level ("Kerwan, 3 of 18"), and once a few levels are done, how
  // long is left. Only a change of step is read out; the count is in the bar's value. `plain`: in
  // a panel of its own (the level editor's), without the strip's box.
  import { levelName } from "$lib/labels";
  import type { Job } from "$lib/queue";
  import {
    isGame,
    phaseLabel,
    progressPercent,
    readProgress,
    tailLines,
    timeLeft,
    toolProgress,
    type Sample,
  } from "$lib/progress";
  import Icon from "./Icon.svelte";

  let { job, now = Date.now(), plain = false }: { job: Job; now?: number; plain?: boolean } = $props();

  let open = $state(false);
  let box = $state<HTMLPreElement | null>(null);

  const tool = $derived(toolProgress(job.lines));
  const phase = $derived(job.state === "queued" || !tool ? phaseLabel(job, now) : tool.words);
  const game = $derived(job.scope.kind === "version" ? (job.scope.key.split("/")[0] ?? "") : "");
  /** "Kerwan, 3 of 18"; "3 of 18" before a level is named. */
  const count = $derived.by(() => {
    if (tool?.done == null || tool.total === null) return null;
    const of = `${tool.done} of ${tool.total}`;
    return tool.level !== null && game ? `${levelName(game, tool.level)}, ${of}` : of;
  });
  const progress = $derived(
    tool ? { percent: progressPercent(tool), note: null } : readProgress(job.actionId, job.lines),
  );

  // The first count seen of this step and the newest, with when: how long is left.
  let first = $state<Sample | null>(null);
  let latest = $state<Sample | null>(null);
  $effect(() => {
    if (tool?.done == null) return;
    const seen = { step: tool.step, done: tool.done, at: Date.now() };
    if (latest?.step === seen.step && latest.done === seen.done) return;
    if (first?.step !== seen.step || seen.done < first.done) first = seen;
    latest = seen;
  });
  const left = $derived(tool ? timeLeft(first, latest, tool.total) : (progress.left ?? null));
  const queued = $derived(job.state === "queued");
  /** The game itself has nothing to count: a bar would only wait for the end of the session. */
  const bar = $derived(!isGame(job));
  const text = $derived(tailLines(job.lines).join("\n"));
  const percent = $derived(progress.percent === null ? null : Math.round(progress.percent));

  // The newest line is in view while the box is open.
  $effect(() => {
    if (open && box && text.length > 0) box.scrollTop = box.scrollHeight;
  });
</script>

<div class="job" class:plain aria-live="off">
  <div class="top">
    <span class="phase" role="status">{phase}{queued ? "…" : ""}</span>
    {#if count}
      <span class="pct">{count}</span>
    {:else if percent !== null}
      <span class="pct">{percent} %</span>
    {:else if progress.note}
      <span class="pct">{progress.note}</span>
    {/if}
    <button class="ghost small toggle" onclick={() => (open = !open)} aria-expanded={open} aria-controls="job-details">
      Details<span class="caret" class:open aria-hidden="true"><Icon name="arrow" size={13} /></span>
    </button>
  </div>

  {#if bar}
    <div
      class="track"
      role="progressbar"
      aria-label={phase}
      aria-valuemin={0}
      aria-valuemax={100}
      aria-valuenow={percent ?? undefined}
      aria-valuetext={count ?? undefined}
    >
      {#if queued}
        <i style:width="0%"></i>
      {:else if percent !== null}
        <i style:width={`${progress.percent}%`}></i>
      {:else}
        <i class="moving"></i>
      {/if}
    </div>
  {/if}

  {#if left && !queued}
    <span class="left">{left}</span>
  {/if}

  {#if open}
    <pre id="job-details" class="mono" bind:this={box} aria-label="The last lines the task printed">{text ||
        "Nothing has been printed yet."}</pre>
  {/if}
</div>

<style>
  .job {
    box-sizing: border-box;
    align-self: stretch;
    min-width: 0;
    display: flex;
    flex-direction: column;
    gap: 8px;
    padding: 12px 16px;
    border-radius: 16px;
    background: color-mix(in srgb, var(--ink-deep) 72%, transparent);
    border: 1px solid var(--line);
    backdrop-filter: blur(6px);
  }

  .job.plain {
    padding: 0;
    border: 0;
    border-radius: 0;
    background: none;
    backdrop-filter: none;
  }

  /* In a panel the step's name is the message: it keeps its words, and the count goes under it
     when both do not fit. */
  .plain .top {
    flex-wrap: wrap;
    row-gap: 2px;
  }

  .plain .phase {
    min-width: min(100%, 14em);
  }

  .left {
    font-size: 0.8125rem;
    color: var(--soft);
  }

  .top {
    display: flex;
    align-items: center;
    gap: 12px;
    min-width: 0;
  }

  .phase {
    flex: 1 1 auto;
    min-width: 0;
    font-size: 0.875rem;
    font-weight: 600;
    color: var(--text);
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .pct {
    flex: none;
    font-size: 0.8125rem;
    font-variant-numeric: tabular-nums;
    color: var(--soft);
  }

  .toggle {
    flex: none;
  }

  .caret {
    display: inline-flex;
    transform: rotate(-90deg);
    transition: transform 0.15s;
  }

  .caret.open {
    transform: rotate(90deg);
  }

  .track {
    position: relative;
    height: 6px;
    overflow: hidden;
    border-radius: 3px;
    background: rgb(255 255 255 / 0.12);
  }

  .track > i {
    position: absolute;
    inset: 0 auto 0 0;
    border-radius: 3px;
    background: linear-gradient(to right, var(--tx2, var(--brand)), var(--tx, var(--amber)));
    transition: width 0.5s ease-out;
  }

  .track > i.moving {
    width: 35%;
    animation: slide 1.5s ease-in-out infinite;
  }

  @keyframes slide {
    from {
      transform: translateX(-100%);
    }
    to {
      transform: translateX(290%);
    }
  }

  pre {
    box-sizing: border-box;
    margin: 0;
    padding: 8px 12px;
    /* About eight lines, then it scrolls. */
    max-height: calc(8 * 1.45em + 16px);
    overflow: auto;
    border-radius: 10px;
    background: var(--ink-deep);
    color: var(--body);
    font-size: 0.75rem;
    line-height: 1.45;
    white-space: pre-wrap;
    overflow-wrap: anywhere;
  }
</style>

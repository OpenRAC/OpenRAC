<script lang="ts">
  // One game on the library page, styled like its card on the website
  // (openrac-site src/components/Progress.tsx), one row per version.
  import type { GameView, VersionView } from "$lib/api";
  import { app, openVersion } from "$lib/app.svelte";
  import { matchedLine, pct } from "$lib/format";
  import { DISC, regionLabel } from "$lib/labels";
  import { themeStyle } from "$lib/themes";
  import ProgressBar from "./ProgressBar.svelte";

  let { game, delay = 0 }: { game: GameView; delay?: number } = $props();

  const developer = $derived(app.config?.developer ?? false);

  /** A version in a player's words: can it be played, and what is missing. */
  function playerState(v: VersionView): { text: string; tone: "ok" | "warn" | "info" } {
    const play = v.actions.find((a) => a.id === "play-runtime");
    if (!play || play.state === "planned") return { text: "Not playable yet", tone: "info" };
    if (v.status.disc.state !== "found") return { text: "Add your disc", tone: "warn" };
    return { text: "Ready to play", tone: "ok" };
  }
</script>

<section class="card rise" style={`${themeStyle(game.id)}; animation-delay: ${delay}ms`} aria-label={game.title}>
  <div class="title box">
    {game.title}
    <small>{developer ? "decompilation" : "PC port"}{game.year ? ` · ${game.year}` : ""}</small>
  </div>

  {#each game.versions as v (v.key)}
    <button
      class="version"
      onclick={() => {
        openVersion(v.key);
      }}
      aria-label={`${v.title}, ${v.region}`}
    >
      <span class="row head">
        <span class="region box">{regionLabel(v.name)}</span>
        <span class="serial">{v.serial}</span>
        <span class="grow"></span>
        <span class="percent box">{v.progress ? pct(v.progress.percent) : "–"}</span>
      </span>
      {#if v.progress}
        <ProgressBar percent={v.progress.percent} label={`${v.title} (${v.region}) code matched`} />
        <span class="line"
          >{developer
            ? matchedLine(v.progress.matchedCode, v.progress.totalCode)
            : "of the game's code decompiled"}</span
        >
      {/if}
      <span class="row pills">
        {#if developer}
          <span class={`pill ${DISC[v.status.disc.state].tone}`}
            ><span class="dot"></span>{DISC[v.status.disc.state].text}</span
          >
          {#if v.inputs.length}
            <span class={`pill ${v.status.inputsReady ? "ok" : "warn"}`}>
              <span class="dot"></span>{v.status.inputsReady ? "Inputs placed" : "Inputs missing"}
            </span>
          {/if}
        {:else}
          {@const state = playerState(v)}
          <span class={`pill ${state.tone}`}><span class="dot"></span>{state.text}</span>
        {/if}
        <span class="grow"></span>
        <span class="open">Open →</span>
      </span>
    </button>
  {/each}
</section>

<style>
  .card {
    position: relative;
    display: flex;
    flex-direction: column;
    gap: 14px;
    padding: 22px;
    border-radius: 30px;
    overflow: hidden;
    background:
      radial-gradient(90% 70% at 70% 0, var(--from), transparent 70%), linear-gradient(160deg, var(--from), var(--to));
    box-shadow: var(--shadow);
  }

  .box {
    border: 5px solid var(--bd);
    background: var(--box);
    color: var(--tx);
    text-align: center;
    box-shadow: var(--shadow-sm);
  }

  .title {
    font-family: var(--title-font);
    font-weight: var(--title-weight);
    font-size: 24px;
    line-height: 1.2;
    padding: 12px 18px;
  }

  .title small {
    display: block;
    margin-top: 4px;
    font-size: 12px;
    letter-spacing: 0.04em;
    color: var(--tx2);
  }

  .version {
    display: flex;
    flex-direction: column;
    align-items: stretch;
    gap: 8px;
    text-align: left;
    border-radius: 18px;
    border: 1px solid rgb(255 255 255 / 0.1);
    background: rgb(0 0 0 / 0.28);
    padding: 12px 14px;
    color: var(--text);
    font-weight: 400;
  }

  .version:hover:not(:disabled) {
    border-color: var(--bd);
    background: rgb(0 0 0 / 0.38);
  }

  .head {
    gap: 10px;
  }

  .region {
    font-family: var(--title-font);
    font-weight: var(--title-weight);
    font-size: 13px;
    border-width: 3px;
    padding: 2px 10px;
  }

  .serial {
    font-family: var(--font-mono);
    font-size: 12px;
    color: var(--soft);
  }

  .percent {
    font-family: var(--title-font);
    font-weight: var(--title-weight);
    font-size: 18px;
    border-width: 3px;
    padding: 2px 12px;
    font-variant-numeric: tabular-nums;
  }

  .line {
    font-size: 12px;
    color: var(--tx2);
  }

  .pills {
    gap: 6px;
    flex-wrap: wrap;
  }

  .pills .pill {
    background: rgb(0 0 0 / 0.35);
  }

  .open {
    font-size: 13px;
    font-weight: 600;
    color: var(--tx);
  }
</style>

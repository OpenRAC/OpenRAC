<script lang="ts">
  // One game on the library page, styled like its card on the website
  // (openrac-site src/components/Progress.tsx), one row per version.
  import type { GameView, VersionView } from "$lib/api";
  import { app, openVersion } from "$lib/app.svelte";
  import { matchedLine, pct } from "$lib/format";
  import { DISC, regionLabel } from "$lib/labels";
  import { themeStyle } from "$lib/themes";
  import Icon from "./Icon.svelte";
  import ImportModal from "./ImportModal.svelte";
  import ProgressBar from "./ProgressBar.svelte";

  let { game, delay = 0 }: { game: GameView; delay?: number } = $props();

  let isCardHovered = $state(false);
  let activeImportVersion = $state<VersionView | null>(null);

  const GAME_MEDIA: Record<string, { bg: string; gif?: string; video?: string }> = {
    rac1: { bg: "/img/rac1-bg.webp", video: "/img/rac1-gameplay.mp4", gif: "/img/rac1-gameplay.gif" },
    rac2: { bg: "/img/gc-bg.webp", gif: "/img/rac2-gameplay.gif" },
    rac3: { bg: "/img/uya-bg.webp", gif: "/img/rac3-gameplay.gif" },
    rac4: { bg: "/img/deadlocked-bg.webp", gif: "/img/rac4-gameplay.gif" },
  };

  const media = $derived(GAME_MEDIA[game.id] ?? null);
  const developer = $derived(app.config?.developer ?? false);

  /** A version in a player's words: can it be played, and what is missing. */
  function playerState(v: VersionView): { text: string; tone: "ok" | "warn" | "info" } {
    const play = v.actions.find((a) => a.id === "play");
    if (!play || play.state === "planned") return { text: "Not playable yet", tone: "info" };
    if (v.status.disc.state !== "found") return { text: "Add your disc", tone: "warn" };
    return { text: "Ready to play", tone: "ok" };
  }
</script>

<section
  class="card rise"
  class:previewing={isCardHovered}
  style={`${themeStyle(game.id)}; animation-delay: ${delay}ms`}
  aria-label={game.title}
  onmouseenter={() => (isCardHovered = true)}
  onmouseleave={() => (isCardHovered = false)}
>
  {#if media}
    <div class="card-backdrop" aria-hidden="true">
      <img src={media.bg} alt="" class="backdrop-img static" class:dimmed={isCardHovered} />
      {#if media.video && isCardHovered}
        <video src={media.video} class="backdrop-img video active" autoplay loop muted playsinline></video>
      {:else if media.gif}
        <img src={media.gif} alt="" class="backdrop-img gif" class:active={isCardHovered} />
      {/if}
      <div class="backdrop-overlay"></div>
    </div>
  {/if}

  <div class="card-content">
    <div class="card-header">
      <div class="title" class:hovered={isCardHovered}>
        <span class="title-text">{game.title}</span>
        <div class="title-meta">
          <span class="title-sub">{developer ? "decompilation" : "PC port"}{game.year ? ` · ${game.year}` : ""}</span>
          {#if isCardHovered}
            <span class="preview-badge"><span class="pulse-dot"></span>Live preview</span>
          {/if}
        </div>
      </div>

      {#if game.versions.length === 1}
        <button
          type="button"
          class="import-pill-btn"
          onclick={() => (activeImportVersion = game.versions[0] ?? null)}
          title={`Import ISO for ${game.title}`}
        >
          <Icon name="disc" size={15} />
          <span>Import ISO</span>
        </button>
      {/if}
    </div>

    <div class="versions-grid">
      {#each game.versions as v (v.key)}
        <div class="version">
          <div
            class="version-main"
            role="button"
            tabindex="0"
            onclick={() => {
              openVersion(v.key);
            }}
            onkeydown={(e) => {
              if (e.key === "Enter" || e.key === " ") openVersion(v.key);
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
          </div>

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
            <button
              type="button"
              class="import-btn"
              onclick={() => (activeImportVersion = v)}
              title={`Import ISO for ${v.title} (${v.region})`}
            >
              <Icon name="disc" size={13} />
              <span>Import ISO</span>
            </button>
            <button
              type="button"
              class="open-btn"
              onclick={() => {
                openVersion(v.key);
              }}
              title={`Open ${v.title} (${v.region})`}
            >
              Open →
            </button>
          </span>
        </div>
      {/each}
    </div>
  </div>
</section>

{#if activeImportVersion}
  <ImportModal version={activeImportVersion} onclose={() => (activeImportVersion = null)} />
{/if}

<style>
  .card {
    position: relative;
    display: flex;
    flex-direction: column;
    padding: 24px;
    border-radius: 30px;
    overflow: hidden;
    background:
      radial-gradient(90% 70% at 70% 0, var(--from), transparent 70%), linear-gradient(160deg, var(--from), var(--to));
    box-shadow: var(--shadow);
  }

  .card-backdrop {
    position: absolute;
    inset: 0;
    pointer-events: none;
    overflow: hidden;
    z-index: 0;
  }

  .backdrop-img {
    position: absolute;
    inset: 0;
    width: 100%;
    height: 100%;
    object-fit: cover;
    pointer-events: none;
    transition: opacity 0.3s ease-in-out;
  }

  .backdrop-img.static {
    opacity: 0.42;
  }

  .backdrop-img.static.dimmed {
    opacity: 0;
  }

  .backdrop-img.gif,
  .backdrop-img.video {
    opacity: 0;
  }

  .backdrop-img.gif.active,
  .backdrop-img.video.active {
    opacity: 0.85;
  }

  .backdrop-overlay {
    position: absolute;
    inset: 0;
    background: linear-gradient(180deg, rgba(0, 0, 0, 0.25) 0%, rgba(0, 0, 0, 0.55) 100%);
    pointer-events: none;
  }

  .card-content {
    position: relative;
    z-index: 1;
    display: flex;
    flex-direction: column;
    gap: 16px;
  }

  .card-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 16px;
    flex-wrap: wrap;
  }

  .box {
    border: 5px solid var(--bd);
    background: var(--box);
    color: var(--tx);
    text-align: center;
    box-shadow: var(--shadow-sm);
  }

  .title {
    display: inline-flex;
    flex-direction: column;
    align-items: flex-start;
    padding: 12px 22px;
    border-radius: 18px;
    background: rgba(12, 14, 20, 0.68);
    backdrop-filter: blur(14px);
    -webkit-backdrop-filter: blur(14px);
    border: 1px solid rgba(255, 255, 255, 0.16);
    box-shadow: 0 6px 20px rgba(0, 0, 0, 0.35);
    cursor: pointer;
    transition:
      transform 0.2s ease,
      box-shadow 0.2s ease,
      border-color 0.2s ease,
      background 0.2s ease;
  }

  .title:hover,
  .title.hovered {
    transform: translateY(-1px);
    border-color: rgba(255, 255, 255, 0.35);
    background: rgba(16, 18, 28, 0.82);
    box-shadow:
      0 8px 28px rgba(0, 0, 0, 0.5),
      0 0 16px color-mix(in srgb, var(--bd) 35%, transparent);
  }

  .title-text {
    font-family: var(--title-font);
    font-weight: var(--title-weight);
    font-size: 24px;
    line-height: 1.25;
    color: #ffffff;
    text-shadow: 0 2px 8px rgba(0, 0, 0, 0.6);
    letter-spacing: 0.02em;
  }

  .title-meta {
    display: flex;
    align-items: center;
    gap: 8px;
    margin-top: 3px;
  }

  .title-sub {
    font-size: 12px;
    font-weight: 500;
    letter-spacing: 0.04em;
    color: var(--tx2);
  }

  .preview-badge {
    display: inline-flex;
    align-items: center;
    gap: 5px;
    font-size: 11px;
    font-weight: 600;
    color: #4ade80;
    background: rgba(74, 222, 128, 0.14);
    border: 1px solid rgba(74, 222, 128, 0.32);
    border-radius: 999px;
    padding: 1px 7px;
  }

  .pulse-dot {
    width: 6px;
    height: 6px;
    border-radius: 50%;
    background: #4ade80;
    box-shadow: 0 0 6px #4ade80;
  }

  .import-pill-btn {
    display: inline-flex;
    align-items: center;
    gap: 8px;
    border: 1px solid rgba(255, 255, 255, 0.18);
    background: rgba(18, 20, 28, 0.68);
    backdrop-filter: blur(14px);
    -webkit-backdrop-filter: blur(14px);
    color: #ffffff;
    font-family: var(--font-sans);
    font-size: 13px;
    font-weight: 600;
    padding: 8px 16px;
    border-radius: 12px;
    cursor: pointer;
    box-shadow: 0 4px 14px rgba(0, 0, 0, 0.3);
    transition:
      transform 0.15s ease,
      background 0.15s ease,
      border-color 0.15s ease;
  }

  .import-pill-btn:hover {
    transform: translateY(-1px);
    border-color: rgba(255, 255, 255, 0.4);
    background: rgba(30, 34, 48, 0.85);
  }

  .versions-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(360px, 1fr));
    gap: 12px;
  }

  .version {
    display: flex;
    flex-direction: column;
    align-items: stretch;
    gap: 10px;
    text-align: left;
    border-radius: 18px;
    border: 1px solid rgb(255 255 255 / 0.14);
    background: rgb(0 0 0 / 0.42);
    backdrop-filter: blur(8px);
    padding: 14px 16px;
    color: var(--text);
    font-weight: 400;
    transition:
      border-color 0.2s ease,
      background 0.2s ease;
  }

  .version:hover {
    border-color: var(--bd);
    background: rgb(0 0 0 / 0.55);
  }

  .version-main {
    display: flex;
    flex-direction: column;
    gap: 8px;
    cursor: pointer;
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
    gap: 8px;
    align-items: center;
    flex-wrap: wrap;
    border-top: 1px solid rgb(255 255 255 / 0.08);
    padding-top: 10px;
  }

  .pills .pill {
    background: rgb(0 0 0 / 0.45);
  }

  .import-btn {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    font-size: 12px;
    font-weight: 600;
    color: #fff;
    background: rgba(255, 255, 255, 0.12);
    border: 1px solid rgba(255, 255, 255, 0.2);
    border-radius: 8px;
    padding: 4px 10px;
    cursor: pointer;
    transition:
      background 0.15s ease,
      border-color 0.15s ease;
  }

  .import-btn:hover {
    background: rgba(255, 255, 255, 0.24);
    border-color: rgba(255, 255, 255, 0.4);
  }

  .open-btn {
    border: none;
    background: none;
    font-size: 13px;
    font-weight: 600;
    color: var(--tx);
    cursor: pointer;
    padding: 4px 8px;
    transition: transform 0.15s ease;
  }

  .open-btn:hover {
    transform: translateX(2px);
  }
</style>

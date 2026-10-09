<script lang="ts">
  // One version of a game: its card, what is in place, and its actions by
  // kind (set up, build, check, play, edit), then where it comes from.
  import ActionCard from "$components/ActionCard.svelte";
  import Icon from "$components/Icon.svelte";
  import ImportModal from "$components/ImportModal.svelte";
  import ProgressBar from "$components/ProgressBar.svelte";
  import { api } from "$lib/api";
  import { app, currentVersion, guard, openVersion } from "$lib/app.svelte";
  import { grouped, matchedLine, pct, size } from "$lib/format";
  import { DISC, INPUT, SECTIONS, regionLabel } from "$lib/labels";
  import { themeStyle } from "$lib/themes";

  let showImportModal = $state(false);
  const v = $derived(currentVersion());
  const game = $derived(app.library?.games.find((g) => g.id === v?.game) ?? null);
  const scope = $derived({ kind: "version" as const, key: v?.key ?? "" });
  const here = $derived(v?.actions.filter((a) => a.thisPlatform) ?? []);
  const elsewhere = $derived(v?.actions.filter((a) => !a.thisPlatform) ?? []);

  const GAME_MEDIA: Record<string, { bg: string; gif?: string; video?: string }> = {
    rac1: { bg: "/img/rac1-bg.webp", video: "/img/rac1-gameplay.mp4", gif: "/img/rac1-gameplay.gif" },
    rac2: { bg: "/img/gc-bg.webp", gif: "/img/rac2-gameplay.gif" },
    rac3: { bg: "/img/uya-bg.webp", gif: "/img/rac3-gameplay.gif" },
    rac4: { bg: "/img/deadlocked-bg.webp", gif: "/img/rac4-gameplay.gif" },
  };

  const media = $derived(v ? GAME_MEDIA[v.game] : null);
</script>

{#if !v || !game}
  <div class="page">
    <p class="muted">That version is not in this checkout.</p>
    <button onclick={() => (app.page = "library")}>Back to the games</button>
  </div>
{:else}
  <div class="page sections" style={themeStyle(v.game)}>
    {#if media}
      <div class="page-ambient-bg" aria-hidden="true">
        <img src={media.bg} alt="" />
      </div>
    {/if}

    <button class="ghost back" onclick={() => (app.page = "library")}><Icon name="arrow" size={16} />All games</button>

    <section class="banner rise" aria-label={`${v.title} (${v.region})`}>
      {#if media}
        <div class="banner-backdrop" aria-hidden="true">
          {#if media.video}
            <video src={media.video} class="banner-bg video" autoplay loop muted playsinline></video>
          {:else if media.gif}
            <img src={media.gif} alt="" class="banner-bg gif" />
          {:else}
            <img src={media.bg} alt="" class="banner-bg static" />
          {/if}
          <div class="banner-overlay"></div>
        </div>
      {/if}

      <div class="left">
        <div class="title">
          {v.title}
          <small>decompilation{game.year ? ` · ${game.year}` : ""} · {v.region}</small>
        </div>
        {#if game.versions.length > 1}
          <div class="versions" role="tablist" aria-label="Versions">
            {#each game.versions as other (other.key)}
              <button
                role="tab"
                aria-selected={other.key === v.key}
                class="box"
                class:on={other.key === v.key}
                onclick={() => {
                  openVersion(other.key);
                }}>{regionLabel(other.name)} · {other.serial}</button
              >
            {/each}
          </div>
        {/if}
      </div>
      <div class="right">
        {#if v.progress}
          <div class="row">
            <span class="box label">Progress</span>
            <span class="box value">{pct(v.progress.percent)}</span>
          </div>
          <ProgressBar percent={v.progress.percent} label={`${v.title} code matched`} />
          <p class="line">
            {matchedLine(v.progress.matchedCode, v.progress.totalCode)}
            {#if v.progress.matchedFunctions !== null && v.progress.totalFunctions !== null}
              · {grouped(v.progress.matchedFunctions)} of {grouped(v.progress.totalFunctions)} functions
            {/if}
            {#if v.progress.date}· {v.progress.date}{/if}
          </p>
        {:else}
          <p class="line">No progress report yet.</p>
        {/if}
      </div>
    </section>

    <section class="panel inputs">
      <div class="row">
        <div class="grow">
          <span class="sec-num"></span>
          <h2>Your disc</h2>
        </div>
        <button class="ghost small import-btn" onclick={() => (showImportModal = true)}>
          <Icon name="disc" size={14} /> Import ISO
        </button>
        <span class={`pill ${DISC[v.status.disc.state].tone}`}
          ><span class="dot"></span>{DISC[v.status.disc.state].text}</span
        >
      </div>
      <dl>
        <dt>Image</dt>
        <dd>
          <span class="mono">{v.disc?.file ?? "–"}</span>
          {#if v.disc?.size}<span class="dim">· {size(v.disc.size)}</span>{/if}
          <span class="muted">· {v.status.disc.message}</span>
        </dd>
        {#each v.status.inputs as input (input.path)}
          <dt>{INPUT[input.from] ?? input.from}</dt>
          <dd>
            <span class="mono">{input.path}</span>
            <span class={input.present ? "ok" : "warn"}>· {input.message}</span>
          </dd>
        {/each}
      </dl>
    </section>

    <div class="kinds">
      {#each SECTIONS as section (section.kind)}
        {@const list = here.filter((a) => a.kind === section.kind)}
        {#if list.length}
          <section class="panel">
            <span class="sec-num"></span>
            <h2>{section.title}</h2>
            <p class="muted blurb">{section.blurb}</p>
            <div class="actions">
              {#each list as action (action.id)}
                <ActionCard {action} {scope} icon={section.icon} />
              {/each}
            </div>
          </section>
        {/if}
      {/each}
    </div>

    {#if elsewhere.length}
      <details class="panel other">
        <summary>{elsewhere.length} more for other platforms</summary>
        <div class="actions">
          {#each elsewhere as action (action.id)}
            <ActionCard {action} {scope} icon="terminal" />
          {/each}
        </div>
      </details>
    {/if}

    <section class="panel about">
      <span class="sec-num"></span>
      <h2>About this version</h2>
      <dl>
        {#if v.target}<dt>Target</dt>
          <dd>{v.target}</dd>{/if}
        {#if v.buildHost}<dt>Builds on</dt>
          <dd>{v.buildHost}</dd>{/if}
        {#if v.source}
          {@const repo = v.source.repo}
          <dt>From</dt>
          <dd>
            <button class="link" onclick={() => void guard(api.openUrl(repo))}>{v.source.name}</button>
            {#if v.source.commit}<span class="mono dim">@{v.source.commit.slice(0, 7)}</span>{/if}
            {#if v.source.date}<span class="dim">· {v.source.date}</span>{/if}
          </dd>
          {#if v.source.license}<dt>License</dt>
            <dd>{v.source.license}</dd>{/if}
        {/if}
        {#if v.progress?.note}<dt>Counted</dt>
          <dd>{v.progress.note}</dd>{/if}
        {#if v.setup}<dt>By hand</dt>
          <dd class="muted">{v.setup}</dd>{/if}
      </dl>
      <div class="row">
        {#if v.readme}
          {@const readme = v.readme}
          <button onclick={() => void guard(api.openPath(readme))}><Icon name="book" size={16} />README</button>
        {/if}
        <button onclick={() => void guard(api.openPath(v.dir))}><Icon name="folder" size={16} />Open the folder</button>
      </div>
    </section>

    {#if showImportModal}
      <ImportModal version={v} onclose={() => (showImportModal = false)} />
    {/if}
  </div>
{/if}

<style>
  .page {
    position: relative;
    max-width: 1180px;
    margin: 0 auto;
    padding: 20px 28px 60px;
    display: flex;
    flex-direction: column;
    gap: 20px;
    z-index: 1;
  }

  .page-ambient-bg {
    position: fixed;
    top: 0;
    left: 0;
    right: 0;
    height: 540px;
    pointer-events: none;
    overflow: hidden;
    z-index: 0;
    opacity: 0.18;
    mask-image: linear-gradient(180deg, rgba(0, 0, 0, 0.9) 0%, rgba(0, 0, 0, 0) 100%);
    -webkit-mask-image: linear-gradient(180deg, rgba(0, 0, 0, 0.9) 0%, rgba(0, 0, 0, 0) 100%);
  }

  .page-ambient-bg img {
    width: 100%;
    height: 100%;
    object-fit: cover;
    filter: blur(32px);
    transform: scale(1.1);
  }

  .back {
    align-self: flex-start;
  }

  .banner {
    position: relative;
    overflow: hidden;
    display: grid;
    grid-template-columns: minmax(0, 1.1fr) minmax(0, 1fr);
    gap: 26px;
    align-items: center;
    padding: 28px;
    border-radius: 30px;
    background:
      radial-gradient(90% 70% at 70% 0, var(--from), transparent 70%), linear-gradient(160deg, var(--from), var(--to));
    box-shadow: var(--shadow);
  }

  .banner-backdrop {
    position: absolute;
    inset: 0;
    pointer-events: none;
    overflow: hidden;
    z-index: 0;
  }

  .banner-bg {
    position: absolute;
    inset: 0;
    width: 100%;
    height: 100%;
    object-fit: cover;
    opacity: 0.44;
  }

  .banner-overlay {
    position: absolute;
    inset: 0;
    background: linear-gradient(180deg, rgba(0, 0, 0, 0.25) 0%, rgba(0, 0, 0, 0.65) 100%);
    pointer-events: none;
  }

  .left,
  .right {
    position: relative;
    z-index: 1;
  }

  .box {
    border: 7px solid var(--bd);
    background: var(--box);
    color: var(--tx);
    text-align: center;
    box-shadow: var(--shadow-sm);
  }

  .title {
    font-family: var(--title-font);
    font-weight: var(--title-weight);
    font-size: clamp(24px, 3vw, 36px);
    line-height: 1.15;
    padding: 16px 24px;
    border-radius: 20px;
    background: rgba(12, 14, 20, 0.65);
    backdrop-filter: blur(14px);
    -webkit-backdrop-filter: blur(14px);
    border: 1px solid rgba(255, 255, 255, 0.16);
    box-shadow: 0 8px 30px rgba(0, 0, 0, 0.4);
    color: #ffffff;
    text-shadow: 0 2px 10px rgba(0, 0, 0, 0.6);
  }

  .title small {
    display: block;
    margin-top: 6px;
    font-size: 13px;
    letter-spacing: 0.04em;
    color: var(--tx2);
  }

  .left {
    display: flex;
    flex-direction: column;
    gap: 12px;
  }

  .versions {
    display: flex;
    gap: 10px;
  }

  .versions button {
    flex: 1;
    border-width: 4px;
    border-radius: 0;
    font-family: var(--title-font);
    font-weight: var(--title-weight);
    font-size: 13px;
    opacity: 0.6;
    padding: 6px 10px;
  }

  .versions button.on,
  .versions button:hover {
    opacity: 1;
    border-color: var(--bd);
  }

  .right {
    display: flex;
    flex-direction: column;
    gap: 12px;
  }

  .right .row {
    gap: 14px;
  }

  .label,
  .value {
    font-family: var(--title-font);
    font-weight: var(--title-weight);
    padding: 8px 14px;
  }

  .label {
    flex: 1;
    font-size: 20px;
  }

  .value {
    flex: 2;
    font-size: 26px;
    font-variant-numeric: tabular-nums;
  }

  .line {
    font-size: 13px;
    color: var(--tx2);
    text-align: center;
  }

  .panel h2 {
    margin: 4px 0 2px;
  }

  .blurb {
    font-size: 14px;
    margin-bottom: 14px;
  }

  /* The action sections two by two on a wide window. */
  .kinds {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(480px, 1fr));
    align-items: start;
    gap: 20px;
  }

  .actions {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(260px, 1fr));
    gap: 14px;
  }

  dl {
    display: grid;
    grid-template-columns: 110px 1fr;
    gap: 6px 16px;
    margin: 14px 0;
    font-size: 14px;
  }

  dt {
    color: var(--dim);
  }

  dd {
    margin: 0;
    min-width: 0;
    overflow-wrap: anywhere;
  }

  .ok {
    color: var(--ok);
  }

  .warn {
    color: var(--warn);
  }

  .other summary {
    cursor: pointer;
    color: var(--soft);
    font-weight: 600;
  }

  .other .actions {
    margin-top: 14px;
  }

  .link {
    border: 0;
    background: none;
    padding: 0;
    color: var(--amber);
    font-weight: 600;
  }
</style>

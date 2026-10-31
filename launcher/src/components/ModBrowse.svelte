<script lang="ts">
  // Browse: the mods of the signed index (core/src/index.rs), as CKAN lists them. On a desktop a
  // table with presets (Compatible, Installed, Upgradeable, Not installed, All), search (lib/
  // modSearch.ts, the website's scorer) and a details pane with Metadata, Relationships, Versions
  // and Files; on Android, cards. Install opens the change set (lib/deepLink.svelte.ts), which
  // asks before anything is downloaded. The index's text is shown as text, never as HTML; a video
  // is its picture and a link to YouTube in the browser, never played here.
  import { api, onAndroid, type IndexMod, type IndexView, type VersionView } from "$lib/api";
  import { app, guardShown } from "$lib/app.svelte";
  import { openChangeSet } from "$lib/deepLink.svelte";
  import {
    apiFits,
    imageUrl,
    inPreset,
    LAUNCHER_API,
    PRESETS,
    siteUrl,
    upgradeable,
    versionHere,
    youtube,
    type Preset,
    type Row,
  } from "$lib/modBrowse";
  import {
    BROWSE as T,
    DETAIL_TAB,
    IMPACT_LABEL,
    INDEX_STATE,
    PRESET_LABEL,
    didYouMean,
    modErrorText,
    noMatch,
    offlineText,
    revokedOffText,
    updateTo,
  } from "$lib/modsText";
  import { closest, highlight, search } from "$lib/modSearch";
  import { modErrorOf } from "$lib/api";
  import { onMount } from "svelte";
  import Icon from "./Icon.svelte";

  let { version }: { version: VersionView } = $props();

  let view = $state<IndexView | null>(null);
  let error = $state<string | null>(null);
  let installed = $state<Record<string, string>>({});
  let query = $state("");
  let preset = $state<Preset>("compatible");
  let selected = $state<string | null>(null);
  let tab = $state<keyof typeof DETAIL_TAB>("metadata");
  let searchBox = $state<HTMLInputElement | null>(null);

  /** The platform key the index names archives by, for this system (plugins::platform_key). */
  const platform = $derived.by(() => {
    switch (app.info?.platform) {
      case "android":
        return "android-arm64";
      case "windows":
        return "windows-x86_64";
      case "macos":
        return "macos-arm64";
      default:
        return "linux-x86_64";
    }
  });
  const prerelease = $derived(app.config?.prereleaseMods ?? app.config?.updateChannel === "dev");

  async function load() {
    error = null;
    try {
      view = await api.indexFetch(version.game, version.name);
      const mine = await api.pluginsList(version.game, version.name).catch(() => []);
      installed = Object.fromEntries(mine.map((p) => [p.id, p.version]));
    } catch (e) {
      const m = modErrorOf(e);
      error = m ? modErrorText(m) : T.failed;
    }
  }
  onMount(() => {
    void load();
  });

  const revoked = (id: string, v: string) =>
    !!view?.index.revoked.some((r) => r.id === id && (r.versions.includes("*") || r.versions.includes(v)));
  const rows = $derived.by((): Row[] =>
    (view?.index.mods ?? []).map((mod) => ({
      mod,
      here: versionHere(
        mod,
        { game: version.game, disc: version.name, platform, api: LAUNCHER_API, prerelease },
        revoked,
      ),
      installed: installed[mod.id] ?? null,
    })),
  );
  const shown = $derived(
    search(
      rows.filter((r) => inPreset(r, preset)).map((r) => ({ ...r, ...r.mod })),
      query,
    ),
  );
  const suggestion = $derived(
    shown.length === 0 && query
      ? closest(
          rows.map((r) => r.mod),
          query,
        )
      : null,
  );
  const current = $derived(rows.find((r) => r.mod.id === selected) ?? null);

  /** Needs the dev build: a version fits its API but none fits this launcher's. */
  const devOnly = (r: Row) => {
    const dev = view?.index.ports.dev.api.split(".").map(Number) as [number, number] | undefined;
    return !r.here && !!dev && r.mod.versions.some((v) => !v.prerelease && apiFits(v.api, dev));
  };

  function install(m: IndexMod) {
    void openChangeSet([{ id: m.id, version: null }], version.game);
  }
  function open(url: string) {
    void guardShown(api.openUrl(url));
  }

  function onkeydown(e: KeyboardEvent) {
    const typing = e.target instanceof HTMLInputElement || e.target instanceof HTMLTextAreaElement;
    if (e.key === "/" && !typing) {
      e.preventDefault();
      searchBox?.focus();
    } else if (e.key === "Escape" && e.target === searchBox) {
      query = "";
    }
  }
</script>

<svelte:window {onkeydown} />

<section class="panel browse" aria-label={T.tabTitle}>
  <div class="head">
    <label class="search">
      <span class="sr">{T.search}</span>
      <input bind:this={searchBox} type="search" placeholder={T.search} bind:value={query} />
      {#if !onAndroid}<kbd>{T.searchHint}</kbd>{/if}
    </label>
    <div class="presets" role="radiogroup" aria-label={T.presetsLabel}>
      {#each PRESETS as p (p)}
        <button
          role="radio"
          aria-checked={preset === p}
          class:on={preset === p}
          onclick={() => {
            preset = p;
          }}>{PRESET_LABEL[p]}</button
        >
      {/each}
    </div>
  </div>

  {#if view}
    {#if view.offline}<p class="quiet">{offlineText(view.fetchedAt)}</p>{/if}
    {#if view.stale}<p class="quiet">{INDEX_STATE.stale}</p>{/if}
    {#if view.newerFormat}<p class="quiet">{INDEX_STATE.newerFormat}</p>{/if}
    {#each view.turnedOff as r (r.id)}<p class="err">{revokedOffText(r.name, r.version, r.reason)}</p>{/each}
  {/if}

  {#if error}
    <p class="err" role="alert">{error}</p>
    <button class="small" onclick={() => void load()}><Icon name="refresh" size={14} />Try again</button>
  {:else if !view}
    <p class="muted"><span class="spinner"></span></p>
  {:else if rows.length === 0}
    <p class="muted">{T.empty}</p>
  {:else if shown.length === 0}
    <div class="none">
      <p>{noMatch(query)}</p>
      {#if suggestion}
        {@const s = suggestion}
        <button
          class="link"
          onclick={() => {
            query = s.name;
          }}>{didYouMean(s.name)}</button
        >
      {/if}
      <button
        class="small"
        onclick={() => {
          query = "";
          preset = "all";
        }}>{T.clearFilters}</button
      >
    </div>
  {:else if onAndroid}
    <ul class="cards">
      {#each shown as r (r.mod.id)}
        {@const icon = r.mod.media.icon ? imageUrl(r.mod.mediaBase, r.mod.media.icon, 96) : null}
        <li class="card">
          {#if icon}<img src={icon} alt="" width="48" height="48" loading="lazy" />{/if}
          <div class="grow">
            <strong>{r.mod.name}</strong>
            <span class="dim">{r.mod.author}</span>
            <p>{r.mod.description}</p>
          </div>
          {#if r.installed && !upgradeable(r)}
            <span class="dim">{T.installed}</span>
          {:else if r.here}
            <button
              class="small"
              onclick={() => {
                install(r.mod);
              }}>{upgradeable(r) ? updateTo(r.here.version) : T.install}</button
            >
          {:else}
            <span class="dim">{devOnly(r) ? T.devOnly : T.notHere}</span>
          {/if}
        </li>
      {/each}
    </ul>
  {:else}
    <div class="split">
      <table>
        <thead>
          <tr><th>{T.name}</th><th>{T.author}</th><th>{T.version}</th><th class="num">{T.downloads}</th><th></th></tr>
        </thead>
        <tbody>
          {#each shown as r (r.mod.id)}
            <tr class:sel={selected === r.mod.id} class:dimmed={!r.here}>
              <td>
                <button
                  class="name"
                  onclick={() => {
                    selected = r.mod.id;
                  }}
                >
                  {#each highlight(r.mod.name, query) as part, i (i)}{#if part.mark}<mark>{part.text}</mark
                      >{:else}{part.text}{/if}{/each}
                </button>
              </td>
              <td class="dim">{r.mod.author}</td>
              <td>{r.installed ?? r.here?.version ?? r.mod.versions[0]?.version ?? ""}</td>
              <td class="num">{r.mod.downloads.toLocaleString()}</td>
              <td>
                {#if r.installed && !upgradeable(r)}
                  <span class="dim">{T.installed}</span>
                {:else if r.here}
                  <button
                    class="small"
                    onclick={() => {
                      install(r.mod);
                    }}>{upgradeable(r) ? updateTo(r.here.version) : T.install}</button
                  >
                {:else}
                  <span class="dim">{devOnly(r) ? T.devOnly : T.notHere}</span>
                {/if}
              </td>
            </tr>
          {/each}
        </tbody>
      </table>

      {#if current}
        {@const m = current.mod}
        {@const banner = m.media.banner ? imageUrl(m.mediaBase, m.media.banner, 480) : null}
        <aside class="details" aria-label={T.detailsAria}>
          {#if banner}<img class="banner" src={banner} alt={m.media.banner?.alt ?? ""} loading="lazy" />{/if}
          <h3>{m.name}</h3>
          <p class="dim">{m.author}</p>
          <button
            class="link"
            onclick={() => {
              open(siteUrl(m.id));
            }}>{T.viewOnSite}<Icon name="external" size={14} /></button
          >
          <div class="dtabs" role="tablist">
            {#each Object.entries(DETAIL_TAB) as [key, label] (key)}
              <button
                role="tab"
                aria-selected={tab === key}
                class:on={tab === key}
                onclick={() => {
                  tab = key as keyof typeof DETAIL_TAB;
                }}>{label}</button
              >
            {/each}
          </div>
          {#if tab === "metadata"}
            <p>{m.description}</p>
            <dl>
              {#if m.license}<dt>{T.license}</dt>
                <dd>{m.license}</dd>{/if}
              <dt>{T.tags}</dt>
              <dd>{m.tags.join(", ") || T.nothing}</dd>
              <dt>{T.impact}</dt>
              <dd>{m.impact.map((i) => IMPACT_LABEL[i] ?? i).join(", ") || T.nothing}</dd>
              <dt>{T.updated}</dt>
              <dd>{m.updated.slice(0, 10)}</dd>
            </dl>
            {#each m.media.videos as video (video.youtube)}
              {@const yt = youtube(video.youtube)}
              {#if yt}
                <button
                  class="video"
                  onclick={() => {
                    open(yt.watch);
                  }}
                >
                  <img src={yt.thumb} alt={video.title} loading="lazy" />
                  <span>{T.watch}<Icon name="external" size={14} /></span>
                </button>
              {/if}
            {/each}
            {#each m.media.screenshots as shot (shot.hash)}
              {@const src = imageUrl(m.mediaBase, shot, 480)}
              {#if src}<figure>
                  <img {src} alt={shot.alt ?? ""} loading="lazy" />{#if shot.caption}<figcaption>
                      {shot.caption}
                    </figcaption>{/if}
                </figure>{/if}
            {/each}
            <pre class="readme">{m.readme || T.noReadme}</pre>
          {:else if tab === "relationships"}
            {@const v = current.here ?? m.versions[0]}
            <dl>
              <dt>{T.requires}</dt>
              <dd>
                {v && v.requires.length > 0
                  ? v.requires.map((r) => `${r.id} ${r.version ?? ""}`).join(", ")
                  : T.nothing}
              </dd>
              <dt>{T.conflicts}</dt>
              <dd>{v && v.conflicts.length > 0 ? v.conflicts.map((c) => c.id).join(", ") : T.nothing}</dd>
              <dt>{T.recommends}</dt>
              <dd>{m.recommends.join(", ") || T.nothing}</dd>
              <dt>{T.suggests}</dt>
              <dd>{m.suggests.join(", ") || T.nothing}</dd>
            </dl>
          {:else if tab === "versions"}
            <ul class="plain">
              {#each m.versions as v (v.version)}
                <li>
                  <strong>{v.version}</strong> <span class="dim">{v.released.slice(0, 10)} · API {v.api ?? "-"}</span>
                </li>
              {:else}<li>{T.noVersions}</li>{/each}
            </ul>
          {:else}
            <ul class="plain mono">
              {#each (current.here ?? m.versions[0])?.files ?? [] as f (f)}<li>{f}</li>{:else}<li>
                  {T.noFiles}
                </li>{/each}
            </ul>
          {/if}
        </aside>
      {/if}
    </div>
  {/if}
</section>

<style>
  .browse {
    display: flex;
    flex-direction: column;
    gap: 12px;
  }

  .head {
    display: flex;
    flex-wrap: wrap;
    gap: 10px;
    align-items: center;
  }

  .search {
    position: relative;
    flex: 1 1 240px;
  }

  .search input {
    width: 100%;
  }

  .search kbd {
    position: absolute;
    right: 10px;
    top: 50%;
    transform: translateY(-50%);
    color: var(--dim);
    font-size: 0.75rem;
  }

  .sr {
    position: absolute;
    width: 1px;
    height: 1px;
    overflow: hidden;
    clip: rect(0 0 0 0);
  }

  .presets {
    display: flex;
    flex-wrap: wrap;
    gap: 4px;
  }

  .presets button,
  .dtabs button {
    border: 1px solid var(--line);
    background: none;
    padding: 4px 10px;
    font-size: 0.8125rem;
    color: var(--soft);
  }

  .presets button.on,
  .dtabs button.on {
    background: var(--panel-hi);
    color: var(--amber);
    border-color: var(--line-strong);
  }

  .quiet {
    margin: 0;
    font-size: 0.8125rem;
    color: var(--dim);
  }

  .err {
    margin: 0;
    color: var(--err);
    font-size: 0.8125rem;
  }

  .split {
    display: grid;
    grid-template-columns: minmax(0, 1fr) minmax(0, 320px);
    gap: 16px;
    align-items: start;
  }

  table {
    width: 100%;
    border-collapse: collapse;
    font-size: 0.8125rem;
  }

  th {
    text-align: left;
    color: var(--dim);
    font-weight: 600;
    padding: 6px 8px;
    border-bottom: 1px solid var(--line-strong);
  }

  td {
    padding: 6px 8px;
    border-bottom: 1px solid var(--line);
    color: var(--body);
  }

  .num {
    text-align: right;
  }

  tr.sel td {
    background: var(--panel-hi);
  }

  tr.dimmed td {
    opacity: 0.6;
  }

  .name {
    border: 0;
    background: none;
    padding: 0;
    color: var(--text);
    font-weight: 600;
    text-align: left;
  }

  mark {
    background: color-mix(in srgb, var(--amber) 20%, transparent);
    color: var(--gold);
  }

  .details {
    display: flex;
    flex-direction: column;
    gap: 8px;
    padding: 12px;
    border: 1px solid var(--line);
    border-radius: var(--radius-sm);
    font-size: 0.8125rem;
    max-height: 70vh;
    overflow-y: auto;
  }

  .details h3 {
    margin: 0;
    color: var(--gold);
  }

  .details p {
    margin: 0;
  }

  .banner,
  figure img,
  .video img {
    width: 100%;
    height: auto;
    border-radius: var(--radius-sm);
  }

  figure {
    margin: 0;
  }

  figcaption {
    color: var(--dim);
  }

  .dtabs {
    display: flex;
    flex-wrap: wrap;
    gap: 4px;
  }

  dl {
    display: grid;
    grid-template-columns: auto 1fr;
    gap: 4px 10px;
    margin: 0;
  }

  dt {
    color: var(--dim);
  }

  dd {
    margin: 0;
  }

  .video {
    display: flex;
    flex-direction: column;
    gap: 4px;
    border: 0;
    background: none;
    padding: 0;
    color: var(--amber);
    text-align: left;
  }

  .video span {
    display: inline-flex;
    gap: 6px;
    align-items: center;
  }

  .readme {
    white-space: pre-wrap;
    font-family: var(--font-sans);
    margin: 0;
    color: var(--body);
  }

  .plain {
    margin: 0;
    padding: 0;
    list-style: none;
  }

  .mono {
    font-family: var(--font-mono);
    font-size: 0.75rem;
  }

  .cards {
    margin: 0;
    padding: 0;
    list-style: none;
    display: flex;
    flex-direction: column;
    gap: 10px;
  }

  .card {
    display: flex;
    gap: 12px;
    align-items: center;
    padding: 12px;
    border: 1px solid var(--line);
    border-radius: var(--radius-sm);
    background: var(--panel-hi);
  }

  .card p {
    margin: 4px 0 0;
    font-size: 0.8125rem;
  }

  .card img {
    border-radius: 8px;
  }

  .grow {
    flex: 1;
    min-width: 0;
    display: flex;
    flex-direction: column;
  }

  .none {
    display: flex;
    flex-wrap: wrap;
    gap: 10px;
    align-items: center;
  }

  .none p {
    margin: 0;
  }

  .link {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    border: 0;
    background: none;
    padding: 0;
    color: var(--amber);
    font-weight: 600;
    align-self: flex-start;
  }

  @media (max-width: 860px) {
    .split {
      grid-template-columns: 1fr;
    }
  }
</style>

<script lang="ts">
  // Making a game's texture pack (core/src/texture_packs.rs, docs/port/TEXTURE_PACKS.md), on Mods →
  // Models and textures (Android, and a launcher without the editor: a fold under My mods). Two steps
  // a player follows: save the game's pictures while playing, then repaint or upscale them into their
  // pack. The pack itself is a row of My mods' packs (PacksPanel.svelte), where it is switched on,
  // shared or replaced; a line here says so, with the way there. Counts per folder (Interface,
  // Shared, each planet) open that folder. Hashes, paths and the environment the game is started
  // with stay out of sight: they are in the docs.
  import Icon from "./Icon.svelte";
  import { api, type TextureFolder, type TexturePackView, type VersionView } from "$lib/api";
  import { toast } from "$lib/app.svelte";
  import { grouped } from "$lib/format";
  import { TEXTURES } from "$lib/modsText";
  import { isPlaying } from "$lib/player.svelte";
  import { textureCount } from "$lib/texturePacks";

  let {
    version,
    onshow,
    folded = false,
    open = $bindable(false),
  }: {
    version: VersionView;
    /** Show it in My mods: where the pack's row is (left out where the row is on the same page). */
    onshow?: () => void;
    /** A fold under My mods' lists (no tabs: Android, a launcher without the level editor). */
    folded?: boolean;
    /** Whether the fold is open (Make your own on the pack's row opens it). */
    open?: boolean;
  } = $props();

  let view = $state<TexturePackView | null>(null);
  let failed = $state<string | null>(null);
  let busy = $state(false);

  /** The newest read wins: switching games while one is read drops the older answer. */
  let reading = 0;
  async function load() {
    const mine = ++reading;
    try {
      const read = await api.texturePacks(version.game, version.name);
      if (mine === reading) {
        view = read;
        failed = null;
      }
    } catch (e) {
      if (mine === reading) failed = String(e);
    }
  }

  // Read again for another game or disc, and when the game stops: it may have saved textures.
  const playing = $derived(isPlaying(version));
  $effect(() => {
    if (version.key && !playing) void load();
  });

  async function work(run: () => Promise<void>) {
    busy = true;
    try {
      await run();
    } catch (e) {
      toast(String(e), "error");
    } finally {
      busy = false;
    }
  }

  const setDumping = (dumping: boolean) =>
    work(async () => {
      view = await api.texturePacksSet(version.game, version.name, { dumping });
      toast(dumping ? "The game saves its textures from your next Play." : "The game no longer saves its textures.");
    });

  const openFolder = (dump: boolean, folder: TextureFolder | null = null) =>
    work(async () => {
      await api.texturePacksOpen(version.game, version.name, dump, folder?.path ?? "");
    });
</script>

<svelte:element
  this={folded ? "details" : "section"}
  id="texture-make"
  class="panel textures"
  aria-label={TEXTURES.makeTitle}
  open={folded ? open : undefined}
  ontoggle={(e: Event) => {
    if (folded) open = (e.currentTarget as HTMLDetailsElement).open;
  }}
>
  {#if folded}
    <summary>
      <div class="grow">
        <h2>{TEXTURES.makeTitle}</h2>
        <p class="muted lead">{TEXTURES.makeLead}</p>
      </div>
      <span class="chevron"><Icon name="down" size={18} /></span>
    </summary>
  {:else}
    <h2>{TEXTURES.makeTitle}</h2>
    <p class="muted lead">{TEXTURES.makeLead}</p>
  {/if}
  {#if failed}
    <p class="muted"><Icon name="warn" size={14} /> {failed}</p>
  {:else if view}
    <ol class="steps">
      <li>
        <div>
          <h3>Save the game's textures while you play</h3>
          <p class="muted">
            Every picture the game shows is saved once, sorted into Interface, Shared and a folder for each planet. They
            come from your disc: keep them to yourself.
          </p>
        </div>
        <div class="controls">
          <label class="toggle">
            <span>{view.dumping ? "On" : "Off"}</span>
            <input
              type="checkbox"
              class="switch"
              role="switch"
              aria-label="Save the game's textures while you play"
              checked={view.dumping}
              disabled={busy}
              onchange={(e) => void setDumping(e.currentTarget.checked)}
            />
          </label>
          <button class="ghost small" disabled={busy} onclick={() => void openFolder(true)}>
            <Icon name="folder" size={14} />Open saved textures
          </button>
        </div>
        {#if view.dumpCount}
          <div class="span counts" aria-label="Saved textures by folder">
            <span class="total">{textureCount(view.dumpCount)} saved</span>
            {#each view.dump as folder (folder.path)}
              <button class="count" title={`Open ${folder.label}`} onclick={() => void openFolder(true, folder)}>
                {folder.label}<b>{grouped(folder.count)}</b>
              </button>
            {/each}
          </div>
        {/if}
      </li>
      <li>
        <div>
          <h3>Make your own pictures</h3>
          <p class="muted">
            Open a saved picture in any image editor, upscale or repaint it at any size, and keep its file name. Put it
            in your pack, in any folder.
          </p>
          <p class="muted">
            Animated: put numbered pictures (0001.png, 0002.png…) in a folder named like the saved picture, without
            .png.
          </p>
        </div>
        <div class="controls">
          <button class="ghost small" disabled={busy} onclick={() => void openFolder(false)}>
            <Icon name="folder" size={14} />Open your pack
          </button>
        </div>
        {#if view.packCount}
          <div class="span counts" aria-label="Your pack by folder">
            <span class="total">{textureCount(view.packCount)} in your pack</span>
            {#each view.pack as folder (folder.path)}
              <button class="count" title={`Open ${folder.label}`} onclick={() => void openFolder(false, folder)}>
                {folder.label}<b>{grouped(folder.count)}</b>
              </button>
            {/each}
          </div>
        {/if}
      </li>
    </ol>
    <p class="where">
      <Icon name="info" size={14} /><span class="grow">{TEXTURES.whereItIs}</span>
      {#if onshow}<button class="small" onclick={onshow}>{TEXTURES.goToMine}</button>{/if}
    </p>
  {/if}
</svelte:element>

<style>
  .textures:not(details) {
    display: flex;
    flex-direction: column;
    gap: 6px;
  }
  /* Folded (Android, and a launcher without the editor): an arrow says it opens. */
  details.textures summary {
    display: flex;
    align-items: center;
    gap: 12px;
    list-style: none;
    cursor: pointer;
    border-radius: var(--radius-sm);
  }
  details.textures summary:focus-visible {
    outline: 2px solid var(--amber);
    outline-offset: 4px;
  }
  .chevron {
    flex: none;
    display: inline-flex;
    color: var(--soft);
    transition: transform var(--medium) var(--ease);
  }
  details.textures[open] .chevron {
    transform: rotate(180deg);
  }
  details.textures summary::-webkit-details-marker {
    display: none;
  }
  details.textures[open] summary {
    margin-bottom: 10px;
  }
  .lead {
    margin: 0;
  }
  .toggle {
    display: inline-flex;
    align-items: center;
    gap: 8px;
    font-size: 0.8125rem;
    color: var(--soft);
    cursor: pointer;
  }
  ol.steps .controls button {
    min-width: 0;
  }
  /* A folder's count: its name and number, opening it. */
  .counts {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 6px;
  }
  .total {
    font-size: 0.8125rem;
    color: var(--soft);
    margin-right: 4px;
  }
  .count {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    max-width: 100%;
    padding: 3px 10px;
    font-size: 0.8125rem;
    color: var(--lav);
    background: var(--ink-deep);
    border-radius: var(--radius-sm);
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }
  .count b {
    color: var(--amber);
    font-weight: 600;
    font-variant-numeric: tabular-nums;
  }

  h2 {
    font-size: 1rem;
  }
  /* Where the pack is: My mods, with the other packs. */
  .where {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 8px 10px;
    margin: 4px 0 0;
    padding: 8px 10px 8px 12px;
    border-radius: var(--radius-sm);
    background: var(--card-hi);
    --control: var(--control-card);
    --control-hi: var(--control-card-hi);
    font-size: 0.8125rem;
  }
  .where > :global(svg) {
    flex: none;
    color: var(--amber);
  }
</style>

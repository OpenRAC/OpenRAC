<script lang="ts">
  // A game's packs on My mods: what changes how the game looks and sounds. First the player's own
  // texture pack (core/src/texture_packs.rs; made in TexturePacksBlock.svelte), a row with its switch,
  // Share and Replace; then the overrides packs (textures, models, levels and sound;
  // core/src/overrides.rs): switch one on or off, change the order (a pack lower in the list wins:
  // its grip drags it, or Up and Down), remove one. Every change is instant. Adding is AddMod's
  // (lib/addMods.ts), a texture pack's .zip too. The packs are the game's, shared by its discs. With
  // many mods, My mods' toolbar (lib/modsView.svelte.ts) finds them and sorts them by name.
  import { api, ask, pickZip, type PackView, type TexturePackView, type VersionView } from "$lib/api";
  import { app, guardShown, toast } from "$lib/app.svelte";
  import { dragSort } from "$lib/dragSort";
  import { arrange, matches, steps } from "$lib/modsList";
  import { PACKS, TEXTURES, VIEW } from "$lib/modsText";
  import { forget, modsView, movable, report, searching, shows } from "$lib/modsView.svelte";
  import { isPlaying } from "$lib/player.svelte";
  import { grouped } from "$lib/format";
  import { packSummary, replaceSummary, textureCount } from "$lib/texturePacks";
  import CommandBox from "./CommandBox.svelte";
  import Icon from "./Icon.svelte";
  import Menu from "./Menu.svelte";
  import Reason from "./Reason.svelte";

  let {
    version: v,
    reload = 0,
    commands = [],
    textures = false,
    onmake,
    onchange,
  }: {
    version: VersionView;
    /** Bumped when something else made a pack (AddMod, an import): the list is read again. */
    reload?: number;
    /** With developer tools on: the same steps from a shell. */
    commands?: string[];
    /** Whether the game has a texture pack (a port that plays, and its disc set up). */
    textures?: boolean;
    /** Make your own: where the texture pack's steps are (Models and textures, or a fold). */
    onmake?: () => void;
    onchange?: () => void;
  } = $props();

  const developer = $derived(app.config?.developer ?? false);

  let packs = $state<PackView[]>([]);
  let packsError = $state<string | null>(null);
  let pack = $state<TexturePackView | null>(null);

  async function load(withTextures: boolean) {
    try {
      packs = await api.overridesList(v.game);
      packsError = null;
    } catch (e) {
      packsError = typeof e === "string" ? e : String(e);
    }
    pack = withTextures ? await api.texturePacks(v.game, v.name).catch(() => null) : null;
  }

  // Read again when RELOAD is bumped, and when the game stops: it may have saved textures.
  const playing = $derived(isPlaying(v));
  $effect(() => {
    if (reload >= 0 && v.key && !playing) void load(textures);
  });

  /** A change to the list: the list as it now is, or, refused (said in a toast), read again, as
   *  a move of several places may have stopped half way. */
  async function change(work: Promise<PackView[]>) {
    const result = await guardShown(work);
    if (result) {
      packs = result;
      onchange?.();
    } else {
      await load(textures);
    }
  }

  async function remove(pack: PackView) {
    const message = `Remove "${pack.name}"? The launcher's copy is deleted; the .zip or folder you added it from is kept.`;
    if (!(await ask(message, PACKS.remove))) return;
    void change(api.overridesRemove(v.game, pack.id));
  }

  /** Moves PACK to where TARGET is, one place at a time (the core moves by one). */
  /** A move is running: the next waits (each is several one-place moves). */
  let moving = $state(false);
  async function moveTo(pack: PackView | undefined, target: PackView | undefined) {
    if (moving || !pack || !target || target.id === pack.id) return;
    const from = packs.findIndex((p) => p.id === pack.id);
    const to = packs.findIndex((p) => p.id === target.id);
    if (from < 0 || to < 0) return;
    moving = true;
    try {
      await change(
        (async () => {
          let list = packs;
          for (const up of steps(from, to)) list = await api.overridesMove(v.game, pack.id, up);
          return list;
        })(),
      );
    } finally {
      moving = false;
    }
  }

  function summary(pack: PackView): string {
    if (pack.isDestination) return "Added ship destination · original planets stay available";
    if (pack.isMap) return "Standalone map · starts separately from the campaign";
    const parts = [];
    if (pack.textures) parts.push(`${pack.textures} texture${pack.textures === 1 ? "" : "s"}`);
    if (pack.models) parts.push(`${pack.models} model${pack.models === 1 ? "" : "s"}`);
    if (pack.levelFiles) parts.push(`${pack.levelFiles} level edit${pack.levelFiles === 1 ? "" : "s"}`);
    const where = pack.levels === 1 ? "1 level" : `${pack.levels} levels`;
    const levels = parts.length ? [`${parts.join(", ")} in ${where}`] : [];
    const sounds = pack.sounds ? [`${pack.sounds} sound${pack.sounds === 1 ? "" : "s"}`] : [];
    return [...levels, ...sounds].join(", ") || "Replaces nothing the game reads";
  }

  /** The packs as shown: searched, in their order or by name. */
  const shown = $derived(arrange(packs, modsView.query, modsView.sort, (p) => [p.name, p.author, p.description]));
  const packShown = $derived(!!pack && matches(modsView.query, TEXTURES.name, TEXTURES.badge));

  // What the list holds, for the toolbar (counts, Mods off, a search that finds nothing).
  const found = $derived(shows("looks") ? shown.length + (packShown ? 1 : 0) : 0);
  $effect(() => {
    report("packs", {
      counts: { looks: packs.length + (pack ? 1 : 0) },
      on: packs.filter((p) => p.enabled).length + (pack?.enabled && pack.packCount > 0 ? 1 : 0),
      found,
    });
  });
  $effect(() => () => {
    forget("packs");
  });

  // The texture pack's own changes.
  const setTextures = (enabled: boolean) =>
    guardShown(api.texturePacksSet(v.game, v.name, { enabled })).then((next) => {
      if (next) pack = next;
    });

  const share = () =>
    guardShown(api.texturePacksExport(v.game, v.name)).then((done) => {
      if (!done?.saved) return;
      const left = done.leftOut ? ` ${grouped(done.leftOut)} unchanged picture(s) from the game were left out.` : "";
      toast(`Saved ${textureCount(done.textures)} as ${done.saved}.${left}`);
    });

  async function replace() {
    const source = await pickZip(TEXTURES.pickTitle);
    if (!source) return;
    const preview = await guardShown(api.texturePacksPreview(v.game, v.name, source));
    if (!preview || !(await ask(replaceSummary(preview), TEXTURES.replaceYes))) return;
    const done = await guardShown(api.texturePacksImport(v.game, v.name, source, "replace"));
    if (!done) return;
    toast(TEXTURES.replaced(textureCount(done.textures), done.kept ?? null));
    await load(textures);
  }
</script>

{#if shows("looks") && (packs.length > 0 || pack !== null || packsError !== null || modsView.filter === "looks") && !(searching() && found === 0)}
  <section class="panel packs" aria-label={PACKS.title}>
    <div class="row head">
      <div class="grow">
        <h2>{PACKS.title}</h2>
        <p class="dim sub">
          {PACKS.sub}{#if packs.length > 1}<span title={PACKS.moveDownTitle}>&nbsp;· {PACKS.orderHint}</span>{/if}
        </p>
      </div>
    </div>

    {#if pack && packShown}
      <!-- The player's own texture pack: one per game, above the others, made on Make your own. -->
      <div class="pack own" class:off={!pack.enabled}>
        <input
          type="checkbox"
          role="switch"
          class="switch"
          aria-label={`Use ${TEXTURES.name}`}
          checked={pack.enabled}
          onchange={(e) => void setTextures(e.currentTarget.checked)}
        />
        <div class="grow">
          <div class="name">
            {TEXTURES.name}<span class="tag" title={TEXTURES.badgeHint}
              ><Icon name="image" size={12} />{TEXTURES.badge}</span
            >
          </div>
          <div class="dim small-text">{packSummary(pack)}</div>
        </div>
        {#if onmake}
          <button class="small" onclick={onmake}><Icon name="edit" size={14} />{TEXTURES.make}</button>
        {/if}
        <Menu
          label={`More for ${TEXTURES.name}`}
          items={[
            ...(pack.packCount ? [{ label: TEXTURES.share, icon: "upload" as const, run: () => void share() }] : []),
            { label: TEXTURES.replace, icon: "download" as const, run: () => void replace() },
            {
              label: TEXTURES.openFolder,
              icon: "folder" as const,
              run: () => void guardShown(api.texturePacksOpen(v.game, v.name, false)),
            },
          ]}
        />
      </div>
    {/if}

    {#if packsError}
      <Reason text={packsError} tone="err" />
    {:else if packs.length === 0}
      {#if !pack}<p class="dim empty">{PACKS.empty}</p>{/if}
    {:else if shown.length === 0}
      <!-- The search found only the texture pack: nothing more to say. -->
    {:else}
      <ul use:dragSort={{ enabled: movable() && !moving, onmove: (from, to) => void moveTo(shown[from], shown[to]) }}>
        {#each shown as p, i (p.id)}
          <li class="pack" class:off={!p.enabled}>
            {#if movable() && shown.length > 1}
              <span class="grip" data-grip title={VIEW.grip(p.name)} aria-hidden="true"
                ><Icon name="grip" size={16} /></span
              >
            {/if}
            <input
              type="checkbox"
              role="switch"
              class="switch"
              aria-label={`Use ${p.name}`}
              checked={p.enabled}
              onchange={(e) => void change(api.overridesSetEnabled(v.game, p.id, e.currentTarget.checked))}
            />
            <div class="grow">
              <div class="name">
                {p.name}{#if p.author}<span class="by">by {p.author}</span>{/if}
              </div>
              <div class="dim small-text">
                {summary(p)}{#if p.description}. {p.description}{/if}
              </div>
            </div>
            {#if developer}<code class="chip" title="The pack's id">{p.id}</code>{/if}
            <div class="order">
              {#if movable()}
                <button
                  class="icon"
                  aria-label={`Move ${p.name} up`}
                  title={PACKS.moveUpTitle}
                  disabled={moving || i === 0}
                  onclick={() => void moveTo(p, shown[i - 1])}><Icon name="up" size={14} /></button
                >
                <button
                  class="icon"
                  aria-label={`Move ${p.name} down`}
                  title={PACKS.moveDownTitle}
                  disabled={moving || i === shown.length - 1}
                  onclick={() => void moveTo(p, shown[i + 1])}><Icon name="down" size={14} /></button
                >
              {/if}
              <button
                class="icon remove"
                aria-label={`Remove ${p.name}`}
                title={PACKS.remove}
                onclick={() => {
                  void remove(p);
                }}><Icon name="trash" size={14} /></button
              >
            </div>
          </li>
        {/each}
      </ul>
      {#if !movable()}<p class="dim empty">{VIEW.orderFrozen}</p>{/if}
    {/if}

    {#if developer && commands.length}<CommandBox lines={commands} />{/if}
  </section>
{/if}

<style>
  /* What follows a name on its line, a space after it (Svelte trims the space). */
  .by {
    margin-left: 0.3em;
  }

  .packs {
    display: flex;
    flex-direction: column;
    gap: 8px;
  }

  h2 {
    font-size: 1rem;
  }

  .sub {
    margin: 2px 0 0;
    font-size: 0.8125rem;
  }

  ul {
    margin: 0;
    padding: 0;
    list-style: none;
  }

  .pack {
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 11px 0;
    border-top: 1px solid var(--line);
  }

  .tag {
    display: inline-flex;
    align-items: center;
    gap: 4px;
    margin-left: 8px;
    padding: 1px 8px;
    border-radius: var(--radius-xs);
    background: var(--ink-deep);
    font-size: 0.6875rem;
    font-weight: 600;
    color: var(--lav);
    vertical-align: 2px;
  }

  .pack input:focus-visible {
    outline: 2px solid var(--amber);
    outline-offset: 2px;
  }

  .name {
    font-size: 0.875rem;
    font-weight: 600;
  }

  .by {
    font-weight: 400;
    color: var(--dim);
  }

  .pack.off .name {
    color: var(--dim);
  }

  .small-text {
    font-size: 0.8125rem;
  }

  .chip {
    flex: none;
    max-width: 30%;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    padding: 2px 8px;
    border-radius: 7px;
    background: var(--control);
    font-size: 0.75rem;
    color: var(--soft);
  }

  .order {
    flex: none;
    display: flex;
    gap: 6px;
  }

  .icon {
    padding: 4px 9px;
    font-size: 0.75rem;
  }

  .remove {
    color: var(--err);
  }

  .empty {
    font-size: 0.875rem;
    padding: 4px 0;
  }
</style>

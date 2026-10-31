<script lang="ts">
  // Mods, one game at a time, with the game list on the left as in Games: every game gets the
  // same page. Its head: the game's name (the game starts
  // with its mods from the game's page: the Mods switch under Play). Then three tabs, so that each of the player's jobs has a place of
  // its own that can be seen from the top: My mods (AddMod: a link, a file or a folder; the
  // gameplay mods, PluginsPanel; the texture, model and level packs,
  // PacksPanel), Level editor (open a level in Godot, pack the changes) and Models and textures
  // (AssetsPanel). A game the editor cannot open but that plays has My mods alone; one the editor
  // cannot open yet says so, and where to follow it. A game it reads in early access (Going
  // Commando, Up Your Arsenal, Deadlocked: no pack or play action of theirs yet) says so in a line.
  // The words are lib/modsText.ts's. The disc image is never changed.
  // The version is the one the game page plays (playerVersion: the player's own disc), so every
  // disc goes the same way and no region is named; packs are shared by the discs
  // (editor/overrides.py stacks them for the one played). With developer tools on: the version,
  // its folders, and the command behind each step under a "Show command" toggle.
  import AddMod from "$components/AddMod.svelte";
  import AssetsPanel from "$components/AssetsPanel.svelte";
  import CommandBox from "$components/CommandBox.svelte";
  import EditorFlow from "$components/EditorFlow.svelte";
  import GameChip from "$components/GameChip.svelte";
  import GamesFrame from "$components/GamesFrame.svelte";
  import Icon from "$components/Icon.svelte";
  import PacksPanel from "$components/PacksPanel.svelte";
  import ModBrowse from "$components/ModBrowse.svelte";
  import ModLoadSummary from "$components/ModLoadSummary.svelte";
  import ModUpdatesPanel from "$components/ModUpdatesPanel.svelte";
  import PluginsPanel from "$components/PluginsPanel.svelte";
  import { api, onAndroid, type ActionView, type GameView, type VersionView } from "$lib/api";
  import { app, guardShown, jobs, run } from "$lib/app.svelte";
  import type { EntryState } from "$lib/labels";
  import { SITE_URL } from "$lib/links";
  import { BROWSE, EDITOR, GAME_LIST, PAGE } from "$lib/modsText";
  import { playerVersion } from "$lib/player.svelte";
  import type { Job } from "$lib/queue";
  import { arrive, enter } from "$lib/motion";
  import { followHistory, modFromAddress, modNav, openMod, resetMod } from "$lib/modNav.svelte";
  import { changeSet } from "$lib/deepLink.svelte";
  import { onMount, untrack } from "svelte";
  import { tablist } from "$lib/tabs";
  import { themeStyle } from "$lib/themes";
  import { fade, fly } from "svelte/transition";

  const developer = $derived(app.config?.developer ?? false);
  const games = $derived(app.library?.games ?? []);

  const action = (v: VersionView, id: string): ActionView | null =>
    v.actions.find((a) => a.id === id && a.thisPlatform) ?? null;

  /** The version of a game the player modifies, the one its page plays, if the level editor can open it. */
  function editable(game: GameView): VersionView | null {
    const v = playerVersion(game);
    const extract = v ? action(v, "editor-extract") : null;
    return v && extract && extract.state !== "planned" ? v : null;
  }

  const standalone = $derived(app.info?.standalone ?? false);

  /** A version's folders: its level projects and packed levels. In a checkout they are its own
   *  (launcher/actions.json); an installed launcher's are in the game's data folder, where
   *  tools/bundle_standalone.py points its actions (install.rs, data_dir). */
  const folders = (v: VersionView) => {
    if (standalone && app.config?.installDir) {
      const data = `${app.config.installDir}/active/${v.game}/data`;
      return {
        projects: `${data}/levels/${v.name}`,
        packed: `${data}/packed/${v.name}`,
        levels: `${data}/port/${v.name}`,
      };
    }
    return {
      projects: `assets/levels/${v.game}/${v.name}`,
      packed: `assets/packed/${v.game}/${v.name}`,
      levels: `assets/port/${v.game}/${v.name}`,
    };
  };

  /** Opens one of those folders: a checkout's through its root, an installed launcher's in the data folder. */
  const openFolder = (v: VersionView, path: string) =>
    guardShown(standalone ? api.openGameFolder(v.game, path) : api.openPath(path));

  const live = (j: Job) => j.state === "queued" || j.state === "running";
  const jobsOf = (v: VersionView, ids: string[]) =>
    jobs.list.filter((j) => j.scope.kind === "version" && j.scope.key === v.key && ids.includes(j.actionId));
  const running = (v: VersionView, ids: string[]) => jobsOf(v, ids).find(live) ?? null;

  /** Early access: the editor reads the game's levels, but they are not packed back and the game
   *  is not playable yet (the version has no editor-pack, and its play is planned). */
  const early = (v: VersionView) => !action(v, "editor-pack");
  const playable = (v: VersionView) => {
    const play = action(v, "play");
    return !!play && play.state !== "planned";
  };

  /** What the game list says for each game on this tab. */
  function modState(game: GameView): EntryState {
    const v = editable(game);
    if (!v) {
      const pv = playerVersion(game);
      return pv && playable(pv)
        ? { text: GAME_LIST.gameplayMods, tone: "ok" }
        : { text: GAME_LIST.notModdable, tone: "info" };
    }
    if (
      early(v) &&
      v.status.disc.state === "found" &&
      !running(v, ["editor-extract", "assets-export", "assets-import", "port-levels"])
    ) {
      return { text: GAME_LIST.early, tone: "info" };
    }
    if (running(v, ["editor-extract", "editor-import", "editor-pack", "assets-export", "assets-import"])) {
      return { text: GAME_LIST.working, tone: "warn", busy: true };
    }
    if (v.status.disc.state !== "found") return { text: GAME_LIST.setUp, tone: "warn" };
    return { text: GAME_LIST.ready, tone: "ok" };
  }

  // The game shown: the one picked in the list, else the one the player last opened if it can be
  // modded, else the first that can, else the first.
  let picked = $state<string | null>(null);
  const shown = $derived.by(() => {
    const byId = (id: string | null | undefined) => games.find((g) => g.id === id) ?? null;
    const last = byId(app.version?.split("/")[0]);
    // Android has no list here: the game is the one picked in Games, whatever it can do.
    return (
      byId(picked) ??
      (last && (onAndroid || editable(last)) ? last : null) ??
      games.find((g) => editable(g)) ??
      games[0] ??
      null
    );
  });
  const v = $derived(shown ? editable(shown) : null);
  /** The version the game page plays, when it has a port: its gameplay mods need no level editor,
   *  so an installed launcher without the editor's tools still shows them. */
  const played = $derived.by(() => {
    const pv = shown ? playerVersion(shown) : null;
    return pv && playable(pv) ? pv : null;
  });

  /** Android's one page of mods (no level editor there): the version they are for. */
  const mine = $derived(onAndroid ? (v ?? played) : null);

  /** The page's tab: My mods, the level editor, or models and textures. */
  type Tab = "mine" | "browse" | "editor" | "assets";
  // ?tab=editor (or assets) opens that tab at start: handy for screenshots, as ?page= is (App.svelte).
  const askedTab = typeof location === "undefined" ? null : new URLSearchParams(location.search).get("tab");
  let tab = $state<Tab>(askedTab === "editor" || askedTab === "assets" || askedTab === "browse" ? askedTab : "mine");
  const TABS: { id: Tab; label: string; icon: "library" | "edit" | "folder" | "globe" }[] = [
    { id: "mine", label: PAGE.tabMine, icon: "library" },
    { id: "browse", label: BROWSE.tabTitle, icon: "globe" },
    { id: "editor", label: PAGE.tabEditor, icon: "edit" },
    { id: "assets", label: PAGE.tabAssets, icon: "folder" },
  ];
  /** Bumped when a pack was added or imported: the packs' list is read again. */
  let reload = $state(0);
  /** Bumped when a gameplay mod was added: their list is read again. */
  let pluginsReload = $state(0);
  const added = (what: { plugin: boolean; pack: boolean }) => {
    if (what.plugin) pluginsReload += 1;
    if (what.pack) reload += 1;
  };

  // A change set applied (DeepLinkInstall): the lists are read again.
  $effect(() => {
    if (changeSet.phase !== "done") return;
    untrack(() => {
      pluginsReload += 1;
      reload += 1;
    });
  });

  // A mod's own page (PluginsPanel shows it in place of My mods' lists): Back and Escape return;
  // another game or tab leaves it. ?mod=<id> opens one at start.
  const onModPage = $derived(modNav.mod !== null);
  onMount(() => {
    const asked = modFromAddress(window.location);
    if (asked) openMod(asked);
    const stop = followHistory();
    return () => {
      stop();
      resetMod();
    };
  });
  $effect(() => {
    if (modNav.mod === null) return;
    // Only a game with gameplay mods has their pages.
    if (v ? !playable(v) : !played) resetMod();
    else tab = "mine";
  });
</script>

<!-- Android: no game list; the mods are for the game picked in Games, named at the top right. -->
<GamesFrame
  label="Mods"
  selected={shown?.id ?? null}
  onselect={onAndroid
    ? undefined
    : (g) => {
        if (g.id !== shown?.id) resetMod();
        picked = g.id;
      }}
  state={modState}
>
  {#if shown}
    {#key shown.id}
      <div class="page" style={themeStyle(shown.id)} in:fly={arrive()}>
        <div class="intro">
          <div class="grow">
            <h1 class:plain={onAndroid}>{onAndroid ? "Mods" : shown.title}</h1>
            {#if v ? !early(v) : played}<p class="muted lead">{PAGE.lead}</p>{/if}
          </div>
          {#if onAndroid}
            <GameChip title={shown.title} what="mods" />
          {/if}
        </div>

        {#if onAndroid && mine}
          <!-- Android: one page, what a player does there in the order they do it. The mods first,
               with Enhanced graphics at the top and its switch in its row; adding one, from a link or
               a .zip the system's picker copies in; then the texture and model packs. No level editor
               or model export: they need Godot and a desktop. -->
          <PluginsPanel version={mine} reload={pluginsReload} onpack={() => (reload += 1)} />
          {#if !onModPage}
            <ModUpdatesPanel version={mine} onchange={() => (pluginsReload += 1)} />
            <AddMod version={mine} links={playable(mine)} onadded={added} />
            {#if playable(mine)}<ModBrowse version={mine} />{/if}
            <PacksPanel version={mine} {reload} />
          {/if}
        {:else if !v && played}
          <!-- The level editor cannot open it here (an installed launcher without its tools, say), but the
               game plays: its gameplay mods, which load when it is played from its page. -->
          {#if !onModPage}<AddMod version={played} onadded={added} />{/if}
          <PluginsPanel version={played} reload={pluginsReload} onpack={() => (reload += 1)} />
          {#if !onModPage}
            <ModUpdatesPanel version={played} onchange={() => (pluginsReload += 1)} />
            <ModBrowse version={played} />
          {/if}
        {:else if !v}
          <!-- Every game has this page; one the editor cannot open yet says so. -->
          <section class="panel none">
            <h2>{PAGE.noneTitle}</h2>
            <p class="muted">{PAGE.noneLine}</p>
            <p>
              <button class="link" onclick={() => void guardShown(api.openUrl(SITE_URL))}
                >{PAGE.noneLink}<Icon name="external" size={14} /></button
              >
            </p>
          </section>
        {:else}
          {@const scope = { kind: "version" as const, key: v.key }}
          {@const adder = action(v, "overrides-add")}
          {@const dirs = folders(v)}

          {#if early(v)}
            <!-- Honest about early access, in a line: what works, what comes later. -->
            <p class="early"><strong>{PAGE.earlyTitle}</strong><span class="muted">{PAGE.earlyLine}</span></p>
          {/if}

          {#if !onModPage}<div class="tabs" role="tablist" aria-label={PAGE.tabsLabel} use:tablist>
              {#each TABS as t (t.id)}
                <button
                  role="tab"
                  id={`mods-tab-${t.id}`}
                  aria-controls="mods-panel"
                  aria-selected={tab === t.id}
                  tabindex={tab === t.id ? 0 : -1}
                  class:on={tab === t.id}
                  onclick={() => (tab = t.id)}><Icon name={t.icon} size={16} />{t.label}</button
                >
              {/each}
            </div>{/if}

          {#key tab}
            <div
              id="mods-panel"
              class="tabpanel"
              role="tabpanel"
              aria-labelledby={`mods-tab-${tab}`}
              in:fade={{ duration: enter().duration ?? 0 }}
            >
              {#if tab === "mine" && onModPage && playable(v)}
                <PluginsPanel version={v} reload={pluginsReload} onpack={() => (reload += 1)} />
              {:else if tab === "mine"}
                <AddMod version={v} links={playable(v)} files={playable(v) || !!adder?.runnable} onadded={added} />
                {#if playable(v)}
                  <!-- Gameplay mods need the game's port: they plug into it (docs/port/MODS.md). -->
                  <PluginsPanel version={v} reload={pluginsReload} onpack={() => (reload += 1)} />
                  <ModUpdatesPanel version={v} onchange={() => (pluginsReload += 1)} />
                {/if}
                <PacksPanel
                  version={v}
                  {reload}
                  onchange={() => reload++}
                  commands={[`python3 editor/overrides.py add <pack.zip> --into <data>/overrides`]}
                />
                <p class="make-own">
                  <span class="muted">{PAGE.makeOwn}</span>
                  <button class="small" onclick={() => (tab = "editor")}
                    ><Icon name="edit" size={14} />{PAGE.makeOwnButton}</button
                  >
                </p>
              {:else if tab === "browse"}
                {#if playable(v)}<ModBrowse version={v} />{/if}
              {:else if tab === "editor"}
                <EditorFlow game={shown} version={v} onchanged={() => reload++} />
                {#if v.game === "rac1"}
                  <ModLoadSummary
                    version={v}
                    refreshKey={`${reload}:${jobsOf(v, ["editor-pack", "overrides-stack"])
                      .filter((j) => !live(j))
                      .map((j) => j.key)
                      .join(",")}`}
                    canPrepare={!!action(v, "overrides-stack")?.runnable && !running(v, ["overrides-stack"])}
                    prepare={() => {
                      const stack = action(v, "overrides-stack");
                      if (stack) void run(scope, stack);
                    }}
                  />
                {/if}
              {:else}
                <section class="panel">
                  <AssetsPanel version={v} onimported={() => reload++} />
                </section>
              {/if}
            </div>
          {/key}

          {#if developer && tab !== "assets" && !onModPage}
            <!-- The developer view: kept apart from the player's, under the tabs, folded (its folders
                 name the user's). -->
            <details class="panel sdk more">
              <summary><h2>{PAGE.developerTitle}</h2></summary>
              <p class="dim">
                This is <code>{v.key}</code>, the version of the disc set up. Level projects:
                <code>{dirs.projects}/level_NN</code>; packed levels: <code>{dirs.packed}</code>.
              </p>
              <CommandBox
                lines={[
                  ...(tab === "editor"
                    ? [
                        `python3 editor/extract.py godot <disc.iso> ${dirs.projects} --split`,
                        `python3 editor/extract.py pack <disc.iso> ${dirs.projects} ${dirs.packed}`,
                      ]
                    : []),
                  `python3 editor/overrides.py stack --into <data>/overrides --out <data>/mods/${v.name} --packed ${dirs.packed} --levels ${dirs.levels}`,
                  `openrac-${v.key.replace("/", "-")} --data <data> --levels ${dirs.levels} --mods <data>/mods/${v.name}`,
                ]}
              />
              <div class="row">
                <button class="small" onclick={() => void openFolder(v, dirs.projects)}
                  ><Icon name="folder" size={14} />{EDITOR.projects}</button
                >
                <button class="small" onclick={() => void openFolder(v, dirs.packed)}
                  ><Icon name="folder" size={14} />{EDITOR.packedFolder}</button
                >
                <button class="small" onclick={() => void guardShown(api.openPath("editor/README.md"))}
                  ><Icon name="book" size={14} />{EDITOR.docs}</button
                >
              </div>
            </details>
          {/if}
        {/if}
      </div>
    {/key}
  {/if}
</GamesFrame>

<style>
  .page {
    max-width: var(--page-max, 960px);
    margin: 0 auto;
    padding: var(--page-top, 24px) var(--page-x, 24px) var(--page-bottom, 48px);
    display: flex;
    flex-direction: column;
    gap: var(--page-gap, 16px);
  }

  /* The head: the game's name, and Play with mods at its right. */
  .intro {
    display: flex;
    align-items: flex-start;
    gap: 16px 24px;
    flex-wrap: wrap;
  }

  .lead {
    margin: 4px 0 0;
  }

  /* The game's name in its own typeface and colour, as on its page. */
  h1 {
    font-family: var(--title-font);
    font-weight: var(--title-weight);
    color: var(--tx);
  }

  h1.plain {
    font-family: var(--font-head);
    font-weight: 600;
    color: var(--text);
  }

  .none {
    display: flex;
    flex-direction: column;
    gap: 10px;
  }

  .none h2 {
    font-size: 1rem;
  }

  .none p {
    margin: 0;
    max-width: 68ch;
  }

  /* Early access: one line, dashed, as what is not there yet (docs/STYLE.md). */
  .early {
    display: flex;
    flex-wrap: wrap;
    gap: 4px 10px;
    margin: 0;
    padding: 10px 16px;
    border: 1px dashed var(--line-strong);
    border-radius: var(--radius-sm);
    font-size: 0.875rem;
  }

  .early strong {
    color: var(--gold);
  }

  /* The page's tabs: the header's round tabs, larger, with an icon each, so the level editor can be
     seen from the top of the page. */
  .tabs {
    display: flex;
    align-self: flex-start;
    flex-wrap: wrap;
    gap: 4px;
    padding: 4px;
    border: 1px solid var(--line-strong);
    border-radius: 999px;
    background: var(--panel);
  }

  .tabs button {
    display: inline-flex;
    align-items: center;
    gap: 8px;
    border: 0;
    background: none;
    padding: 8px 18px;
    font-size: 0.875rem;
    font-weight: 600;
    color: var(--soft);
    transition:
      color var(--fast) var(--ease),
      background var(--fast) var(--ease);
  }

  .tabs button.on {
    background: var(--panel-hi);
    color: var(--amber);
    box-shadow: inset 0 0 0 1px var(--line-strong);
  }

  @media (hover: hover) {
    .tabs button:hover:not(.on) {
      color: var(--text);
    }
  }

  .tabs button:focus-visible {
    outline: 2px solid var(--amber);
    outline-offset: 2px;
  }

  .tabpanel {
    display: flex;
    flex-direction: column;
    gap: var(--page-gap, 16px);
  }

  .make-own {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    justify-content: center;
    gap: 10px;
    margin: 4px 0 0;
    font-size: 0.875rem;
  }

  .sdk {
    border-style: dashed;
    font-size: 0.8125rem;
  }

  .sdk > :global(:not(summary)) {
    margin-top: 10px;
  }

  .sdk h2 {
    font-size: 0.875rem;
  }

  .sdk p {
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
  }
</style>

<script lang="ts">
  // Settings, Files: every folder OpenRAC keeps the player's files in, in one place (core
  // files.rs), the same list as the game menu's Files tab. No other page opens a folder. The
  // picked game's own folders follow the player's; Developer tools' output shows with them on.
  // A place is opened by its id, never by a path.
  import { api, errorText, type FilePlace, onAndroid } from "$lib/api";
  import { app, guardShown } from "$lib/app.svelte";
  import { supports } from "$lib/capabilities";

  import Icon from "./Icon.svelte";

  const versions = $derived(app.library?.games.flatMap((g) => g.versions) ?? []);
  let picked = $state<string | null>(app.version);
  const version = $derived(
    versions.find((v) => v.key === picked) ?? versions.find((v) => v.key === app.version) ?? null,
  );
  let places = $state<FilePlace[]>([]);
  let failed = $state<string | null>(null);

  let asked = 0;
  $effect(() => {
    const game = version?.game ?? null;
    const name = version?.name ?? null;
    const mine = ++asked;

    // Read again when Developer tools are switched: the core lists their folders then.
    if (app.config?.developer === undefined) return;

    api.filePlaces(game, name).then(
      (found) => {
        if (mine !== asked) return;
        places = found;
        failed = null;
      },
      (e: unknown) => {
        if (mine === asked) failed = errorText(e);
      },
    );
  });

  const GROUPS: { id: FilePlace["group"]; title: string }[] = [
    { id: "yours", title: "Your files" },
    { id: "game", title: "This game" },
    { id: "developer", title: "Developer tools" },
  ];

  const counted = (n: number) =>
    n === 0 ? "Empty" : n >= 1000 ? "1000+ files" : n === 1 ? "1 file" : `${n} files`;

  const open = (place: FilePlace) =>
    void guardShown(api.openFilePlace(place.id, version?.game ?? null, version?.name ?? null));
</script>

<section class="panel files" id="files">
  <h2>Files</h2>
  <p class="dim lead">
    Every folder OpenRAC keeps your files in.{onAndroid
      ? " They open in the Files app, under OpenRAC. Uninstalling asks whether to keep the app's data: keep it, and all of it is back when you install OpenRAC again."
      : ""}
  </p>

  {#if failed}
    <p class="err note" role="alert">{failed}</p>
  {/if}

  {#each GROUPS as group (group.id)}
    {@const rows = places.filter((p) => p.group === group.id)}
    {#if rows.length}
      <div class="head">
        <h3>{group.id === "game" && version ? version.title : group.title}</h3>
        {#if group.id === "game" && versions.length > 1}
          <select aria-label="Game" bind:value={picked}>
            {#each versions as v (v.key)}
              <option value={v.key}>{v.title} · {v.region}</option>
            {/each}
          </select>
        {/if}
      </div>
      {#each rows as place (place.id)}
        <div class="setting">
          <span class="text">
            <strong>{place.title}</strong>
            <small>{place.help}</small>
            <small class="mono path" title={place.path}>{place.path}</small>
          </span>
          <span class="dim count">{counted(place.count)}</span>
          <button
            class="small"
            disabled={!supports(app.info, "openFolders")}
            aria-label={`Open ${place.title}`}
            onclick={() => {
              open(place);
            }}><Icon name="folder" size={14} />Open</button
          >
        </div>
      {/each}
    {/if}
  {/each}
</section>

<style>
  .lead {
    margin: 0 0 8px;
    font-size: 0.8125rem;
  }

  .head {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    margin-top: 14px;
  }

  .head h3 {
    margin: 0;
  }

  .path {
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .count {
    flex: none;
    font-size: 0.8125rem;
  }
</style>

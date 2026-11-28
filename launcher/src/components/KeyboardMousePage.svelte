<script lang="ts">
  // Settings, Controls, Keyboard and mouse: the built-in entry's page (core keyboard_mouse.rs) for
  // one game version, the keys, mouse and gyro tabs as ModPage.svelte draws them. Back returns to
  // Settings.
  import { api, errorText, type PluginView } from "$lib/api";
  import { app } from "$lib/app.svelte";

  import ModPage from "./ModPage.svelte";
  import Reason from "./Reason.svelte";

  let { version, onback }: { version: string; onback: () => void } = $props();

  const v = $derived(
    app.library?.games.flatMap((g) => g.versions).find((x) => x.key === version) ?? null,
  );
  let mod = $state<PluginView | null>(null);
  let failed = $state<string | null>(null);
  let asked = 0;

  $effect(() => {
    if (!v) return;
    const mine = ++asked;

    api.keyboardMouseView(v.game, v.name).then(
      (found) => {
        if (mine !== asked) return;
        mod = found;
        failed = found ? null : "This game has no keyboard and mouse settings.";
      },
      (e: unknown) => {
        if (mine === asked) failed = errorText(e);
      },
    );
  });

  async function save(key: string, value: unknown) {
    if (!v) return;
    const mine = ++asked;
    const found = await api.keyboardMouseSetSetting(v.game, v.name, key, value);

    if (mine === asked) mod = found;
  }

  const none = () => undefined;
</script>

{#if mod}
  <ModPage
    {mod}
    developer={app.config?.developer ?? false}
    onsave={save}
    onenable={none}
    onupdate={none}
    onpin={none}
    onhomepage={none}
    onremove={none}
    {onback}
    backTo="Settings"
  />
{:else if failed}
  <Reason text={failed} tone="err" />
  <button onclick={onback}>Back to Settings</button>
{/if}

<script lang="ts">
  // One version of a game. A player sees three steps (set up from your disc, play,
  // edit levels); with developer tools on in Settings, the contributor's
  // page with every action, the inputs and the progress.
  import { app } from "$lib/app.svelte";
  import GameDeveloper from "./GameDeveloper.svelte";
  import GamePlayer from "./GamePlayer.svelte";
</script>

{#if app.config?.developer}<GameDeveloper />{:else}<GamePlayer />{/if}

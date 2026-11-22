<script lang="ts">
  // Settings: the multiplayer server every game's online play goes to (core/src/multiplayer.rs),
  // the official one unless the player names another, as anyone can host one (OpenRAC/Server).
  // Play gives it to the game as OPENRAC_SERVER=host:port; the multiplayer mod and Up Your
  // Arsenal's online play read it. "Test connection" makes a short TCP connection to it.
  import { api, errorText } from "$lib/api";
  import {
    hostProblem,
    isOfficial,
    OFFICIAL_HOST,
    OFFICIAL_PORT,
    portProblem,
    serverAddress,
  } from "$lib/multiplayerServer";

  let { host = $bindable(), port = $bindable() }: { host: string; port: number } = $props();

  const hostError = $derived(hostProblem(host));
  const portError = $derived(portProblem(port));
  const problem = $derived(hostError ?? portError);
  const official = $derived(isOfficial(host, port));

  /** What the last test found, for the server it tested; another server typed clears it. */
  let tested = $state<{ address: string; ok: boolean; text: string } | null>(null);
  let testing = $state(false);
  const address = $derived(problem ? "" : serverAddress(host, port));
  const shown = $derived(tested?.address === address ? tested : null);

  async function test() {
    const at = address;

    testing = true;

    try {
      await api.multiplayerServerTest(host.trim(), port);
      tested = { address: at, ok: true, text: `Reachable: ${at} answered.` };
    } catch (e) {
      tested = { address: at, ok: false, text: `Unreachable: ${errorText(e)}` };
    }

    testing = false;
  }

  function reset() {
    host = OFFICIAL_HOST;
    port = OFFICIAL_PORT;
  }
</script>

<div class="field">
  <label for="multiplayer-server-host">Multiplayer server</label>
  <div class="row">
    <input
      id="multiplayer-server-host"
      type="text"
      class="mono host"
      autocomplete="off"
      autocapitalize="off"
      spellcheck="false"
      maxlength="253"
      aria-label="Host"
      aria-invalid={!!hostError}
      aria-describedby="multiplayer-server-note"
      placeholder={OFFICIAL_HOST}
      bind:value={host}
    />
    <span class="colon" aria-hidden="true">:</span>
    <input
      id="multiplayer-server-port"
      type="number"
      class="mono port"
      min="1"
      max="65535"
      step="1"
      inputmode="numeric"
      aria-label="Port"
      aria-invalid={!!portError}
      aria-describedby="multiplayer-server-note"
      placeholder={String(OFFICIAL_PORT)}
      bind:value={port}
    />
  </div>
  <p id="multiplayer-server-note" class="dim note">
    {problem ??
      (official
        ? "The official OpenRAC server. Online play in every game goes here."
        : "Your own server: online play in every game goes here. Everyone you play with uses the same one.")}
  </p>
  <p class="dim note actions">
    <button class="link" disabled={official} onclick={reset}>Reset to official</button>
    ·
    <button class="link" disabled={!!problem || testing} onclick={() => void test()}
      >Test connection</button
    >
    {#if testing}<span role="status">· Connecting…</span>{:else if shown}<span
        role="status"
        class:ok={shown.ok}
        class:err={!shown.ok}>· {shown.text}</span
      >{/if}
  </p>
</div>

<style>
  .field {
    display: flex;
    flex-direction: column;
    gap: 6px;
    padding: 12px 0 2px;
    border-top: 1px solid var(--line);
  }

  .field label {
    font-size: 0.8125rem;
    font-weight: 600;
  }

  .row {
    display: flex;
    align-items: center;
    gap: 6px;
    max-width: 400px;
  }

  .host {
    flex: 1;
    min-width: 0;
  }

  .port {
    width: 6.5em;
    flex: none;
  }

  .note {
    font-size: 0.8125rem;
  }

  .link {
    border: 0;
    background: none;
    box-shadow: none;
    padding: 0;
    min-height: 0;
    color: var(--amber);
    font-size: 0.8125rem;
  }

  .link:disabled {
    opacity: 0.5;
    cursor: default;
  }

  .ok {
    color: var(--ok);
  }

  .err {
    color: var(--err);
  }
</style>

<script lang="ts">
  // The website's progress bar: the game's colours come from the --bd, --tx
  // and --tx2 custom properties of the element around it (themes.ts).
  let { percent, label }: { percent: number; label: string } = $props();
</script>

<div
  class="bar"
  role="progressbar"
  aria-label={label}
  aria-valuenow={Math.round(percent * 10) / 10}
  aria-valuemin={0}
  aria-valuemax={100}
>
  <i style:width={`${Math.min(100, Math.max(0, percent))}%`}></i>
</div>

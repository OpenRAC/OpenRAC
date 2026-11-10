<script lang="ts">
  // Behind Android's Games: a wide band of fine lines in the game's colour, bending slowly across
  // the screen; it slides along as the wheel of discs turns (`shift`, in discs). One still frame
  // when motion is reduced.
  let { shift = 0, still = false }: { shift?: number; still?: boolean } = $props();

  let canvas = $state<HTMLCanvasElement | null>(null);

  const LINES = 44;

  /** The band's line `v` (0 to 1) at `u` across the screen (0 to 1), as a share of the height. */
  function band(u: number, v: number, t: number): number {
    return (
      0.74 +
      Math.sin(u * 3.4 + t * 0.32) * 0.12 +
      Math.sin(u * 1.3 - t * 0.21 + 1.7) * 0.072 +
      (v - 0.5) * 0.42 * (0.25 + 0.75 * Math.abs(Math.cos(u * 2.2 + t * 0.17)))
    );
  }

  /** The colour as numbers: --glow is a registered colour, so it reads back as rgb(). */
  function rgb(el: Element): [number, number, number] {
    const [r, g, b] = (
      getComputedStyle(el)
        .getPropertyValue("--glow")
        .match(/[\d.]+/g) ?? []
    ).map(Number);
    return r === undefined || g === undefined || b === undefined ? [244, 190, 64] : [r, g, b];
  }

  let shown = 0;
  $effect(() => {
    shown = shift;
  });

  $effect(() => {
    const c = canvas;
    if (!c) return;
    const x = c.getContext("2d");
    if (!x) return;
    let w = 0,
      h = 0,
      frame = 0,
      colour = rgb(c),
      read = 0,
      slide = shown;
    const size = () => {
      // Fine lines need no more than this, and a phone's 3x would triple the work.
      const dpr = Math.min(devicePixelRatio || 1, 1.5);
      w = c.clientWidth;
      h = c.clientHeight;
      c.width = Math.round(w * dpr);
      c.height = Math.round(h * dpr);
      x.setTransform(dpr, 0, 0, dpr, 0, 0);
    };
    const draw = (t: number) => {
      const [r, g, b] = colour;
      const light = (a: number) => `rgba(${r + (255 - r) * 0.8},${g + (255 - g) * 0.8},${b + (255 - b) * 0.8},${a})`;
      x.clearRect(0, 0, w, h);
      const sky = x.createLinearGradient(0, 0, 0, h);
      sky.addColorStop(0, `rgba(${r},${g},${b},0.16)`);
      sky.addColorStop(0.7, `rgba(${r},${g},${b},0.03)`);
      sky.addColorStop(1, "rgba(0,0,0,0)");
      x.fillStyle = sky;
      x.fillRect(0, 0, w, h);
      x.globalCompositeOperation = "lighter";
      x.lineWidth = 1;
      for (let k = 0; k < LINES; k++) {
        const v = k / (LINES - 1);
        x.beginPath();
        for (let px = -10; px <= w + 10; px += 5) {
          const y = band(px / w + slide * 0.08, v, t) * h;
          if (px < 0) x.moveTo(px, y);
          else x.lineTo(px, y);
        }
        x.strokeStyle = light(0.07 + 0.22 * Math.sin(v * Math.PI) ** 2);
        x.stroke();
      }
      x.globalCompositeOperation = "source-over";
    };
    const start = performance.now();
    const tick = (now: number) => {
      frame = requestAnimationFrame(tick);
      if (document.hidden) return;
      // The colour follows the game's as it changes (a transition), read a few times a second.
      if (now - read > 120) {
        colour = rgb(c);
        read = now;
      }
      slide += (shown - slide) * 0.08;
      draw(10 + (now - start) / 1000);
    };
    const observer = new ResizeObserver(() => {
      size();
      if (still) draw(10);
    });
    observer.observe(c);
    size();
    if (still) draw(10);
    else frame = requestAnimationFrame(tick);
    return () => {
      cancelAnimationFrame(frame);
      observer.disconnect();
    };
  });
</script>

<canvas bind:this={canvas} aria-hidden="true"></canvas>

<style>
  canvas {
    position: absolute;
    inset: 0;
    width: 100%;
    height: 100%;
  }
</style>

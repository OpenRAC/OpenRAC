<script lang="ts">
  // Behind Android's pages: a ribbon of light in the game's colour, like silk or a wave of water,
  // folding and drifting slowly across the screen, with specks of light about it. It slides along
  // as the wheel of discs turns (`shift`, in discs). One still frame when motion is reduced.
  let { shift = 0, still = false }: { shift?: number; still?: boolean } = $props();

  let canvas = $state<HTMLCanvasElement | null>(null);

  /** The ribbon's sheets, each a fold of the same cloth: where it lies (share of the height), how
   *  far it rises and falls, how wide it is, and its own pace. */
  const SHEETS = [
    { mid: 0.63, rise: 0.09, wide: 0.1, pace: 1, phase: 0, light: 1 },
    { mid: 0.66, rise: 0.07, wide: 0.07, pace: 0.8, phase: 2.1, light: 0.7 },
    { mid: 0.6, rise: 0.06, wide: 0.05, pace: 1.25, phase: 4.2, light: 0.55 },
  ];
  /** Threads in a sheet: drawn small and blurred, they merge into one smooth cloth, brighter where
   *  it folds (its threads crowd there). */
  const THREADS = 26;

  /** The middle and half-width of sheet `s` at `u` across (0 to 1), in shares of the height. */
  function sheet(s: (typeof SHEETS)[number], u: number, t: number): [number, number] {
    const q = t * s.pace;
    const mid =
      s.mid +
      Math.sin(u * 2.3 + q * 0.21 + s.phase) * s.rise +
      Math.sin(u * 4.1 - q * 0.13 + s.phase * 1.7) * s.rise * 0.45;
    const half = s.wide * (0.08 + 0.92 * Math.abs(Math.cos(u * 1.9 - q * 0.11 + s.phase * 0.6)));
    return [mid, half];
  }

  /** Specks of light about the ribbon: where along it, how far off it, size, pace, twinkle. */
  const SPECKS = Array.from({ length: 70 }, (_, i) => {
    const r = (n: number) => (((Math.sin(i * 12.9898 + n * 78.233) * 43758.5453) % 1) + 1) % 1;
    return { u: r(1), off: (r(2) - 0.5) * 2, size: 0.5 + r(3) ** 3 * 2, pace: 0.004 + r(4) * 0.012, tw: r(5) * 6.28 };
  });

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
      // A soft picture needs no more than this, and a phone's 3x would triple the work.
      const dpr = Math.min(devicePixelRatio || 1, 1.5);
      w = c.clientWidth;
      h = c.clientHeight;
      c.width = Math.round(w * dpr);
      c.height = Math.round(h * dpr);
      x.setTransform(dpr, 0, 0, dpr, 0, 0);
    };
    // The cloth is drawn at a third of the size, then blurred onto the page: soft, and cheap.
    const cloth = document.createElement("canvas");
    const cx = cloth.getContext("2d");
    const draw = (t: number) => {
      const [r, g, b] = colour;
      const tint = (white: number, a: number) =>
        `rgba(${r + (255 - r) * white},${g + (255 - g) * white},${b + (255 - b) * white},${a})`;
      x.clearRect(0, 0, w, h);
      // The deep below the wave holds a little of its light.
      const deep = x.createLinearGradient(0, h * 0.45, 0, h);
      deep.addColorStop(0, "rgba(0,0,0,0)");
      deep.addColorStop(1, tint(0, 0.13));
      x.fillStyle = deep;
      x.fillRect(0, 0, w, h);
      const along = slide * 0.08;
      if (cx) {
        const cw = Math.max(1, Math.round(w / 3)),
          ch = Math.max(1, Math.round(h / 3));
        if (cloth.width !== cw || cloth.height !== ch) {
          cloth.width = cw;
          cloth.height = ch;
        }
        cx.clearRect(0, 0, cw, ch);
        cx.globalCompositeOperation = "lighter";
        cx.lineWidth = 1;
        for (const s of SHEETS) {
          for (let k = 0; k < THREADS; k++) {
            const v = k / (THREADS - 1) - 0.5;
            cx.beginPath();
            for (let px = -4; px <= cw + 4; px += 3) {
              const [mid, half] = sheet(s, px / cw + along, t);
              const y = (mid + v * 2 * half) * ch;
              if (px < 0) cx.moveTo(px, y);
              else cx.lineTo(px, y);
            }
            // Its upper edge catches the light; its middle is thinnest.
            const edge = k === 0 ? 2.2 : k === THREADS - 1 ? 1.2 : 0.55 + 0.45 * Math.abs(v * 2);
            cx.strokeStyle = tint(0.55, 0.075 * s.light * edge);
            cx.stroke();
          }
        }
        x.save();
        x.globalCompositeOperation = "lighter";
        x.filter = "blur(2px)";
        x.drawImage(cloth, 0, 0, w, h);
        x.filter = "none";
        x.globalAlpha = 0.5;
        x.drawImage(cloth, 0, 0, w, h);
        x.restore();
      }
      // The specks, near the ribbon, drifting along it and twinkling.
      x.globalCompositeOperation = "lighter";
      const main = SHEETS[0];
      if (main)
        for (const p of SPECKS) {
          const u = (p.u + t * p.pace) % 1;
          const [mid, half] = sheet(main, u + along, t);
          const y = (mid + p.off * (half + 0.05)) * h;
          const a = 0.2 + 0.6 * Math.sin(t * 1.3 + p.tw) ** 4;
          x.fillStyle = tint(0.85, a);
          x.beginPath();
          x.arc(u * w, y, p.size, 0, Math.PI * 2);
          x.fill();
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

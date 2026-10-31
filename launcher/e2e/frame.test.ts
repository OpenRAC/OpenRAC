// The desktop's tabs with a list (SideFrame), as the browser preview draws them: the list and the
// page each scroll in their own column, and <main> never does; and the list is there when a slide
// takes its picture of the next page, even on the first press, its page's code still on its way.
// WebKitGTK stops drawing the list during a slide if <main> becomes scrollable part way through it
// (docs/STYLE.md). Chromium, which runs this, does not, so this holds the layout that keeps it from
// happening, not the drawing. `yarn test:android` runs it with the Android pages
// (vitest.e2e.config.ts).
import { chromium, type Browser, type Page } from "playwright-core";
import { createServer, type ViteDevServer } from "vite";
import { afterAll, afterEach, beforeAll, beforeEach, describe, expect, it } from "vitest";

declare global {
  interface Window {
    /** The slides so far (countSlides). */
    slides: { started: number; finished: number; parts: string[] };
  }
}

let server: ViteDevServer;
let browser: Browser;
let page: Page;
let base = "";

beforeAll(async () => {
  server = await createServer({
    configFile: "vite.config.ts",
    logLevel: "error",
    server: { port: 0, strictPort: false },
  });
  await server.listen();
  base = server.resolvedUrls?.local[0] ?? "";
  const executablePath = import.meta.env.OPENRAC_E2E_CHROMIUM as string | undefined;
  browser = await chromium.launch(executablePath ? { executablePath } : {});
}, 60_000);

afterAll(async () => {
  await browser.close();
  await server.close();
});

/** Runs in the page before its own scripts: counts the slides, and keeps which parts of the page
 *  the last one moves on its own. */
function countSlides() {
  window.slides = { started: 0, finished: 0, parts: [] };
  const start = document.startViewTransition.bind(document);
  document.startViewTransition = (update) => {
    window.slides.started += 1;
    const transition = start(update);
    const done = () => {
      window.slides.finished += 1;
    };
    // A skipped slide has no parts, and still finishes.
    transition.ready
      .then(() => {
        window.slides.parts = document
          .getAnimations()
          .map((animation) => (animation.effect as KeyframeEffect | null)?.pseudoElement ?? "")
          .filter((part) => part.includes("view-transition"));
      })
      .catch(() => undefined);
    void transition.finished.then(done, done);
    return transition;
  };
}

beforeEach(async () => {
  page = await browser.newPage({ viewport: { width: 1200, height: 780 } });
  await page.addInitScript(countSlides);
});

afterEach(async () => {
  await page.close();
});

/** Opens the preview on Settings, the tab without a list, once the slide there is over. */
async function openSettings(query = "") {
  await page.goto(`${base}?latest&page=settings${query}`);
  await page.locator("button.tab[data-page=settings][aria-current=page]").waitFor();
  await page.waitForFunction(() => window.slides.finished === window.slides.started);
}

/** Presses TAB and watches every frame until its slide is over and what its page loads has had
 *  time to arrive. A press during a slide would make no slide of its own, so `slides` says that
 *  this one did. `overflow`: how far <main> overflowed, at its worst, while the list was there. */
async function press(tab: string) {
  return page.evaluate(async (tab) => {
    const before = window.slides.started;
    const wantsList = tab !== "settings";
    document.querySelector<HTMLElement>(`button.tab[data-page=${tab}]`)?.click();
    let overflow = -1;
    let end = performance.now() + 10_000;
    let over = false;
    while (performance.now() < end) {
      await new Promise(requestAnimationFrame);
      const main = document.querySelector("main");
      const list = !!main?.querySelector("aside");
      if (main && list) overflow = Math.max(overflow, main.scrollHeight - main.clientHeight);
      const { started, finished } = window.slides;
      if (!over && started > before && finished === started && list === wantsList) {
        over = true;
        end = performance.now() + 500;
      }
    }
    return { slides: window.slides.started - before, overflow, parts: window.slides.parts };
  }, tab);
}

describe("a desktop tab with a list", () => {
  for (const [width, height] of [
    [1200, 780],
    [960, 560],
  ] as const) {
    it(`never makes <main> scrollable, in a ${width} x ${height} window`, async () => {
      await page.setViewportSize({ width, height });
      await openSettings("&developer");
      for (const tab of ["library", "mods", "develop"]) {
        const shown = await press(tab);
        // At 0, not -1: the list did come, and <main> had nothing to scroll on any frame.
        expect(shown, tab).toMatchObject({ slides: 1, overflow: 0 });
        expect((await press("settings")).slides, tab).toBe(1);
      }
    });
  }

  it("has the list in the slide on the first press, its page's code still on its way", async () => {
    // Once, so that the preview's server has the code ready and only the delay below remains.
    await page.goto(`${base}?latest&page=mods`);
    await page.locator("main aside").waitFor();
    // The page's code takes longer than the slide's picture, every time, on any machine.
    await page.route("**/pages/Mods.svelte*", async (route) => {
      await new Promise((done) => setTimeout(done, 300));
      await route.continue();
    });
    await openSettings();
    const fetched = await page.evaluate(() =>
      performance.getEntriesByType("resource").some((entry) => entry.name.includes("Mods")),
    );
    expect(fetched).toBe(false);
    expect((await press("mods")).parts).toContain("::view-transition-new(side)");
  });

  it("scrolls the page beside the list, which stays put", async () => {
    await openSettings();
    await press("mods");
    // What the page loads makes it taller than this window.
    await page.waitForFunction(() => {
      const column = document.querySelector("main aside + div");
      return !!column && column.scrollHeight > column.clientHeight;
    });
    const moved = await page.evaluate(() => {
      const list = document.querySelector("main aside");
      const column = list?.nextElementSibling;
      const inPage = column?.firstElementChild;
      if (!list || !column || !inPage) return null;
      const before = { list: list.getBoundingClientRect().top, page: inPage.getBoundingClientRect().top };
      column.scrollTop = 100;
      return {
        list: list.getBoundingClientRect().top - before.list,
        page: inPage.getBoundingClientRect().top - before.page,
        main: document.querySelector("main")?.scrollTop,
      };
    });
    expect(moved).toEqual({ list: 0, page: -100, main: 0 });
  });
});
